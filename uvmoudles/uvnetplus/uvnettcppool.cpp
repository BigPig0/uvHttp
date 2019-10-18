#include "uvnetplus.h"
#include "uvnettcppool.h"
#include "utilc.h"
#include "util.h"
#include <time.h>
#include <sstream>

namespace uvNetPlus {

//////////////////////////////////////////////////////////////////////////
//////////////   ���ӵ��������ĵ�������    ///////////////////////////////

static void OnTcpConnect(CTcpClient* skt, std::string error) {
    CUNTcpClient   *clt  = (CUNTcpClient*)skt;
    TcpConnect     *conn = (TcpConnect*)clt->userData;
    TcpAgent       *agent= conn->m_pAgent;
    CUNTcpRequest  *req  = conn->m_pReq;
    CUNTcpConnPool *pool = agent->m_pTcpConnPool;

    if(error.empty()){
        //���ӷ������ɹ���������������
        //Log::debug("add new connect");
        conn->m_eState = ConnState_snd;
        conn->m_nLastTime = time(NULL);
        for(auto &buf:req->buffs){
            conn->m_pTcpClient->Send(buf.base, buf.len);
            if(req->copy && conn->m_pTcpClient->copy)
                free(buf.base);
        }
        req->buffs.clear();
        return;
    }

    //���ӷ�����ʧ��
    //Log::error("tcp connect failed");
    //��busy�����Ƴ�
    agent->m_listBusyConns.remove(conn);
    if(req->OnRequest) {
        req->OnRequest(req, error);
    } else if(pool->OnRequest) {
        pool->OnRequest(req, error);
    }

    //ɾ��������ʵ��
    delete conn;

    pool->m_pNet->AddEvent(ASYNC_EVENT_TCPAGENT_REQUEST, agent);
}

static void OnTcpRecv(CTcpClient* skt, char *data, int len) {
    CUNTcpClient   *clt  = (CUNTcpClient*)skt;
    TcpConnect     *conn = (TcpConnect*)clt->userData;
    TcpAgent       *agent= conn->m_pAgent;
    CUNTcpRequest  *req  = conn->m_pReq;
    CUNTcpConnPool *pool = agent->m_pTcpConnPool;
    conn->m_eState = ConnState_rcv;
    if(req->OnResponse) {
        req->OnResponse(req, "", data, len);
    } else if(pool->OnResponse) {
        pool->OnResponse(req, "", data, len);
    }
}

static void OnTcpDrain(CTcpClient* skt) {
    CUNTcpClient   *clt  = (CUNTcpClient*)skt;
    TcpConnect     *conn = (TcpConnect*)clt->userData;
    TcpAgent       *agent= conn->m_pAgent;
    CUNTcpRequest  *req  = conn->m_pReq;
    CUNTcpConnPool *pool = agent->m_pTcpConnPool;
    conn->m_eState = ConnState_sndok;
    if(req->OnRequest) {
        req->OnRequest(req, "");
    } else if(pool->OnRequest) {
        // ���ͽ����ص�������ÿ��ֻ��tcpclient�����һ������
        pool->OnRequest(req,"");
    }
}

// �����ر��ղ����رջص������ﶼ��Զ�˹ر�
static void OnTcpClose(CTcpClient* skt) {
    CUNTcpClient   *clt  = (CUNTcpClient*)skt;
    TcpConnect     *conn = (TcpConnect*)clt->userData;
    TcpAgent       *agent= conn->m_pAgent;
    CUNTcpRequest  *req  = conn->m_pReq;
    CUNTcpConnPool *pool = agent->m_pTcpConnPool;
    if (conn->m_eState == ConnState_Idle) {
        // �������ӶϿ����ӿ��ж����Ƴ�
        delete conn;
        agent->m_listIdleConns.remove(conn);
    } else if(conn->m_eState == ConnState_snd){
        // ����û�гɹ�
        if(req->OnRequest) {
            req->OnRequest(req, "remote close");
        } else if(pool->OnRequest) {
            pool->OnRequest(req, "remote close");
        }
        delete conn;
        agent->m_listBusyConns.remove(conn);
        delete req;
    } else if(conn->m_eState == ConnState_sndok) {
        // �Ѿ����ͳɹ���û��Ӧ��
        if(req->recv){
            //��ҪӦ��û�гɹ�
            if(req->OnResponse) {
                req->OnResponse(req, "δ�յ�Ӧ��͹ر�", nullptr, 0);
            } else if(pool->OnResponse) {
                pool->OnResponse(req, "δ�յ�Ӧ��͹ر�", nullptr, 0);
            }
        } else {
            //����ҪӦ��û��ȫ�����ͣ���Ҫ�û�ȷ��ȫ���������
            if(req->OnResponse) {
                req->OnResponse(req, "û��ȫ��������ɾ͹ر�", nullptr, 0);
            } else if(pool->OnResponse) {
                pool->OnResponse(req, "û��ȫ��������ɾ͹ر�", nullptr, 0);
            }
        }
        delete conn;
        agent->m_listBusyConns.remove(conn);
        delete req;
    } else if(conn->m_eState == ConnState_rcv) {
        // �յ�Ӧ�𣬵��û�û��ȷ�Ͻ������
        if(req->OnResponse) {
            req->OnResponse(req, "û��ȫ��������ɾ͹ر�", nullptr, 0);
        } else if(pool->OnResponse) {
            pool->OnResponse(req, "û��ȫ��������ɾ͹ر�", nullptr, 0);
        }
        delete conn;
        agent->m_listBusyConns.remove(conn);
        delete req;
    }
}

static void OnTcpEnd(CTcpClient* skt) {

}

static void OnTcpError(CTcpClient* skt, std::string error) {

}

TcpConnect::TcpConnect(TcpAgent *agt, CUNTcpRequest *request, string hostip)
    : m_pReq(request)
    , m_pAgent(agt)
    , m_strIP(hostip)
    , m_nPort(request->port)
    , m_eState(ConnState_Idle)
{
    m_nLastTime = time(NULL);
    m_pTcpClient = new CUNTcpClient(request->pool->m_pNet, false);
    m_pTcpClient->userData  = this;
    m_pTcpClient->OnConnect = OnTcpConnect;
    m_pTcpClient->OnRecv    = OnTcpRecv;
    m_pTcpClient->OnDrain   = OnTcpDrain;
    m_pTcpClient->OnCLose   = OnTcpClose;

    m_pTcpClient->m_strRemoteIP = hostip;
    m_pTcpClient->m_nRemotePort = request->port;

    request->conn = this;
    m_pAgent->m_listBusyConns.push_back(this);

    m_pTcpClient->syncConnect();
}

TcpConnect::~TcpConnect()
{
    //Log::debug("~TcpConnect()");
    m_pTcpClient->Delete();
}

//////////////////////////////////////////////////////////////////////////
/**  */

static void on_uv_getaddrinfo(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
    TcpAgent *agent = (TcpAgent*)req->data;
    free(req);
    agent->HostDns(status, res);
    uv_freeaddrinfo(res);
}

TcpAgent::TcpAgent(CUNTcpConnPool *p)
    : m_pNet(p->m_pNet)
    , m_pTcpConnPool(p)
{
}

TcpAgent::~TcpAgent() {
    //Log::debug("~TcpAgent()");
}

void TcpAgent::HostInfo(string host){
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

void TcpAgent::HostDns(int status, struct addrinfo* res) {
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

    Request(NULL);
}

void TcpAgent::Request(CUNTcpRequest *req) {
    //�ⲿ�����ȷŵ������б�
    if(req) {
        m_listReqs.push_back(req);
    }

    //host������δ�ɹ�, û����Ҫ���������
    if(m_listIP.empty() || m_listReqs.empty()){
        return;
    }

    // ȡ����һ������
    req = m_listReqs.front();
    m_listReqs.pop_front();
    //Log::debug("busyConn %d idleConn %d", m_listBusyConns.size(), m_listIdleConns.size());
    //���ҿ�������
    if(!m_listIdleConns.empty()){
        TcpConnect *conn = m_listIdleConns.front();
        m_listIdleConns.pop_front();

        // ʹ�����е����ӽ��з�������
        //Log::debug("use a idle connect send");
        conn->m_eState = ConnState_snd;
        conn->m_nLastTime = time(NULL);
        m_listBusyConns.push_back(conn);
        conn->m_pReq = req;
        req->conn = conn;
        for(auto &buf:req->buffs){
            conn->m_pTcpClient->Send(buf.base, buf.len);
            if(req->copy && conn->m_pTcpClient->copy)
                free(buf.base);
        }
        req->buffs.clear();

        //������һ������
        m_pNet->AddEvent(ASYNC_EVENT_TCPAGENT_REQUEST, this);
    } else {
        //û�п�������
        //Log::debug("busy %d idle %d max %d", m_listBusyConns.size(), m_listIdleConns.size(), m_nMaxConns);
        if(m_listBusyConns.size() >= m_nMaxConns){
            //���������ﵽ����
            //Log::debug("busy conn is max");
            //��ʱ���ܽ������ӣ������󷵻��������б�
            m_listReqs.push_front(req);
        } else {
            //��������δ�����ޣ���������l������
            if(m_listBusyConns.size() + m_listIdleConns.size() >= m_nMaxConns
                || m_listIdleConns.size() >= m_nMaxIdle) {
                    //���������ﵽ���� �����������ﵽ���ޣ������Ͽ���������
                    //Log::debug("need discont a idle connect");
                    TcpConnect* tmp = m_listIdleConns.back();
                    m_listIdleConns.pop_back();
                    delete tmp;
            } 

            //Log::debug("create a new connect and send");
            // �������ӣ��������ӳɹ���������
            string ip = m_listIP.front();
            m_listIP.pop_front();
            m_listIP.push_back(ip);
            TcpConnect *conn = new TcpConnect(this, req, ip);

            //������һ������
            m_pNet->AddEvent(ASYNC_EVENT_TCPAGENT_REQUEST, this);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
/////////////               TCP����             //////////////////////////

CTcpRequest::CTcpRequest()
    : OnRequest(NULL)
    , OnResponse(NULL){}

CTcpRequest::~CTcpRequest(){}

CUNTcpRequest::CUNTcpRequest()
{

}

CUNTcpRequest::~CUNTcpRequest()
{
}

void CUNTcpRequest::Request(const char* buff, int length)
{
    char *data;
    if(copy) {
        data = (char*)malloc(length);
        memcpy(data, buff, length);
    } else {
        data = (char*)buff;
    }

    if(conn && conn->m_pTcpClient)
        conn->m_pTcpClient->Send(data, length);
    else {
        buffs.push_back(uv_buf_init(data, length));  //TcpClient��δ���������͵����ݻ�������
    }
}

void CUNTcpRequest::Finish()
{
    TcpAgent       *agent= conn->m_pAgent;
    CUNTcpConnPool *pool = agent->m_pTcpConnPool;
    conn->m_eState = ConnState_Idle;
    conn->m_pReq = nullptr;
    agent->m_listBusyConns.remove(conn);
    if(agent->m_listIdleConns.size() < pool->maxIdle){
        agent->m_listIdleConns.push_front(conn);
    } else {
        delete conn;
    }
    agent->m_pNet->AddEvent(ASYNC_EVENT_TCPAGENT_REQUEST, agent);
    delete this;
}

//////////////////////////////////////////////////////////////////////////
/** TCP�ͻ������ӳ� */

static void on_timer_cb(uv_timer_t* handle) {
    CUNTcpConnPool *pool = (CUNTcpConnPool*)handle->data;
    time_t now = time(NULL);
    //��������agent
    for(auto it = pool->m_mapAgents.begin(); it != pool->m_mapAgents.end(); ){
        TcpAgent* agent = it->second;
        while(!agent->m_listIdleConns.empty()){
            TcpConnect *conn = agent->m_listIdleConns.back();
            if(difftime(now, conn->m_nLastTime) < agent->m_nTimeOut)
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
    , OnResponse(NULL)
{}

CTcpConnPool::~CTcpConnPool(){}

CTcpConnPool* CTcpConnPool::Create(CNet* net, ReqCB onReq, ResCB onRes)
{
    CUNTcpConnPool *pool = new CUNTcpConnPool((CUVNetPlus*)net);
    pool->OnRequest = onReq;
    pool->OnResponse = onRes;
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
    CUNTcpRequest* req=nullptr;
    uv_mutex_lock(&m_ReqMtx);
    while(!m_listReqs.empty()) {
        req = m_listReqs.front();
        m_listReqs.pop_front();

        //����agent
        TcpAgent* agent = nullptr;
        stringstream ss;
        ss << req->host << ":" << req->port;
        auto agtfind = m_mapAgents.find(ss.str());
        if (agtfind == m_mapAgents.end()) {
            // û���ҵ�agent����Ҫ�½�
            agent = new TcpAgent(this);
            agent->m_strHost = req->host;
            agent->m_nPort = req->port;
            agent->m_nMaxConns = maxConns;
            agent->m_nMaxIdle = maxIdle;
            agent->m_nTimeOut = timeOut;
            agent->HostInfo(req->host);
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

CTcpRequest* CUNTcpConnPool::Request(std::string host, uint32_t port
                                     , string localaddr, void *usr
                                     , bool copy, bool recv
                                     , ReqCB onReq, ResCB onRes)
{
    CUNTcpRequest *req = new CUNTcpRequest();
    req->host = host;
    req->port = port;
    req->usr = usr;
    req->copy = copy;
    req->recv = recv;
    req->pool = this;
    req->conn = nullptr;
    req->OnRequest = onReq;
    req->OnResponse = onRes;

    uv_mutex_lock(&m_ReqMtx);
    m_listReqs.push_back(req);
    uv_mutex_unlock(&m_ReqMtx);
    m_pNet->AddEvent(ASYNC_EVENT_TCPCONN_REQUEST, this);

    return req;
}


}