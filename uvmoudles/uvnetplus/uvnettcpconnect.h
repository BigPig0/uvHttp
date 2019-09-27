/*!
 * \file uvnettcpconnect.h
 * \date 2019/09/05 16:11
 *
 * \author wlla
 * Contact: user@company.com
 *
 * \brief 
 *
 * TODO: ʵ��CTcpConnect�ӿڡ�
 * ��������ָ����ַ���͡��������ݡ��ڲ�ά�����ӳأ��ⲿֻ�ɼ������Ӧ��
 * ��������ֻ���Ͳ�Ӧ�� ����Ӧ����Ӧ��ʱ�ⲿ�ж�Ӧ�����
 *
 * \note
*/

#pragma once
#include "uvnetplus.h"
#include "uvnetprivate.h"
#include "uvnettcp.h"
#include <list>
using namespace std;

namespace uvNetPlus {

class CUNTcpConnPool;
class CUNTcpRequest;

enum ConnState { 
    ConnState_Idle = 0, //���У��ڿ����б���
    ConnState_snd,      //��������
    ConnState_sndok,    //�������
    ConnState_rcv,      //�յ�Ӧ��
};

class TcpConnect {
public:
    TcpConnect(CUNTcpConnPool *p, CUNTcpRequest *request);
    ~TcpConnect();

    CUNTcpConnPool *pool;     //�������ڵ����ӳ�
    CUNTcpClient   *client;   //�ͻ�������ʵ��
    CUNTcpRequest  *req;      //��ǰִ�е�����
    string          ip;       //������ip
    uint32_t        port;     //�������˿�
    time_t          lastTime; //���ͨѶʱ��
    ConnState       state;    //����״̬
};

class CUNTcpRequest : public TcpRequest {
public:
    CUNTcpRequest();
    ~CUNTcpRequest();

    virtual void Request(const char* buff, int length);
    virtual void Finish();

    CUNTcpConnPool *pool;    //�������ڵ����ӳ�
    TcpConnect     *conn;    //������ʹ�õ�����
};

class CUNTcpConnPool : public CTcpConnPool
{
public:
    CUNTcpConnPool(CUVNetPlus* net);
    ~CUNTcpConnPool();

    void syncInit();
    void syncRequest();
    void syncClose();

    virtual void Delete();

    virtual TcpRequest* Request(string ip, uint32_t port,  const char* data, int len, void *usr=nullptr, bool copy=true, bool recv=true);

    virtual void MaxConns(uint32_t num){m_nMaxConns = num;}

    virtual void MaxIdle(uint32_t num){m_nMaxIdle = num;}

    fnOnConnectRequest      m_funOnRequest;
    fnOnConnectResponse     m_funOnResponse;

public:
    CUVNetPlus         *m_pNet;         //�¼��߳̾��
    uint32_t            m_nMaxConns;    //��������� Ĭ��512(busy+idle)
    uint32_t            m_nMaxIdle;     //������������ Ĭ��100
    uint32_t            m_nBusyCount;   //��ǰʹ���е�������
    uint32_t            m_nIdleCount;   //��ǰ���е�������
    uint32_t            m_nTimeOut;     //�������ӳ�ʱʱ�� ��

    list<TcpConnect*>   m_listBusyConns;    //����ʹ���е�����
    list<TcpConnect*>   m_listIdleConns;    //�������� frontʱ��Ͻ� backʱ��Ͼ�
    list<CUNTcpRequest*> m_listReqs;      //�����б�
    uv_mutex_t          m_ReqMtx;        //�����б���

    uv_timer_t         *m_uvTimer;     //��ʱ�������жϿ��������Ƿ�ʱ
};
}