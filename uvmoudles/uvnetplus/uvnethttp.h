#pragma once
#include "uvnetpuclic.h"
#include "uvnettcp.h"
#include <string>
#include <stdint.h>

namespace uvNetPlus {
namespace Http {

class ClientRequest;
class IncomingMessage;
class ServerResponse;
class Server;


/** HTTP�ͻ�������,���ڿͻ�����֯���ݲ����� */
class ClientRequest : public CHttpRequest
{
public:
    ClientRequest(CTcpConnPool *pool);
    ~ClientRequest();

    /** ɾ��ʵ�� */
    virtual void Delete();

    /**
     * ��������һ�����ݣ����chunked=true������һ��chunk������
     * ���chunked=false��ʹ�����������η������ݣ������Լ�������ͷ������length
     * @param chunk ��Ҫ���͵�����
     * @param len ���͵����ݳ���
     * @param cb ����д�뻺������
     */
    virtual bool Write(const char* chunk, int len);

    /**
     * ���һ���������������δ���͵Ĳ������䷢�ͣ����chunked=true�����ⷢ�ͽ�����'0\r\n\r\n'
     * ���chunked=false,Э��ͷû�з��ͣ����Զ����length
     */
    virtual bool End();

    /**
     * �൱��Write(data, len, cb);end();
     */
    virtual void End(const char* data, int len);

    /** �����ӳػ�ȡsocket��� */
    void Socket(CTcpSocket *skt);

    /** ������� */
    void ConnectFinish(string err);

    /* �յ������ݴ��� */
    void Receive(const char *data, int len);

    /** ���������� */
    void DoError(string err);

private:
    std::string GetAgentName();
    std::string GetHeadersString();

    /** ����httpͷ���ɹ�����true������httpͷ����false */
    bool ParseHeader();
    /** �������ݣ��Ѿ������������ݻ�鷵��true������false */
    bool ParseContent();

    CTcpConnPool        *connPool;
    CTcpSocket          *tcpSocket;
    IncomingMessage     *incMsg;
    bool                 connected;     //�Ѿ�����
    bool                 connecting;    //��������
    bool                 parseHeader;   //�������н�����httpͷ��Ĭ��false
    list<string>         sendBuffs;     //skt����֮ǰ�����淢�͵�����
    uv_mutex_t           mutex;         //write��end�̰߳�ȫ
    std::string          recvBuff;      //�������ݻ���
};

/** ���������Ӧ�����ݲ����� */
class ServerResponse : public CHttpResponse
{
public:
    ServerResponse();
    ~ServerResponse();

    /**
     * ���һ��β������
     * @param key β�����ݵ�field name�����ֵ�Ѿ���header�е�Trailer�ﶨ����
     * @param value β�����ݵ�field value
     */
    virtual void AddTrailers(std::string key, std::string value);

    /**
     * Sends a HTTP/1.1 100 Continue message������write��end�Ĺ���
     */
    virtual void WriteContinue();

    /**
     * Sends a HTTP/1.1 102 Processing message to the client
     */
    virtual void WriteProcessing();

    /**
     * ��ʾ��дhttpͷ�����ú���ʽhttpͷ�Ľӿھ���Ч��
     * @param statusCode ��Ӧ״̬��
     * @param statusMessage �Զ���״̬��Ϣ������Ϊ�գ���ʹ�ñ�׼��Ϣ
     * @param headers httpͷ�������ַ�����ÿ�ж�Ҫ����"\r\n"
     */
    virtual void WriteHead(int statusCode, std::string statusMessage, std::string headers);

    /**
     * ��������˴˷�������û�е���writeHead()����ʹ����ʽͷ����������ͷ
     */
    virtual void Write(const char* chunk, int len, ResCb cb = NULL);

    /**
     * ����Ӧ����������ݶ��Ѿ����͡�ÿ��ʵ������Ҫ����һ��end��ִ�к�ᴥ��OnFinish
     */
    virtual void End();

    /**
     * �൱�ڵ���write(data, len, cb) ; end()
     */
    virtual void End(const char* data, int len, ResCb cb = NULL);

private:
    std::string GetHeadersString();

    hash_list   m_Trailers;
};

/** ���յ������ݣ�����˽��յ��������ͻ��˽��յ���Ӧ�� */
class IncomingMessage : public CIncomingMessage
{
public:
    IncomingMessage();
    ~IncomingMessage();
};

/** http��������� */
class CSvrConn {
public:
    CSvrConn();
    /** ����httpͷ���ɹ�����true������httpͷ����false */
    bool ParseHeader();
    /** �������ݣ��Ѿ������������ݻ�鷵��true������false */
    bool ParseContent();

    Server          *http;
    CTcpServer      *server;
    CTcpSocket      *client;
    std::string      buff;   //�������ݻ���
    IncomingMessage *inc;    //�������������������
    ServerResponse  *res;    //Ӧ��
    bool             parseHeader;   //�������н�����httpͷ��Ĭ��false��������ɺ�Ҫ����Ϊfalse��
};

/** http���� */
class Server : public CHttpServer
{
public:
    Server(CNet* net);
    ~Server();

    /** �������������� */
    bool Listen(std::string strIP, uint32_t nPort);
    /** �������ر� */
    void Close();
    /** �������Ƿ��ڼ������� */
    bool Listening();

private:
    static void OnListen(CTcpServer* svr, std::string err);
    static void OnTcpConnection(CTcpServer* svr, std::string err, CTcpSocket* client);
    static void OnSvrCltRecv(CTcpSocket* skt, char *data, int len);
    static void OnSvrCltDrain(CTcpSocket* skt);
    static void OnSvrCltClose(CTcpSocket* skt);
    static void OnSvrCltEnd(CTcpSocket* skt);
    static void OnSvrCltError(CTcpSocket* skt, string err);

private:
    int           m_nPort;      //��������˿�
    CTcpServer   *m_pTcpSvr;    //tcp��������

    std::unordered_multimap<std::string,CSvrConn*> m_pConns;   //�������ӵĿͻ�������
};

};

}