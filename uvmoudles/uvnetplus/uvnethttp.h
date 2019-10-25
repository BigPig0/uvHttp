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

/** httpЭ��ʹ�õ�socket�ķ��Ͳ��� */
class SendingMessage
{
public:
    SendingMessage();
    virtual ~SendingMessage();

    /**
     * ��ʾ��дhttpͷ�����ú���ʽhttpͷ�Ľӿھ���Ч��
     * @param headers httpͷ�������ַ���������ÿһ�н�β��"\r\n"
     */
    void WriteHead(std::string headers);

    /**
     * ��ȡ�Ѿ��趨����ʽͷ
     */
    std::vector<std::string> GetHeader(std::string name);

    /**
     * ��ȡ�����趨����ʽͷ��key
     */
    std::vector<std::string> GetHeaderNames();

    /**
     * ��ʽͷ�Ƿ��Ѿ�����һ������
     */
    bool HasHeader(std::string name);

    /**
     * �Ƴ�һ����ʽͷ
     */
    void RemoveHeader(std::string name);

    /**
     * ��������һ��ͷ��ֵ����������һ����ʽͷ
     * param name field name
     * param value ����field value
     * param values ��NULL��β���ַ������飬���field value
     */
    void SetHeader(std::string name, std::string value);
    void SetHeader(std::string name, char **values);

    /**
     * �������ݳ��ȡ����ݷֶ�η��ͣ��Ҳ�ʹ��chunkedʱʹ�á�
     */
    void SetContentLen(uint32_t len);

    /**
     * �鿴�Ƿ����
     */
    bool Finished();

    CTcpClient*         m_pSocket;     // tcp����
protected:
    /** ����ʽͷ����ַ��� */
    std::string getImHeaderString();

protected:
    std::string         m_strHeaders;   // ��ʽ��ͷ
    hash_list           m_Headers;      // ��ʽͷ������
    bool                m_bHeadersSent; // header�Ƿ��Ѿ�����
    bool                m_bFinished;    // ����Ӧ���Ƿ����
    uint32_t            m_nContentLen;  // �������ݵĳ���
};


/** HTTP�ͻ�������,���ڿͻ�����֯���ݲ����� */
class ClientRequest : public SendingMessage, public CHttpRequest
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
    virtual bool Write(const char* chunk, int len, ReqCb cb = NULL);

    /**
     * ���һ���������������δ���͵Ĳ������䷢�ͣ����chunked=true�����ⷢ�ͽ�����'0\r\n\r\n'
     * ���chunked=false,Э��ͷû�з��ͣ����Զ����length
     */
    virtual bool End();

    /**
     * �൱��Write(data, len, cb);end();
     */
    virtual void End(const char* data, int len, ReqCb cb = NULL);

    virtual void WriteHead(std::string headers);
    virtual std::vector<std::string> GetHeader(std::string name);
    virtual std::vector<std::string> GetHeaderNames();
    virtual bool HasHeader(std::string name);
    virtual void RemoveHeader(std::string name);
    virtual void SetHeader(std::string name, std::string value);
    virtual void SetHeader(std::string name, char **values);
    virtual void SetContentLen(uint32_t len);
    virtual bool Finished();

    /* �յ������ݴ��� */
    void Receive(const char *data, int len);

private:
    std::string GetAgentName();
    std::string GetHeadersString();

    /** ����httpͷ���ɹ�����true������httpͷ����false */
    bool ParseHeader();
    /** �������ݣ��Ѿ������������ݻ�鷵��true������false */
    bool ParseContent();

    CTcpConnPool        *m_pTcpPool;
    CTcpRequest         *m_pTcpReq;
    IncomingMessage     *inc;
    bool                 parseHeader;   //�������н�����httpͷ��Ĭ��false
    std::string          buff;   //�������ݻ���
};

/** ���������Ӧ�����ݲ����� */
class ServerResponse : public SendingMessage, public CHttpResponse
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

    virtual void WriteHead(std::string headers);
    virtual std::vector<std::string> GetHeader(std::string name);
    virtual std::vector<std::string> GetHeaderNames();
    virtual bool HasHeader(std::string name);
    virtual void RemoveHeader(std::string name);
    virtual void SetHeader(std::string name, std::string value);
    virtual void SetHeader(std::string name, char **values);
    virtual void SetContentLen(uint32_t len);
    virtual bool Finished();
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
    CTcpClient      *client;
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
    static void OnTcpConnection(CTcpServer* svr, std::string err, CTcpClient* client);
    static void OnSvrCltRecv(CTcpClient* skt, char *data, int len);
    static void OnSvrCltDrain(CTcpClient* skt);
    static void OnSvrCltClose(CTcpClient* skt);
    static void OnSvrCltEnd(CTcpClient* skt);
    static void OnSvrCltError(CTcpClient* skt, string err);

private:
    int           m_nPort;      //��������˿�
    CTcpServer   *m_pTcpSvr;    //tcp��������

    std::unordered_multimap<std::string,CSvrConn*> m_pConns;   //�������ӵĿͻ�������
};

};

}