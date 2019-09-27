#include "uvnetplus.h"
#include "uvnettcpconnect.h"
#include "utilc.h"
#include "util.h"
#include <time.h>

namespace uvNetPlus {

static void OnTcpConnect(CTcpClient* skt, std::string error) {
    CUNTcpClient   *clt  = (CUNTcpClient*)skt;
    TcpConnect     *conn = (TcpConnect*)clt->m_pUsr;
    CUNTcpRequest  *req  = conn->req;
    CUNTcpConnPool *pool = conn->pool;

    if(error.empty()){
        //���ӷ������ɹ���������������
        //Log::debug("add new connect");
        conn->state = ConnState_snd;
        conn->lastTime = time(NULL);
        conn->client->Send(req->data, req->len);
        return;
    }

    //���ӷ�����ʧ��
    //Log::error("tcp connect failed");
    //��busy�����Ƴ�
    pool->m_listBusyConns.remove(conn);
    if(pool->m_funOnRequest) {
        pool->m_funOnRequest(req, error);
    }

    //ɾ��������ʵ��
    delete conn;

    pool->m_pNet->AddEvent(ASYNC_EVENT_TCPCONN_RQEUEST, pool);
}

static void OnTcpRecv(CTcpClient* skt, char *data, int len) {
    CUNTcpClient   *clt  = (CUNTcpClient*)skt;
    TcpConnect     *conn = (TcpConnect*)clt->m_pUsr;
    CUNTcpRequest  *req  = conn->req;
    CUNTcpConnPool *pool = conn->pool;
    conn->state = ConnState_rcv;
    if(pool->m_funOnResponse) {
        pool->m_funOnResponse(req, "", data, len);
    }
}

static void OnTcpDrain(CTcpClient* skt) {
    CUNTcpClient   *clt  = (CUNTcpClient*)skt;
    TcpConnect     *conn = (TcpConnect*)clt->m_pUsr;
    CUNTcpRequest  *req  = conn->req;
    CUNTcpConnPool *pool = conn->pool;
    conn->state = ConnState_sndok;
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
    CUNTcpRequest  *req  = conn->req;
    CUNTcpConnPool *pool = conn->pool;
    if (conn->state == ConnState_Idle) {
        // �������ӶϿ����ӿ��ж����Ƴ�
        delete conn;
        pool->m_listIdleConns.remove(conn);
    } else if(conn->state == ConnState_snd){
        // ����û�гɹ�
        if(pool->m_funOnRequest) {
            pool->m_funOnRequest(req, "Զ�˶Ͽ�");
        }
        delete conn;
        pool->m_listBusyConns.remove(conn);
        delete req;
    } else if(conn->state == ConnState_sndok) {
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
        pool->m_listBusyConns.remove(conn);
        delete req;
    } else if(conn->state == ConnState_rcv) {
        // �յ�Ӧ�𣬵��û�û��ȷ�Ͻ������
        if(pool->m_funOnResponse) {
            pool->m_funOnResponse(req, "û��ȫ��������ɾ͹ر�", nullptr, 0);
        }
        delete conn;
        pool->m_listBusyConns.remove(conn);
        delete req;
    }
}

static void OnTcpEnd(CTcpClient* skt) {

}

static void OnTcpError(CTcpClient* skt, std::string error) {

}

TcpConnect::TcpConnect(CUNTcpConnPool *p, CUNTcpRequest *request)
    : req(request)
    , pool(p)
    , ip(request->ip)
    , port(request->port)
    , state(ConnState_Idle)
{
    lastTime = time(NULL);
    client = new CUNTcpClient(request->pool->m_pNet, NULL, this, false);
    client->m_funOnConnect = OnTcpConnect;
    client->m_funOnRecv    = OnTcpRecv;
    client->m_funOnDrain   = OnTcpDrain;
    client->m_funOnCLose   = OnTcpClose;

    client->m_strRemoteIP = request->ip;
    client->m_nRemotePort = request->port;

    request->conn = this;
    pool->m_listBusyConns.push_back(this);

    client->syncConnect();
}

TcpConnect::~TcpConnect()
{
    client->Delete();
}

//////////////////////////////////////////////////////////////////////////

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

