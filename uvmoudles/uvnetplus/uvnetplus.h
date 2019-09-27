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
public:
    typedef void (*fnOnTcpEvent)(CTcpClient* skt);
    typedef void (*fnOnTcpRecv)(CTcpClient* skt, char *data, int len);
    typedef void (*fnOnTcpError)(CTcpClient* skt, std::string error);

    static CTcpClient* Create(CNet* net, fnOnTcpEvent onReady, void *usr=nullptr, bool copy=true);
    virtual void Delete() = 0;
    virtual void Connect(std::string strIP, uint32_t nPort, fnOnTcpError onConnect) = 0;
    virtual void SetLocal(std::string strIP, uint32_t nPort) = 0; 
    virtual void HandleRecv(fnOnTcpRecv onRecv) = 0;
    virtual void HandleDrain(fnOnTcpEvent onDrain) = 0;
    virtual void HandleClose(fnOnTcpEvent onClose) = 0;
    virtual void HandleEnd(fnOnTcpEvent onEnd) = 0;
    virtual void HandleTimeOut(fnOnTcpEvent onTimeOut) = 0;
    virtual void HandleError(fnOnTcpError onError) = 0;
    virtual void Send(char *pData, uint32_t nLen) = 0;
    virtual void SetUserData(void* usr) = 0;
    virtual void* UserData() = 0;
protected:
    CTcpClient(){};
    virtual ~CTcpClient(){};
};

/** TCP����� */
class CTcpServer
{
public:
    typedef void (*fnOnTcpSvr)(CTcpServer* svr, std::string err);
    typedef void (*fnOnTcpConnection)(CTcpServer* svr, std::string err, CTcpClient* client);

    static CTcpServer* Create(CNet* net, fnOnTcpConnection onConnection, void *usr=nullptr);
    virtual void Delete() = 0;
    virtual void Listen(std::string strIP, uint32_t nPort, fnOnTcpSvr onListion) = 0;
    virtual void HandleClose(fnOnTcpSvr onClose) = 0;
    virtual void HandleError(fnOnTcpSvr onError) = 0;
    virtual void* UserData() = 0;
protected:
    CTcpServer(){};
    virtual ~CTcpServer(){};
};

//////////////////////////////////////////////////////////////////////////

/** TCP���ӳ� ����ṹ */
class TcpRequest {
public:
    std::string     host;
    uint32_t        port;
    char           *data;
    int             len;
    void           *usr;
    bool            copy;
    bool            recv;

    /* ������׷�ӷ�������,�ڷ��ͻص���ʹ��,�������߳� */
    virtual void Request(const char* buff, int length) = 0;

    /**
     * recvΪtrueʱ��������ɣ�ֻ���ڽ��ջص���ʹ�ã��������
     * recvΪfalseʱ��������ɣ�ֻ���ڷ��ͻص���ʹ�ã��������
     */
    virtual void Finish() = 0;

protected:
    TcpRequest(){};
    virtual ~TcpRequest(){};
};

/** TCP���ӳأ���������Ӧ�� */
class CTcpConnPool
{
public:
    /**
     * ���巢������ص�
     * @param usr �û��󶨵�����
     * @param error ������Ϣ��Ϊ��ʱ�ɹ�
     */
    typedef void (*fnOnConnectRequest)(TcpRequest* req, std::string error);

    /**
     * �����յ�Ӧ��Ļص�
     * @param usr �û��󶨵�����
     * @param error ������Ϣ��Ϊ��ʱ�ɹ�
     * @param data �յ�������,�û���Ҫ�����ߣ��������ݻ�ı�
     * @param len �յ������ݵĳ���
     */
    typedef void (*fnOnConnectResponse)(TcpRequest* req, std::string error, const char *data, int len);

    /**
     * �������ӳ�
     * @param net loopʵ��
     * @param onReq �����������ص�
     * @param onRes Ӧ��ص�
     */
    static CTcpConnPool* Create(CNet* net, fnOnConnectRequest onReq, fnOnConnectResponse onRes);

    virtual void Delete() = 0;

    /**
     * ����һ������
     * @param ip ����Ŀ��ip
     * @param port ����Ŀ��˿�
     * @param data ��Ҫ���͵�����
     * @param len ��Ҫ���͵����ݳ���
     * @param usr ��һ���û����ݣ��ص�ʱ��Ϊ�������
     * @param copy ���͵������Ƿ񿽱����ڲ�
     * @param recv �Ƿ���Ҫ����Ӧ��
     * @return �����µ�����ʵ��
     */
    virtual TcpRequest* Request(std::string ip, uint32_t port, const char* data, int len, void *usr=nullptr, bool copy=true, bool recv=true) = 0;

    virtual void MaxConns(uint32_t num) = 0;
    virtual void MaxIdle(uint32_t num) = 0;
protected:
    CTcpConnPool(){};
    virtual ~CTcpConnPool(){};
};

}

