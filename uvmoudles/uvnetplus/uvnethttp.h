#pragma once
#include "uvnetpuclic.h"
#include "uvnettcp.h"
#include <string>
#include <stdint.h>
#include <list>
#include <unordered_map>

namespace uvNetPlus {
namespace Http {

typedef std::unordered_multimap<std::string, std::string> hash_list;

enum METHOD {
    OPTIONS = 0,
    HEAD,
    GET,
    POST,
    PUT,
    DEL,
    TRACE,
    CONNECT
};

enum VERSION {
    HTTP1_0 = 0,
    HTTP1_1,
    HTTP2,
    HTTP3
};

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
     * �鿴�Ƿ����
     */
    bool Finished();

protected:
    /** ����ʽͷ����ַ��� */
    std::string getImHeaderString();

protected:
    std::string         m_strHeaders;  // ��ʽ��ͷ
    hash_list           m_Headers;     // ��ʽͷ������
    bool                m_bHeadersSent; // header�Ƿ��Ѿ�����
    bool                m_bFinished;   // ����Ӧ���Ƿ����
    CTcpClient*         m_pSocket;     // tcp����
};


/** HTTP�ͻ�������,���ڿͻ�����֯���ݲ����� */
class ClientRequest : public SendingMessage
{
    typedef void(*ReqCb)(ClientRequest *request);
    typedef void(*ResCB)(ClientRequest *request, IncomingMessage* response);
public:
    ClientRequest(CTcpConnPool *pool);
    ~ClientRequest();

    PROTOCOL            protocol;  // Э��,http��https
    METHOD              method;    // ����
    std::string         path;      // ����·��
    VERSION             version;   // http�汾�� 1.0��1.1
    std::string         host;      // ������IP
    int                 port;      // �˿�
    std::string         localaddr; // ָ������IP��Ĭ��Ϊ��
    int                 localport; // ָ�����ض˿ڣ� Ĭ��Ϊ0��ֻ�к������������Ҫ���ã�����������Ҫ
    bool                keepAlive; // �Ƿ�ʹ�ó�����, trueʱ��ʹ��CTcpConnPool��������
    bool                chunked;   // Transfer-Encoding: chunked


    /** �ͻ����յ�connect������Ӧ��ʱ�ص� */
    ResCB OnConnect;
    /** �ͻ����յ�1xxӦ��(101����)ʱ�ص� */
    ResCB OnInformation;
    /** �ͻ����յ�101 upgrade ʱ�ص� */
    ResCB OnUpgrade;
    /** �ͻ����յ�Ӧ��ʱ�ص������������ָ���ص����򲻻��ٴν������� */
    ResCB OnResponse;

    /**
     * ��������һ�����ݣ����chunked=true������һ��chunk������
     * ���chunked=false��ʹ�����������η������ݣ������Լ�������ͷ������length
     * @param chunk ��Ҫ���͵�����
     * @param len ���͵����ݳ���
     * @param cb ����д�뻺������
     */
    bool Write(char* chunk, int len, ReqCb cb = NULL);

    /**
     * ���һ���������������δ���͵Ĳ������䷢�ͣ����chunked=true�����ⷢ�ͽ�����'0\r\n\r\n'
     * ���chunked=false,Э��ͷû�з��ͣ����Զ����length
     */
    bool End();

    /**
     * �൱��Write(data, len, cb);end();
     */
    void End(char* data, int len, ReqCb cb = NULL);


private:
    std::string GetAgentName();
    std::string GetHeadersString();

    CTcpConnPool        *m_pTcpPool;
    CTcpRequest          *m_pTcpReq;
};

/** ���������Ӧ�����ݲ����� */
class ServerResponse : public SendingMessage
{
    typedef void(*ResCb)(ServerResponse *response);
public:
    ServerResponse();
    ~ServerResponse();

    bool                sendDate;      // Ĭ��true���ڷ���ͷʱ�Զ����Dateͷ(�Ѵ����򲻻����)
    int                 statusCode;    // ״̬��
    std::string         statusMessage; //�Զ����״̬��Ϣ�����Ϊ�գ�����ʱ��ȡ��׼��Ϣ
    VERSION             version;       // http�汾�� 1.0��1.1
    bool                keepAlive; // �Ƿ�ʹ�ó�����, trueʱ��ʹ��CTcpConnPool��������
    bool                chunked;   // Transfer-Encoding: chunked