    conn->client->Send(data, len);
}

void CUNTcpRequest::Finish()
{
    CUNTcpConnPool *pool = conn->pool;
    conn->state = ConnState_Idle;
    conn->req = nullptr;
    pool->m_listBusyConns.remove(conn);
    if(pool->m_listIdleConns.size() < pool->m_nMaxIdle){
        pool->m_listIdleConns.push_front(conn);
    } else {
        delete conn;
    }
    pool->m_pNet->AddEvent(ASYNC_EVENT_TCPCONN_RQEUEST, pool);
    delete this;
}

//////////////////////////////////////////////////////////////////////////

static void on_timer_cb(uv_timer_t* handle) {
    CUNTcpConnPool *pool = (CUNTcpConnPool*)handle->data;
    time_t now = time(NULL);
    while(!pool->m_listIdleConns.empty()){
        TcpConnect *conn = pool->m_listIdleConns.back();
        if(difftime(now, conn->lastTime) < pool->m_nTimeOut)
            break;

        delete conn;
        pool->m_listIdleConns.pop_back();
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
    if(!m_listReqs.empty()) {
        req = m_listReqs.front();
        m_listReqs.pop_front();
    }
    uv_mutex_unlock(&m_ReqMtx);
    if(!req){
        // û�еȴ�������
        return;
    }

    //Log::debug("busyConn %d idleConn %d", m_listBusyConns.size(), m_listIdleConns.size());
    //���ҿ�������
    TcpConnect* conn = nullptr;
    for(auto it=m_listIdleConns.begin(); it!=m_listIdleConns.end(); it++) {
        if((*it)->ip == req->ip && (*it)->port == req->port) {
            //Log::debug("get a idle connect");
            conn = (*it);
            m_listIdleConns.erase(it);
            break;
        }
    }
    if(!conn){
        //û�п�������
        //Log::debug("busy %d idle %d max %d", m_listBusyConns.size(), m_listIdleConns.size(), m_nMaxConns);
        if(m_listBusyConns.size() >= m_nMaxConns){
            //���������ﵽ����
            //Log::debug("busy conn is max");
            //��ʱ���ܽ������ӣ������󷵻��������б�
            uv_mutex_lock(&m_ReqMtx);
            m_listReqs.push_front(req);
            uv_mutex_unlock(&m_ReqMtx);
        } else {
            //��������δ�����ޣ���������l������
            if(m_listBusyConns.size() + m_listIdleConns.size() >= m_nMaxConns
                || m_listIdleConns.size() >= m_nMaxIdle) {
                    //���������ﵽ���� �����������ﵽ���ޣ������Ͽ���������
                    //Log::debug("need discont a idle connect");
                    TcpConnect* tmp = m_listIdleConns.back();
                    delete tmp;
                    m_listIdleConns.pop_back();
            } 

            //Log::debug("create a new connect and send");
            // �������ӣ��������ӳɹ���������
            conn = new TcpConnect(this, req);

            //׼��������һ������
            m_pNet->AddEvent(ASYNC_EVENT_TCPCONN_RQEUEST, this);
        }
    } else {
        // ʹ�����е����ӽ��з�������
        //Log::debug("use a idle connect send");
        conn->state = ConnState_snd;
        conn->lastTime = time(NULL);
        m_listBusyConns.push_back(conn);
        conn->req = req;
        req->conn = conn;
        conn->client->Send(req->data, req->len);

        //׼��������һ������
        m_pNet->AddEvent(ASYNC_EVENT_TCPCONN_RQEUEST, this);
    }
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

TcpRequest* CUNTcpConnPool::Request(std::string ip, uint32_t port, const char* data, int len, void *usr, bool copy, bool recv)
{
    CUNTcpRequest *req = new CUNTcpRequest();
    req->ip = ip;
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
    m_pNet->AddEvent(ASYNC_EVENT_TCPCONN_RQEUEST, this);

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