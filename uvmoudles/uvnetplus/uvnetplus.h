#pragma once
#include "uvnetpuclic.h"
#include <string>
#include <stdint.h>

namespace uvNetPlus {

/** �¼�ѭ��eventloopִ���̣߳���װuv_loop */
class CNet
{
public:
    static CNet* Create();
    virtual ~CNet(){};
};

//////////////////////////////////////////////////////////////////////////

/** TCP�ͻ��� */
class CTcpClient
{
    typedef void (*EventCB)(CTcpClient* skt);
    typedef void (*RecvCB)(CTcpClient* skt, char *data, int len);
    typedef void (*ErrorCB)(CTcpClient* skt, std::string error);
public:
    EventCB      OnReady;     //socket�������
    ErrorCB      OnConnect;   //�������
    RecvCB       OnRecv;      //�յ�����
    EventCB      OnDrain;     //���Ͷ���ȫ�����
    EventCB      OnCLose;     //socket�ر�
    EventCB      OnEnd;       //�յ��Է�fin,����eof
    EventCB      OnTimeout;   //��ʱ�ص�
    ErrorCB      OnError;     //����ص�

    bool         autoRecv;    //���ӽ������Ƿ������Զ��������ݡ�Ĭ��true
    bool         copy;        //���͵����ݿ�������ʱ����
    void        *userData;    //�û����Զ�������

    /**
     * ����һ��tcp���ӿͻ���ʵ��
     * @param net �������
     * @param usr �趨�û��Զ�������
     * @param copy ���÷��ͽӿ�ʱ���Ƿ����ݿ������������ڲ����й���
     */
    static CTcpClient* Create(CNet* net, void *usr=nullptr, bool copy=true);

    /**
     * �첽ɾ�����ʵ��
     */
    virtual void Delete() = 0;

    /**
     * ���ӷ�������������ɺ����OnConnect�ص�
     */
    virtual void Connect(std::string strIP, uint32_t nPort) = 0;

    /**
     * ����socket�ı��ض˿ڣ������ָ��������ϵͳ�Զ�����
     * @param strIP ����IP������ָ������ʹ����һ���������ձ�ʾ��ָ����
     * @param nPort �����˿ڣ�0��ʾ��ָ��
     */
    virtual void SetLocal(std::string strIP, uint32_t nPort) = 0; 

    /**
     * �������ݡ������ݷŵ����ػ�������
     */
    virtual void Send(const char *pData, uint32_t nLen) = 0;
protected:
    CTcpClient();
    virtual ~CTcpClient() = 0;
};

/** TCP����� */
class CTcpServer
{
    typedef void (*EventCB)(CTcpServer* svr, std::string err);
    typedef void (*ConnCB)(CTcpServer* svr, std::string err, CTcpClient* client);
public:

    EventCB          OnListen;       // ����������ɻص�������ʱ���״�����Ϣ
    ConnCB           OnConnection;   // �����ӻص�
    EventCB          OnClose;        // ����socket�ر���ɻص�
    EventCB          OnError;        // ��������ص�

    void            *userData;

    /**
     * ����һ��tcp�����ʵ��
     * @param net �������
     * @param onConnection ָ���յ�������ʱ�Ļص�
     * @param usr �趨�û��Զ�������
     */
    static CTcpServer* Create(CNet* net, ConnCB onConnection, void *usr=nullptr);

    /**
     * �첽ɾ����ǰʵ��
     */
    virtual void Delete() = 0;

    /**
     * ��������
     * @param strIP ����IP������ָ������ʹ����һ������
     * @param nPort ���ؼ����˿�
     */
    virtual bool Listen(std::string strIP, uint32_t nPort) = 0;

    /** �������Ƿ��ڼ������� */
    virtual bool Listening() = 0;
protected:
    CTcpServer();
    virtual ~CTcpServer() = 0;
};

//////////////////////////////////////////////////////////////////////////

/** TCP���ӳ� ����ṹ */
class CTcpRequest {
public:
    std::string     host;   //����Ŀ��������ip
    uint32_t        port;   //����˿�
    std::string     localaddr; //����ip������ʹ����һ��������Ĭ�Ͽգ�������
    void           *usr;    //�û��Զ�������
    bool            copy;   //��Ҫ���͵������Ƿ񿽱����ڲ�ά��
    bool            recv;   //tcp�����Ƿ���Ҫ��������

    /* ������׷�ӷ�������,�ڷ��ͻص���ʹ��,�������߳� */
    virtual void Request(const char* buff, int length) = 0;

    /**
     * һ��������ɣ���socket�ŵ����г�����ȥ
     */
    virtual void Finish() = 0;

protected:
    CTcpRequest(){};
    virtual ~CTcpRequest(){};
};

/** TCP���ӳأ���������Ӧ�� */
class CTcpConnPool
{
    typedef void (*ReqCB)(CTcpRequest* req, std::string error);
    typedef void (*ResCB)(CTcpRequest* req, std::string error, const char *data, int len);
public:
    uint32_t   maxConns;    //��������� Ĭ��512(busy+idle)
    uint32_t   maxIdle;     //������������ Ĭ��100
    uint32_t   timeOut;     //�������ӳ�ʱʱ�� �� Ĭ��20s 0Ϊ������ʱ

    ReqCB      OnRequest;
    ResCB      OnResponse;

    /**
     * �������ӳ�
     * @param net loopʵ��
     * @param onReq �����������ص�
     * @param onRes Ӧ��ص�
     */
    static CTcpConnPool* Create(CNet* net, ReqCB onReq, ResCB onRes);

    /**
     * �첽ɾ�����ӳ�
     */
    virtual void Delete() = 0;

    /**
     * ����һ������
     * @param host ����Ŀ��������˿�
     * @param port ����Ŀ��˿�
     * @param localaddr ����ip��ָ��������Ϊ�ձ�ʾ��ָ��
     * @param usr ��һ���û����ݣ��ص�ʱ��Ϊ�������
     * @param copy ���͵������Ƿ񿽱����ڲ�
     * @param recv �Ƿ���Ҫ����Ӧ��
     * @return �����µ�����ʵ��
     */
    virtual CTcpRequest* Request(std::string host, uint32_t port, std::string localaddr, void *usr=nullptr, bool copy=true, bool recv=true) = 0;

protected:
    CTcpConnPool();
    virtual ~CTcpConnPool() = 0;
};

//////////////////////////////////////////////////////////////////////////


}

