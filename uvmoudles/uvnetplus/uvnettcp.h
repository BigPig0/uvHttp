#pragma once
#include "uvnetplus.h"
#include "uvnetprivate.h"

namespace uvNetPlus {

extern bool net_is_ipv4(const char* input);
extern bool net_is_ipv6(const char* input);
extern int  net_is_ip(const char* input);

class CUNTcpServer;
//////////////////////////////////////////////////////////////////////////

class CUNTcpClient : public uvNetPlus::CTcpClient
{
public:
    CUNTcpClient(CUVNetPlus* net, bool copy = true);
    ~CUNTcpClient();
    virtual void Delete();
    virtual void Connect(std::string strIP, uint32_t nPort);
    virtual void SetLocal(std::string strIP, uint32_t nPort);
    virtual void Send(const char *pData, uint32_t nLen);

    void syncInit();
    void syncConnect();
    void syncSend();
    void syncClose();

public:
    CUVNetPlus       *m_pNet;      //�¼��߳̾��
    CUNTcpServer     *m_pSvr;      //�ͻ���ʵ��Ϊnull�������ʵ��ָ�����������
    uv_tcp_t          uvTcp;

    string            m_strRemoteIP; //Զ��ip
    uint32_t          m_nRemotePort; //Զ�˶˿�
    string            m_strLocalIP;  //����ip
    uint32_t          m_nLocalPort;  //���ض˿�
    bool              m_bSetLocal;   //��Ϊ�ͻ���ʱ���Ƿ����ñ��ذ���Ϣ
    bool              m_bInit;       //�Ƿ��ʼ��uv_tcp_t����
    bool              m_bConnect;    //�Ƿ��Ѿ��ɹ����ӷ�����

    char             *readBuff;         // ���ջ���
    uint32_t          bytesRead;        // ͳ���ۼƽ��մ�С
    list<uv_buf_t>    sendList;         // ���ͻ���
    list<uv_buf_t>    sendingList;      // ���ڷ���
    uv_mutex_t        sendMtx;          // ������
};

//////////////////////////////////////////////////////////////////////////

class CUNTcpServer : public uvNetPlus::CTcpServer
{
public:
    CUNTcpServer(CUVNetPlus* net);
    ~CUNTcpServer();
    virtual void Delete();
    virtual bool Listen(std::string strIP, uint32_t nPort);
    virtual bool Listening();
    void syncListen();
    void syncConnection(uv_stream_t* server, int status);
    void syncClose();
    void removeClient(CUNTcpClient* c);

public:
    CUVNetPlus       *m_pNet;
    uv_tcp_t          uvTcp;

    string            m_strLocalIP;
    uint32_t          m_nLocalPort;
    int               m_nBacklog;       //syns queue�Ĵ�С��Ĭ��Ϊ512
    bool              m_bListening;     //��ʼ����ʱtrue�����ڼ���ʱΪfalse
    int               m_nFamily;        //�󶨱���IP���� 4 �� 6


    list<CUNTcpClient*> m_listClients;
};

}