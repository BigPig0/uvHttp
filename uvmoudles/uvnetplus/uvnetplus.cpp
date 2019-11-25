#include "uvnetplus.h"
#include "uvnetprivate.h"
#include "uvnettcp.h"
#include "uvnettcppool.h"
#include "Log.h"

namespace uvNetPlus {

static void on_uv_async(uv_async_t* handle) {
    UV_NODE* h = (UV_NODE*)handle->data;
    uv_mutex_lock(&h->m_uvMtxAsEvts);
    for(auto e : h->m_listAsyncEvents) {
        if(e.event == ASYNC_EVENT_TCP_CONNECT) {
            CUNTcpSocket *tcp = (CUNTcpSocket*)e.param;
            tcp->syncConnect();
        } else if(e.event == ASYNC_EVENT_TCP_SEND) {
            CUNTcpSocket *tcp = (CUNTcpSocket*)e.param;
            tcp->syncSend();
        } else if(e.event == ASYNC_EVENT_TCP_LISTEN) {
            CUNTcpServer *tcp = (CUNTcpServer*)e.param;
            tcp->syncListen();
        } else if(e.event == ASYNC_EVENT_TCP_CLTCLOSE) {
            CUNTcpSocket *tcp = (CUNTcpSocket*)e.param;
            tcp->syncClose();
        } else if(e.event == ASYNC_EVENT_TCP_SVRCLOSE) {
            CUNTcpServer *tcp = (CUNTcpServer*)e.param;
            tcp->syncClose();
        } else if(e.event == ASYNC_EVENT_TCPCONN_INIT) {
            CUNTcpConnPool *pool = (CUNTcpConnPool*)e.param;
            pool->syncInit();
        } else if(e.event == ASYNC_EVENT_TCPCONN_REQUEST) {
            CUNTcpConnPool *pool = (CUNTcpConnPool*)e.param;
            pool->syncRequest();
        } else if(e.event == ASYNC_EVENT_TCP_CONNCLOSE) {
            CUNTcpPoolSocket *pool = (CUNTcpPoolSocket*)e.param;
            pool->syncClose();
        }else if(e.event == ASYNC_EVENT_TCPAGENT_REQUEST) {
            CTcpPoolAgent *pool = (CTcpPoolAgent*)e.param;
            pool->Request(NULL);
        } else if(e.event == ASYNC_EVENT_TCPCONN_CLOSE) {
            CUNTcpConnPool *pool = (CUNTcpConnPool*)e.param;
            pool->syncClose();
        } 
    }
    h->m_listAsyncEvents.clear();
    uv_mutex_unlock(&h->m_uvMtxAsEvts);
}

static void run_loop_thread(void* arg)
{
    UV_NODE* h = (UV_NODE*)arg;
    while (h->m_bRun) {
        uv_run(&h->m_uvLoop, UV_RUN_DEFAULT);
        Sleep(100);
    }

    uv_loop_close(&h->m_uvLoop);
    delete h;
}

CUVNetPlus::CUVNetPlus()
{
    pNode = new UV_NODE;
    uv_loop_init(&pNode->m_uvLoop);
    pNode->m_bRun = true;
    pNode->m_uvAsync.data = pNode;
    uv_async_init(&pNode->m_uvLoop, &pNode->m_uvAsync, on_uv_async);
    uv_mutex_init(&pNode->m_uvMtxAsEvts);
    uv_thread_t tid;
    uv_thread_create(&tid, run_loop_thread, pNode);
}

CUVNetPlus::~CUVNetPlus()
{
    pNode->m_bRun = false;
}

void CUVNetPlus::AddEvent(UV_ASYNC_EVENT e, void* param) {
    UV_EVET ue = {e, param};
    uv_mutex_lock(&pNode->m_uvMtxAsEvts);
    pNode->m_listAsyncEvents.push_back(ue);
    uv_mutex_unlock(&pNode->m_uvMtxAsEvts);
    uv_async_send(&pNode->m_uvAsync);
}

CNet* CNet::Create() {
    return new CUVNetPlus();
}
}