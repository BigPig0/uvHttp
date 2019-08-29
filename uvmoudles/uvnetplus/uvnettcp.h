#pragma once
#include "uvnetplus.h"
#include "uvnetprivate.h"

class CUNTcpServer;
//////////////////////////////////////////////////////////////////////////

class CUNTcpClient : public uvNetPlus::CTcpClient
{
public:
    CUNTcpClient(CUVNetPlus* net, fnOnTcpEvent onReady, void *usr);
    ~CUNTcpClient();
    virtual void Delete();
    void syncInit();
    void syncConnect();
    void syncSend();
    void syncClose();
    virtual void Connect(std::string strIP, uint32_t nPort, fnOnTcpError onConnect);
    virtual void SetLocal(std::string strIP, uint32_t nPort);
    virtual void HandleRecv(fnOnTcpRecv onRecv);
    virtual void HandleDrain(fnOnTcpEvent onDrain);
    virtual void HandleClose(fnOnTcpEvent onClose);
    virtual void HandleEnd(fnOnTcpEvent onEnd);
    virtual void HandleTimeOut(fnOnTcpEvent onTimeOut);
    virtual void HandleError(fnOnTcpError onError);
    virtual void Send(char *pData, uint32_t nLen);
    virtual void SetUserData(void* usr){m_pData = usr;};
    virtual void* UserData(){return m_pData;};

public:
    CUVNetPlus       *m_pNet;      //�¼��߳̾��
    CUNTcpServer     *m_pSvr;      //�ͻ���ʵ��Ϊnull�������ʵ��ָ�����������
    uv_tcp_t          uvTcp;
    void             *m_pData;

    string            m_strRemoteIP; //Զ��ip
    uint32_t          m_nRemotePort; //Զ�˶˿�
    string            m_strLocalIP;  //����ip
    uint32_t          m_nLocalPort;  //���ض˿�
    bool              m_bSetLocal;   //��Ϊ�ͻ���ʱ���Ƿ����ñ��ذ���Ϣ
    bool              m_bInit;


    fnOnTcpEvent      m_funOnReady;     //socket�������
    fnOnTcpError      m_funOnConnect;   //�������
    fnOnTcpRecv       m_funOnRecv;      //�յ�����
    fnOnTcpEvent      m_funOnDrain;     //���Ͷ���ȫ�����
    fnOnTcpEvent      m_funOnCLose;     //socket�ر�
    fnOnTcpEvent      m_funOnEnd;       //�յ��Է�fin,����eof
    fnOnTcpEvent      m_funOnTimeout;   //��ʱ�ص�
    fnOnTcpError      m_funOnError;     //����ص�

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
    CUNTcpServer(CUVNetPlus* net, fnOnTcpConnection onConnection, void *usr);
    ~CUNTcpServer();
    virtual void Delete();
    void syncListen();
    void syncConnection(uv_stream_t* server, int status);
    void syncClose();
    virtual void Listen(std::string strIP, uint32_t nPort, fnOnTcpSvr onListion);
    virtual void HandleClose(fnOnTcpSvr onClose);
    virtual void HandleError(fnOnTcpSvr onError);
    virtual void* UserData(){return m_pData;};
    void removeClient(CUNTcpClient* c);

public:
    CUVNetPlus       *m_pNet;
    uv_tcp_t          uvTcp;
    void             *m_pData;

    string            m_strLocalIP;
    uint32_t          m_nLocalPort;
    int               m_nBacklog;       //syns queue�Ĵ�С��Ĭ��Ϊ512
    bool              m_bInit;          //��ʼ����ʱtrue�����ڼ���ʱΪfalse

    fnOnTcpSvr          m_funOnListen;
    fnOnTcpConnection   m_funOnConnection;
    fnOnTcpSvr          m_funOnClose;
    fnOnTcpSvr          m_funOnError;

    list<CUNTcpClient*> m_listClients;
};

