/*!
 * \file uvnettcppool.h
 * \date 2019/09/05 16:11
 *
 * \author wlla
 * Contact: user@company.com
 *
 * \brief 
 *
 * TODO: ʵ��CTcpConnect�ӿڡ�
 * ��������ָ����ַ���͡��������ݡ��ڲ�ά�����ӳأ��ⲿֻ�ɼ������Ӧ��
 * ��������ֻ���Ͳ�Ӧ�𡢷���Ӧ����Ӧ��ʱ�ⲿ�ж�Ӧ�����
 *
 * \note
*/

#pragma once
#include "uvnetplus.h"
#include "uvnettcp.h"
#include <list>
#include <unordered_map>
using namespace std;

namespace uvNetPlus {

class TcpAgent;
class CUNTcpConnPool;
class CUNTcpRequest;

enum ConnState {
    ConnState_Init = 0, //�´���������
    ConnState_Idle,     //���У��ڿ����б���
    ConnState_snd,      //��������
    ConnState_sndok,    //�������
    ConnState_rcv,      //�յ�Ӧ��
};

/**
 * tcp�ͻ���������
 */
class TcpConnect {
public:
    TcpConnect(TcpAgent *agt, CUNTcpRequest *request, string hostip);
    ~TcpConnect();

    TcpAgent       *m_pAgent;     //�������ڵ�agent
    CUNTcpClient   *m_pTcpClient; //�ͻ�������ʵ��
    CUNTcpRequest  *m_pReq;       //��ǰִ�е�����
    string          m_strIP;      //������ip ����ط��ѽ�host������ip��
    uint32_t        m_nPort;      //�������˿�
    time_t          m_nLastTime;  //���ͨѶʱ��
    ConnState       m_eState;     //����״̬
};

/**
 * tcp�ͻ���agent�࣬����һ��ͬһ����ַ�������б�
 * ���е����Ӷ���ͬ����host�Ͷ˿ڣ�һ��host�����ǲ�ͬ��ip
 */
class TcpAgent {
public:
    TcpAgent(CUNTcpConnPool *p);
    ~TcpAgent();

    void HostInfo(string host);
    void Request(CUNTcpRequest *req);

public:
    CUVNetPlus         *m_pNet;         //�¼��߳̾��
    uint32_t            m_nMaxConns;    //��������� Ĭ��512(busy+idle)
    uint32_t            m_nMaxIdle;     //������������ Ĭ��100
    uint32_t            m_nTimeOut;     //�������ӳ�ʱʱ�� �� Ĭ��20s  0��������ʱ

    CUNTcpConnPool     *m_pTcpConnPool;      //�������ڵ����ӳ�
    string              m_strHost;           //������host��ip
    uint32_t            m_nPort;             //�������˿�
    list<string>        m_listIP;            //����host�õ���ip��ַ������ʹ�ã������ͨ���Ƴ�

    list<TcpConnect*>    m_listBusyConns;    //����ʹ���е�����
    list<TcpConnect*>    m_listIdleConns;    //�������� frontʱ��Ͻ� backʱ��Ͼ�
    list<CUNTcpRequest*> m_listReqs;         //�����б�
};

/**
 * TCP�����࣬�̳��Ե����ӿ�
 */
class CUNTcpRequest : public CTcpRequest {
public:
    CUNTcpRequest();
    ~CUNTcpRequest();

    virtual void Request(const char* buff, int length);
    virtual void Finish();

    string getAgentName();

    TcpAgent       *agent;   //�������ڵ�agent
    CUNTcpConnPool *pool;    //�������ڵ����ӳ�
    TcpConnect     *conn;    //������ʹ�õ�����

    list<uv_buf_t>  buffs;   //TcpClient����֮ǰҪ���͵����ݽ��л���
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

    virtual CTcpRequest* Request(string host, uint32_t port, string localaddr, void *usr=nullptr, bool copy=true, bool recv=true);

public:
    CUVNetPlus         *m_pNet;         //�¼��߳̾��
    uint32_t            m_nBusyCount;   //��ǰʹ���е�������
    uint32_t            m_nIdleCount;   //��ǰ���е�������

    list<CUNTcpRequest*> m_listReqs;      //�����б�,�ݴ��ⲿ������
    uv_mutex_t           m_ReqMtx;        //�����б���

    uv_timer_t         *m_uvTimer;     //��ʱ�������жϿ��������Ƿ�ʱ

    unordered_map<string, TcpAgent*>   m_mapAgents;
};
}