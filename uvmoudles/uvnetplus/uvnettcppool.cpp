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
    TcpConnect     *conn = (TcpConnect*)clt->m_pUsr;
    TcpAgent       *agent= conn->m_pAgent;
    CUNTcpRequest  *req  = conn->m_pReq;
    CUNTcpConnPool *pool = agent->m_pTcpConnPool;

    if(error.empty()){
        //���ӷ������ɹ���������������
        //Log::debug("add new connect");
        conn->m_eState = ConnState_snd;
        conn->m_nLastTime = time(NULL);
        conn->m_pTcpClient->Send(req->data, req->len);
        return;
    }

    //���ӷ�����ʧ��
    //Log::error("tcp connect failed");
    //��busy�����Ƴ�
    agent->m_listBusyConns.remove(conn);
    if(pool->m_funOnRequest) {
        pool->m_funOnRequest(req, error);
    }

    //ɾ��������ʵ��
    delete conn;

    pool->m_pNet->AddEvent(ASYNC_EVENT_TCPAGENT_REQUEST, agent);
}

static void OnTcpRecv(CTcpClient* skt, char *data, int len) {
    CUNTcpClient   *clt  = (CUNTcpClient*)skt;
    TcpConnect     *conn = (TcpConnect*)clt->m_pUsr;
    TcpAgent       *agent= conn->m_pAgent;
    CUNTcpRequest  *req  = conn->m_pReq;
    CUNTcpConnPool *pool = agent->m_pTcpConnPool;
    conn->m_eState = ConnState_rcv;
    if(pool->m_funOnResponse) {
        pool->m_funOnResponse(req, "", data, len);
    }
}

static void OnTcpDrain(CTcpClient* skt) {
    CUNTcpClient   *clt  = (CUNTcpClient*)skt;
    TcpConnect     *conn = (TcpConnect*)clt->m_pUsr;
    TcpAgent       *agent= conn->m_pAgent;
    CUNTcpRequest  *req  = conn->m_pReq;
    CUNTcpConnPool *pool = agent->m_pTcpConnPool;
    conn->m_eState = ConnState_sndok;
    if(pool->m_funOnRequest) {
        // ���ͽ����ص�������ÿ��ֻ��tcpclient�����һ������
        pool->m_funOnRequest(req,"");
    }
    if(req->copy){
        SAFE_FREE(req->data);
    }
}

// �����ر��ղ����رջص������ﶼ��Զ�˹ر�
static void OnTcpClose(CTcpClient* skt) {
    CUNTcpClient   *clt  = (CUNTcpClient*)skt;
    TcpConnect     *conn = (TcpConnect*)clt->m_pUsr;
    TcpAgent       *agent= conn->m_pAgent;
    CUNTcpRequest  *req  = conn->m_pReq;
    CUNTcpConnPool *pool = agent->m_pTcpConnPool;
    if (conn->m_eState == ConnState_Idle) {
        // �������ӶϿ����ӿ��ж����Ƴ�
        delete conn;
        agent->m_listIdleConns.remove(conn);
    } else if(conn->m_eState == ConnState_snd){
        // ����û�гɹ�
        if(pool->m_funOnRequest) {
            pool->m_funOnRequest(req, "Զ�˶Ͽ�");
        }
        delete conn;
        agent->m_listBusyConns.remove(conn);
        delete req;
    } else if(conn->m_eState == ConnState_sndok) {
        // �Ѿ����ͳɹ���û��Ӧ��
        if(req->recv){
            //��ҪӦ��û�гɹ�
            if(pool->m_funOnResponse) {
                pool->m_funOnResponse(req, "δ�յ�Ӧ��͹ر�", nullptr, 0);
            }
        } else {
            //����ҪӦ��û��ȫ�����ͣ���Ҫ�û�ȷ��ȫ���������
            if(pool->m_funOnResponse) {
                pool->m_funOnResponse(req, "û��ȫ��������ɾ͹ر�", nullptr, 0);
            }
        }
        delete conn;
        agent->m_listBusyConns.remove(conn);
        delete req;
    } else if(conn->m_eState == ConnState_rcv) {
        // �յ�Ӧ�𣬵��û�û��ȷ�Ͻ������
        if(pool->m_funOnResponse) {
            pool->m_funOnResponse(req, "û��ȫ��������ɾ͹ر�", nullptr, 0);
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
    m_pTcpClient = new CUNTcpClient(request->pool->m_pNet, NULL, this, false);
    m_pTcpClient->m_funOnConnect = OnTcpConnect;
    m_pTcpClient->m_funOnRecv    = OnTcpRecv;
    m_pTcpClient->m_funOnDrain   = OnTcpDrain;
    m_pTcpClient->m_funOnCLose   = OnTcpClose;

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
    }
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
        conn->m_pTcpClient->Send(req->data, req->len);

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

CUNTcpRequest::CUNTcpRequest()
{

}

CUNTcpRequest::~CUNTcpRequest()
{
    if(copy){
        SAFE_FREE(data);
    }
}

void CUNTcpRequest::Request(const char* buff, int length)
{
    if(copy) {
        SAFE_FREE(data);
        data = (char*)malloc(length);
        memcpy(data, buff, length);
    } else {
        data = (char*)buff;
    }
    len = length;

    conn->m_pTcpClient->Send(data, len);
}

void CUNTcpRequest::Finish()
{
    TcpAgent       *agent= conn->m_pAgent;
    CUNTcpConnPool *pool = agent->m_pTcpConnPool;
    conn->m_eState = ConnState_Idle;
    conn->m_pReq = nullptr;
    agent->m_listBusyConns.remove(conn);
    if(agent->m_listIdleConns.size() < pool->m_nMaxIdle){
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

CUNTcpConnPool::CUNTcpConnPool(CUVNetPlus* net)
    : m_pNet(net)
    , m_nMaxConns(512)
    , m_nMaxIdle(100)
    , m_nBusyCount(0)
    , m_nIdleCount(0)
    , m_nTimeOut(20)
    , m_funOnRequest(NULL)
    , m_funOnResponse(NULL)
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
            agent->m_nMaxConns = m_nMaxConns;
            agent->m_nMaxIdle = m_nMaxIdle;
            agent->m_nTimeOut = m_nTimeOut;
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

TcpRequest* CUNTcpConnPool::Request(std::string host, uint32_t port, const char* data, int len, void *usr, bool copy, bool recv)
{
    CUNTcpRequest *req = new CUNTcpRequest();
    req->host = host;
    req->port = port;
    if(copy) {
        req->data = (char*)malloc(len);
        memcpy(req->data, data, len);
    } else {
        req->data = (char*)data;
    }
    req->len = len;
    req->usr = usr;
    req->copy = copy;
    req->recv = recv;
    req->pool = this;
    req->conn = nullptr;

    uv_mutex_lock(&m_ReqMtx);
    m_listReqs.push_back(req);
    uv_mutex_unlock(&m_ReqMtx);
    m_pNet->AddEvent(ASYNC_EVENT_TCPCONN_REQUEST, this);

    return req;
}

CTcpConnPool* CTcpConnPool::Create(CNet* net, fnOnConnectRequest onReq, fnOnConnectResponse onRes)
{
    CUNTcpConnPool *pool = new CUNTcpConnPool((CUVNetPlus*)net);
    pool->m_funOnRequest = onReq;
    pool->m_funOnResponse = onRes;
    return pool;
}

}