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

class CTcpPoolAgent;
class CUNTcpConnPool;

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
class CUNTcpPoolSocket : public CUNTcpSocket {
public:
    CUNTcpPoolSocket(CUVNetPlus* net, bool copy);
    ~CUNTcpPoolSocket();

    void syncClose();

    virtual void Delete();

    CTcpPoolAgent  *m_pAgent;     //�������ڵ�agent
    CUNTcpConnPool *connPool;     //�������ڵ����ӳ�
    CTcpRequest    *m_pReq;       //��ǰִ�е�����
    time_t          m_nLastTime;  //���ͨѶʱ��
};

/**
 * tcp�ͻ���agent�࣬����һ��ͬһ����ַ�������б�
 * ���е����Ӷ���ͬ����host�Ͷ˿ڣ�һ��host�����ǲ�ͬ��ip
 */
class CTcpPoolAgent {
public:
    CTcpPoolAgent(CUVNetPlus* net, CUNTcpConnPool *p);
    ~CTcpPoolAgent();

    /** ͨ��dns����������Ӧ��ip */
    void syncHostDns(string host);

    /** dns���������ɹ� */
    void OnParseHost(int status, struct addrinfo* res);

    void Delete();

    /** ���������������ȡ���� */
    bool Request(CTcpRequest *req);

    /** ����ʹ����ϵ�socket */
    void GiveBackSkt(CUNTcpPoolSocket *skt);

public:
    string     host;
    uint32_t   port;
    string     localaddr;
    uint32_t   maxConns;    //��������� Ĭ��512(busy+idle)
    uint32_t   maxIdle;     //������������ Ĭ��100
    uint32_t   timeOut;     //�������ӳ�ʱʱ�� �� Ĭ��20s 0Ϊ������ʱ

    CUVNetPlus         *m_pNet;                //�¼��߳̾��
    CUNTcpConnPool     *m_pTcpConnPool;        //���ڵ����ӳ�

    list<string>           m_listIP;           //����host�õ���ip��ַ������ʹ�ã������ͨ���Ƴ�
    list<CUNTcpPoolSocket*>   m_listBusyConns;    //����ʹ���е�����
    list<CUNTcpPoolSocket*>   m_listIdleConns;    //�������� frontʱ��Ͻ� backʱ��Ͼ�
    list<CTcpRequest*>     m_listReqs;         //�����б�
};

/**
 * TCP�ͻ������ӳ�
 */
class CUNTcpConnPool : public CTcpConnPool
{
public:
    CUNTcpConnPool(CUVNetPlus* net);
    ~CUNTcpConnPool();

    void syncInit();
    void syncRequest();
    void syncClose();

    virtual void Delete();

    virtual bool Request(std::string host, uint32_t port, std::string localaddr
        , void *usr=nullptr, bool copy=true, bool recv=true);

    virtual bool Request(CTcpRequest *req);

public:
    CUVNetPlus         *m_pNet;         //�¼��߳̾��
    uint32_t            m_nBusyCount;   //��ǰʹ���е�������
    uint32_t            m_nIdleCount;   //��ǰ���е�������

    list<CTcpRequest*>  m_listReqs;      //�����б�,�ݴ��ⲿ������
    uv_mutex_t          m_ReqMtx;        //�����б���

    uv_timer_t         *m_uvTimer;     //��ʱ�������жϿ��������Ƿ�ʱ

    unordered_map<string, CTcpPoolAgent*>   m_mapAgents;
};
}