    /** �������ǰ,socket�ж��˻�ص��÷��� */
    ResCb OnClose;
    /** Ӧ�������ʱ�ص����������ݶ��Ѿ����� */
    ResCb OnFinish;

    /**
     * ���һ��β������
     * @param key β�����ݵ�field name�����ֵ�Ѿ���header�е�Trailer�ﶨ����
     * @param value β�����ݵ�field value
     */
    void AddTrailers(std::string key, std::string value);

    /**
     * Sends a HTTP/1.1 100 Continue message������write��end�Ĺ���
     */
    void WriteContinue();

    /**
     * Sends a HTTP/1.1 102 Processing message to the client
     */
    void WriteProcessing();

    /**
     * ��ʾ��дhttpͷ�����ú���ʽhttpͷ�Ľӿھ���Ч��
     * @param statusCode ��Ӧ״̬��
     * @param statusMessage �Զ���״̬��Ϣ������Ϊ�գ���ʹ�ñ�׼��Ϣ
     * @param headers httpͷ�������ַ�����ÿ�ж�Ҫ����"\r\n"
     */
    void WriteHead(int statusCode, std::string statusMessage, std::string headers);

    /**
     * ��������˴˷�������û�е���writeHead()����ʹ����ʽͷ����������ͷ
     */
    void Write(char* chunk, int len, ResCb cb = NULL);

    /**
     * ����Ӧ����������ݶ��Ѿ����͡�ÿ��ʵ������Ҫ����һ��end��ִ�к�ᴥ��OnFinish
     */
    void End();

    /**
     * �൱�ڵ���write(data, len, cb) ; end()
     */
    void End(char* data, int len, ResCb cb = NULL);

private:
    std::string GetHeadersString();

    hash_list   m_Trailers;
};

/** ���յ������ݣ�����˽��յ��������ͻ��˽��յ���Ӧ�� */
class IncomingMessage
{
public:
    IncomingMessage();
    ~IncomingMessage();

    bool aborted;   //������ֹʱ����Ϊtrue
    bool complete;  //http��Ϣ��������ʱ����Ϊtrue

    METHOD      method;     // ���󷽷�
    std::string url;        // ����·��

    int         statusCode;     //Ӧ��״̬��
    std::string statusMessage;  //Ӧ��״̬��Ϣ

    VERSION     httpVersion;    //http�汾�� 1.1��1.0
    std::string rawHeaders;     //������ͷ���ַ���
    std::string rawTrailers;    //������β���ַ���
    hash_list   headers;        //�����õ�httpͷ����ֵ��
    hash_list   trailers;       //�����õ�httpβ����ֵ��
};

/** http���� */
class Server
{
    typedef void(*ReqCb)(Server *server, IncomingMessage *request, ServerResponse *response);
public:
    Server(CUVNetPlus* net);
    ~Server();

    /** �������������� */
    bool Listen(std::string strIP, uint32_t nPort);
    /** �������ر� */
    void Close();
    /** �������Ƿ��ڼ������� */
    bool Listening();

    /** ���ܵ�һ������'Expect: 100-continue'������ʱ���ã����û��ָ�����Զ�����'100 Continue' */
    ReqCb OnCheckContinue;
    /** ���յ�һ������Expectͷ��������100������ʱ���ã����û��ָ�����Զ�����'417 Expectation Failed' */
    ReqCb OnCheckExpectation;
    /** �յ�upgrade����ʱ���� */
    ReqCb OnUpgrade;
    /** ���յ�һ���������������ָ���Ļص����Ͳ���������� */
    ReqCb OnRequest;

private:
    static void OnListen(CTcpServer* svr, std::string err);
    static void OnTcpConnection(CTcpServer* svr, std::string err, CTcpClient* client);

private:
    int           m_nPort;      //��������˿�
    CTcpServer   *m_pTcpSvr;    //tcp��������
};

};

}