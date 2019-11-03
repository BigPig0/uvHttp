#include "uvnetplus.h"
#include "uvnettcppool.h"
#include "utilc.h"
#include "util.h"
#include <time.h>
#include <sstream>

namespace uvNetPlus {

//////////////////////////////////////////////////////////////////////////
//////////////   ���ӵ��������ĵ�������    ///////////////////////////////

CUNTcpPoolSocket::CUNTcpPoolSocket(CUVNetPlus* net, bool copy)
    : CUNTcpSocket(net, copy)
{
}

CUNTcpPoolSocket::~CUNTcpPoolSocket()
{
    //Log::debug("~CUNTcpConnect()");
}

void CUNTcpPoolSocket::syncClose()
{
    m_pReq = nullptr;
    m_pAgent->GiveBackSkt(this);
}

void CUNTcpPoolSocket::Delete()
{
    m_pNet->AddEvent(ASYNC_EVENT_TCP_CONNCLOSE, this);
}

//////////////////////////////////////////////////////////////////////////
/** ָ����ַ�����ӹ��� */

static void on_uv_getaddrinfo(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
    CTcpPoolAgent *agent = (CTcpPoolAgent*)req->data;
    free(req);
    agent->OnParseHost(status, res);
    uv_freeaddrinfo(res);
}


void CTcpPoolAgent::Delete()
{

}

CTcpPoolAgent::CTcpPoolAgent(CUVNetPlus* net, CUNTcpConnPool *p)
    : m_pNet(net)
    , m_pTcpConnPool(p)
{
}

CTcpPoolAgent::~CTcpPoolAgent() {
    Log::debug("~CTcpPoolAgent()");
}

void CTcpPoolAgent::syncHostDns(string host){
    //�ж�host����������ip
    if(net_is_ip(host.c_str()) > 0) {
        m_listIP.push_back(host);
    } else {
        //dns����host
        uv_getaddrinfo_t *req = new uv_getaddrinfo_t;
        struct addrinfo *hints = new struct addrinfo;
        req->data = this;
        hints->ai_family = PF_UNSPEC;
        hints->ai_socktype = SOCK_STREAM;
        hints->ai_protocol = IPPROTO_TCP;
        hints->ai_flags = 0;
        uv_getaddrinfo(&m_pNet->pNode->m_uvLoop, req, on_uv_getaddrinfo, host.c_str(), NULL, hints);
    }
}

void CTcpPoolAgent::OnParseHost(int status, struct addrinfo* res) {
    if(status < 0) {
        return;
    }

    while(res) {
        if(res->ai_family == PF_INET) {
            char addr[17] = {0};
            uv_ip4_name((struct sockaddr_in*)res->ai_addr, addr, 17);
            m_listIP.push_back(addr);
        } else if(res->ai_family == PF_INET6) {
            char addr[46] = {0};
            uv_ip6_name((struct sockaddr_in6*)res->ai_addr, addr, 46);
            m_listIP.push_back(addr);
        }
        res = res->ai_next;
    }

    Request((CTcpRequest*)NULL);
}

bool CTcpPoolAgent::Request(CTcpRequest *req) {
    //�ⲿ�����ȷŵ������б�
    if(req) {
        m_listReqs.push_back(req);
    }

    //host������δ�ɹ�, û����Ҫ���������
    if(m_listIP.empty() || m_listReqs.empty()){
        return false;
    }

    //û�п�������
    //Log::debug("busy %d idle %d max %d", m_listBusyConns.size(), m_listIdleConns.size(), m_nMaxConns);
    if(m_listBusyConns.size() >= maxConns){
        //���������ﵽ����
        //Log::debug("busy conn is max");
        return false;
    }

    // ȡ����һ������
    req = m_listReqs.front();
    m_listReqs.pop_front();
    //Log::debug("busyConn %d idleConn %d", m_listBusyConns.size(), m_listIdleConns.size());
    //���ҿ�������
    if(!m_listIdleConns.empty()){
        CUNTcpPoolSocket *conn = m_listIdleConns.front();
        m_listIdleConns.pop_front();

        //������һ������
        m_pNet->AddEvent(ASYNC_EVENT_TCPAGENT_REQUEST, this);

        // ʹ�����е����ӽ��з�������
        //Log::debug("use a idle connect send");
        conn->m_nLastTime = time(NULL);
        m_listBusyConns.push_back(conn);
        conn->m_pReq = req;

        //�ص�֪ͨ�û�ȡ����socket
        if(m_pTcpConnPool->OnRequest)
            m_pTcpConnPool->OnRequest(req, conn, true);
    } else {
        //��������δ�����ޣ���������l������
        //Log::debug("create a new connect and send");
        // �������ӣ��������ӳɹ���������
        string ip = m_listIP.front();
        m_listIP.pop_front();
        m_listIP.push_back(ip);

        //������һ������
        m_pNet->AddEvent(ASYNC_EVENT_TCPAGENT_REQUEST, this);

        CUNTcpPoolSocket *conn = new CUNTcpPoolSocket(m_pNet, req->copy);
        conn->m_nLastTime = time(NULL);
        m_listBusyConns.push_back(conn);
        conn->m_pReq = req;
        conn->m_pAgent = this;

        //�ص�֪ͨ�û�ȡ����socket
        if(m_pTcpConnPool->OnRequest)
            m_pTcpConnPool->OnRequest(req, conn, false);

        conn->Connect(ip, port);
    }

    return true;
}

void CTcpPoolAgent::GiveBackSkt(CUNTcpPoolSocket *skt) {   
    m_listBusyConns.remove(skt);
    if(skt->m_bConnect && m_listIdleConns.size() < maxIdle){
        m_listIdleConns.push_front(skt);
    } else {
        skt->CUNTcpSocket::Delete();
    }
    m_pNet->AddEvent(ASYNC_EVENT_TCPAGENT_REQUEST, this);
}

//////////////////////////////////////////////////////////////////////////
/** TCP�ͻ������ӳ� */

static void on_timer_cb(uv_timer_t* handle) {
    CUNTcpConnPool *pool = (CUNTcpConnPool*)handle->data;
    time_t now = time(NULL);
    //��������agent
    for(auto it = pool->m_mapAgents.begin(); it != pool->m_mapAgents.end(); ){
        CTcpPoolAgent* agent = it->second;
        while(!agent->m_listIdleConns.empty()){
            CUNTcpPoolSocket *conn = agent->m_listIdleConns.back();
            if(difftime(now, conn->m_nLastTime) < agent->timeOut)
                break;

            delete conn;
            agent->m_listIdleConns.pop_back();
        }

        // ���ĳ��agent��û�����ӡ������������
        if(agent->m_listBusyConns.empty()
            && agent->m_listIdleConns.empty()
            && agent->m_listReqs.empty()){
                delete agent;
                it = pool->m_mapAgents.erase(it);
        } else {
            it++;
        }
    }
}

static void on_timer_close(uv_handle_t* handle) {
    uv_timer_t* t = (uv_timer_t*)handle;
    delete t;
}

CTcpConnPool::CTcpConnPool()
    : maxConns(512)
    , maxIdle(100)
    , timeOut(20)
    , OnRequest(NULL)
{}

CTcpConnPool::~CTcpConnPool(){}

CTcpConnPool* CTcpConnPool::Create(CNet* net, ReqCB onReq)
{
    CUNTcpConnPool *pool = new CUNTcpConnPool((CUVNetPlus*)net);
    pool->OnRequest = onReq;
    return pool;
}

CUNTcpConnPool::CUNTcpConnPool(CUVNetPlus* net)
    : m_pNet(net)
    , m_nBusyCount(0)
    , m_nIdleCount(0)
{
    uv_mutex_init(&m_ReqMtx);
    m_pNet->AddEvent(ASYNC_EVENT_TCPCONN_INIT, this);
}

CUNTcpConnPool::~CUNTcpConnPool()
{
    uv_mutex_destroy(&m_ReqMtx);
}

void CUNTcpConnPool::syncInit()
{
    m_uvTimer = new uv_timer_t;
    m_uvTimer->data = this;
    uv_timer_init(&m_pNet->pNode->m_uvLoop, m_uvTimer);
    uv_timer_start(m_uvTimer, on_timer_cb, 5000, 5000);
}

void CUNTcpConnPool::syncRequest()
{
    //ȡ����һ������
    CTcpRequest* req=nullptr;
    uv_mutex_lock(&m_ReqMtx);
    while(!m_listReqs.empty()) {
        req = m_listReqs.front();
        m_listReqs.pop_front();

        //����agent
        CTcpPoolAgent* agent = nullptr;
        stringstream ss;
        ss << req->host << ":" << req->port;
        auto agtfind = m_mapAgents.find(ss.str());
        if (agtfind == m_mapAgents.end()) {
            // û���ҵ�agent����Ҫ�½�
            agent = new CTcpPoolAgent(m_pNet, this);
            agent->host = req->host;
            agent->port = req->port;
            agent->localaddr = req->localaddr;
            agent->maxConns = maxConns;
            agent->maxIdle = maxIdle;
            agent->timeOut = timeOut;
            agent->syncHostDns(req->host);
            m_mapAgents.insert(make_pair(ss.str(), agent));
        } else {
            // �ҵ�����agent
            agent = agtfind->second;
        }
        agent->Request(req);
    }
    uv_mutex_unlock(&m_ReqMtx);    
}

void CUNTcpConnPool::syncClose()
{
    uv_timer_stop(m_uvTimer);
    uv_close((uv_handle_t*)m_uvTimer, on_timer_close);
    delete this;
}

void CUNTcpConnPool::Delete()
{
    m_pNet->AddEvent(ASYNC_EVENT_TCPCONN_CLOSE, this);
}

bool CUNTcpConnPool::Request(std::string host, uint32_t port, std::string localaddr
                 , void *usr/*=nullptr*/, bool copy/*=true*/, bool recv/*=true*/)
{
    CTcpRequest *req = new CTcpRequest();
    req->host = host;
    req->port = port;
    req->copy = copy;
    req->recv = recv;
    req->usr = usr;

    return Request(req);
}

bool CUNTcpConnPool::Request(CTcpRequest *req)
{
    req->pool = this;
    uv_mutex_lock(&m_ReqMtx);
    m_listReqs.push_back(req);
    uv_mutex_unlock(&m_ReqMtx);
    m_pNet->AddEvent(ASYNC_EVENT_TCPCONN_REQUEST, this);
    return true;
}


}