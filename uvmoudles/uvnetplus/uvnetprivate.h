#pragma once
#include "uv.h"
#include <list>

using namespace std;

namespace uvNetPlus {

enum UV_ASYNC_EVENT
{
    ASYNC_EVENT_TCP_CLIENT = 0, //�½�һ��tcp�ͻ���
    ASYNC_EVENT_TCP_CONNECT,    //tcp�ͻ�������
    ASYNC_EVENT_TCP_SEND,       //tcp��������
    ASYNC_EVENT_TCP_LISTEN,     //tcp����˼���
    ASYNC_EVENT_TCP_CLTCLOSE,   //tcp�ͻ��˹ر�
    ASYNC_EVENT_TCP_SVRCLOSE,   //tcp����˹ر�
    ASYNC_EVENT_TCPCONN_INIT,   //tcp���ӳس�ʼ����ʱ��
    ASYNC_EVENT_TCPCONN_REQUEST,//tcp���ӳ��л�ȡsocket
    ASYNC_EVENT_TCPCONN_CLOSE,  //tcp���ӳعر�
    ASYNC_EVENT_TCPAGENT_REQUEST, //tcp agent�л�ȡsocket
    ASYNC_EVENT_TCP_CONNCLOSE,  //���ӳ��е�socket����
};

struct UV_EVET {
    UV_ASYNC_EVENT event;
    void* param;
};

struct UV_NODE
{
    bool            m_bRun;
    uv_loop_t       m_uvLoop;
    uv_async_t      m_uvAsync;
    list<UV_EVET>   m_listAsyncEvents;
    uv_mutex_t      m_uvMtxAsEvts;
};


class CUVNetPlus : public CNet
{
public:
    CUVNetPlus();
    ~CUVNetPlus();

    void AddEvent(UV_ASYNC_EVENT e, void* param);

    UV_NODE *pNode;
};

}