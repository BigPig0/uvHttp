#include "uvnetplus.h"
#include "uvnettcpconnect.h"
#include "utilc.h"
#include <time.h>

namespace uvNetPlus {

static void OnTcpConnect(CTcpClient* skt, std::string error) {
    if(error.empty())
        return;

    //����ʧ��
    CUNTcpClient   *clt  = (CUNTcpClient*)skt;
    TcpConnect     *conn = (TcpConnect*)clt->m_pUsr;
    CUNTcpRequest  *req  = conn->req;
    CUNTcpConnPool *pool = conn->pool;
    if(pool->m_funOnRequest) {
        pool->m_funOnRequest(req, error);
    }

    delete conn;
    pool->m_pBusyConns.remove(conn);
    delete req;

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
        pool->m_pIdleConns.remove(conn);
    } else if(conn->state == ConnState_snd){
        // ����û�гɹ�
        if(pool->m_funOnRequest) {
            pool->m_funOnRequest(req, "Զ�˶Ͽ�");
        }
        delete conn;
        pool->m_pBusyConns.remove(conn);
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
        pool->m_pBusyConns.remove(conn);
        delete req;
    } else if(conn->state == ConnState_rcv) {
        // �յ�Ӧ�𣬵��û�û��ȷ�Ͻ������
        if(pool->m_funOnResponse) {
            pool->m_funOnResponse(req, "û��ȫ��������ɾ͹ر�", nullptr, 0);
        }
        delete conn;
        pool->m_pBusyConns.remove(conn);
        delete req;
    }
}

static void OnTcpEnd(CTcpClient* skt) {

}

static void OnTcpError(CTcpClient* skt, std::string error) {

}

TcpConnect::TcpConnect(CUNTcpRequest *request)
    : req(request)
    , ip(request->ip)
    , port(request->port)
{
    lastTime = time(NULL);
    client = new CUNTcpClient(request->pool->m_pNet, NULL, this, false);
    client->m_funOnConnect = OnTcpConnect;
    client->m_funOnRecv    = OnTcpRecv;
    client->m_funOnDrain   = OnTcpDrain;
    client->m_funOnCLose   = OnTcpClose;
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
    pool->m_pBusyConns.remove(conn);
    if(pool->m_pIdleConns.size() < pool->m_nIdleCount){
        pool->m_pIdleConns.push_front(conn);
    } else {
        delete conn;
    }
    delete this;
}

//////////////////////////////////////////////////////////////////////////

static void on_timer_cb(uv_timer_t* handle) {
    CUNTcpConnPool *pool = (CUNTcpConnPool*)handle->data;
    time_t now = time(NULL);
    TcpConnect *conn = pool->m_pIdleConns.back();
    while(conn){
        if(difftime(now, conn->lastTime) < pool->m_nTimeOut)
            break;

        delete conn;
        pool->m_pIdleConns.pop_back();
        conn = pool->m_pIdleConns.back();
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
    if(!m_pReqList.empty()) {
        req = m_pReqList.front();
        m_pReqList.pop_front();
    }
    uv_mutex_unlock(&m_ReqMtx);
    if(!req){
        // û�еȴ�������
        return;
    }

    //���ҿ�������
    TcpConnect* conn = nullptr;
    for(auto it=m_pIdleConns.begin(); it!=m_pIdleConns.end(); it++) {
        if((*it)->ip == req->ip && (*it)->port == req->port) {
            conn = (*it);
            m_pIdleConns.erase(it);
            break;
        }
    }
    if(!conn){
        //û�п�������
        if(m_pBusyConns.size() >= m_nMaxConns){
            ;//���������ﵽ����
        } else {
            //��������δ�����ޣ���������l������
            if(m_pBusyConns.size() + m_pIdleConns.size() >= m_nMaxConns
                || m_pIdleConns.size() >= m_nMaxIdle) {
                    //���������ﵽ���� �����������ﵽ���ޣ������Ͽ���������
                    TcpConnect* tmp = m_pIdleConns.back();
                    delete tmp;
                    m_pIdleConns.pop_back();
            } 

            conn = new TcpConnect(req);
        }
    }

    if(conn) {
        conn->state = ConnState_snd;
        conn->client->m_pUsr = req;
        conn->lastTime = time(NULL);
        m_pBusyConns.push_back(conn);
        req->conn = conn;
        conn->client->Send(req->data, req->len);

        m_pNet->AddEvent(ASYNC_EVENT_TCPCONN_RQEUEST, this);
    } else {
        //��ʱ���ܽ������ӣ������󷵻��������б�
        uv_mutex_lock(&m_ReqMtx);
        m_pReqList.push_front(req);
        uv_mutex_unlock(&m_ReqMtx);
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
    m_pReqList.push_back(req);
    uv_mutex_unlock(&m_ReqMtx);
    m_pNet->AddEvent(ASYNC_EVENT_TCPCONN_RQEUEST, this);

    return req;
}

CTcpConnPool* CTcpConnPool::Create(CNet* net, fnOnConnectRequest onReq, fnOnConnectResponse onRes)
{
    CUNTcpConnPool *ret = new CUNTcpConnPool((CUVNetPlus*)net);
    ret->m_funOnRequest = onReq;
    ret->m_funOnResponse = onRes;
    return ret;
}

}