#pragma once
#include "uvnetpuclic.h"
#include <string>
#include <list>
#include <unordered_map>
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

class CIncomingMessage {
public:
    bool aborted;   //������ֹʱ����Ϊtrue
    bool complete;  //http��Ϣ��������ʱ����Ϊtrue

    METHOD      method;     // ���󷽷�
    std::string url;        // ����·��

    int         statusCode;     //Ӧ��״̬��
    std::string statusMessage;  //Ӧ��״̬��Ϣ

    VERSION     version;        //http�汾�� 1.1��1.0
    std::string rawHeaders;     //������ͷ���ַ���
    std::string rawTrailers;    //������β���ַ���
    hash_list   headers;        //�����õ�httpͷ����ֵ��
    hash_list   trailers;       //�����õ�httpβ����ֵ��
    bool        keepAlive;      // �Ƿ�ʹ�ó�����, trueʱ��ʹ��CTcpConnPool��������
    bool        chunked;        // Transfer-Encoding: chunked
    uint32_t    contentLen;     // chunkedΪfalseʱ�����ݳ��ȣ�chunkedΪtrueʱ���鳤��
    std::string content;        // һ�εĽ�������
protected:
    CIncomingMessage();
    virtual ~CIncomingMessage() = 0;
};

class CHttpMsg {
public:
    /**
     * ��ʾ��дhttpͷ�����ú���ʽhttpͷ�Ľӿھ���Ч��
     * @param headers httpͷ�������ַ���������ÿһ�н�β��"\r\n"
     */
    virtual void WriteHead(std::string headers) = 0;

    /**
     * ��ȡ�Ѿ��趨����ʽͷ
     */
    virtual std::vector<std::string> GetHeader(std::string name) = 0;

    /**
     * ��ȡ�����趨����ʽͷ��key
     */
    virtual std::vector<std::string> GetHeaderNames() = 0;

    /**
     * ��ʽͷ�Ƿ��Ѿ�����һ������
     */
    virtual bool HasHeader(std::string name) = 0;

    /**
     * �Ƴ�һ����ʽͷ
     */
    virtual void RemoveHeader(std::string name) = 0;

    /**
     * ��������һ��ͷ��ֵ����������һ����ʽͷ
     * param name field name
     * param value ����field value
     * param values ��NULL��β���ַ������飬���field value
     */
    virtual void SetHeader(std::string name, std::string value) = 0;
    virtual void SetHeader(std::string name, char **values) = 0;

    /**
     * �鿴�Ƿ����
     */
    virtual bool Finished() = 0;
};

class CHttpRequest : public CHttpMsg {
public:
    typedef void(*ReqCb)(CHttpRequest *request);
    typedef void(*ResCB)(CHttpRequest *request, CIncomingMessage* response);

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

    /** ����ʵ�� */
    static CHttpRequest* Create(CTcpConnPool *pool);

    /**
     * ��������һ�����ݣ����chunked=true������һ��chunk������
     * ���chunked=false��ʹ�����������η������ݣ������Լ�������ͷ������length
     * @param chunk ��Ҫ���͵�����
     * @param len ���͵����ݳ���
     * @param cb ����д�뻺������
     */
    virtual bool Write(char* chunk, int len, ReqCb cb = NULL) = 0;

    /**
     * ���һ���������������δ���͵Ĳ������䷢�ͣ����chunked=true�����ⷢ�ͽ�����'0\r\n\r\n'
     * ���chunked=false,Э��ͷû�з��ͣ����Զ����length
     */
    virtual bool End() = 0;

    /**
     * �൱��Write(data, len, cb);end();
     */
    virtual void End(char* data, int len, ReqCb cb = NULL) = 0;
protected:
    CHttpRequest();
    virtual ~CHttpRequest() = 0;
};

class CHttpResponse : public CHttpMsg {
public:
    typedef void(*ResCb)(CHttpResponse *response);

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
    virtual void AddTrailers(std::string key, std::string value) = 0;

    /**
     * Sends a HTTP/1.1 100 Continue message������write��end�Ĺ���
     */
    virtual void WriteContinue() = 0;

    /**
     * Sends a HTTP/1.1 102 Processing message to the client
     */
    virtual void WriteProcessing() = 0;

    /**
     * ��ʾ��дhttpͷ�����ú���ʽhttpͷ�Ľӿھ���Ч��
     * @param statusCode ��Ӧ״̬��
     * @param statusMessage �Զ���״̬��Ϣ������Ϊ�գ���ʹ�ñ�׼��Ϣ
     * @param headers httpͷ�������ַ�����ÿ�ж�Ҫ����"\r\n"
     */
    virtual void WriteHead(int statusCode, std::string statusMessage, std::string headers) = 0;

    /**
     * ��������˴˷�������û�е���writeHead()����ʹ����ʽͷ����������ͷ
     */
    virtual void Write(char* chunk, int len, ResCb cb = NULL) = 0;

    /**
     * ����Ӧ����������ݶ��Ѿ����͡�ÿ��ʵ������Ҫ����һ��end��ִ�к�ᴥ��OnFinish
     */
    virtual void End() = 0;

    /**
     * �൱�ڵ���write(data, len, cb) ; end()
     */
    virtual void End(char* data, int len, ResCb cb = NULL) = 0;

protected:
    CHttpResponse();
    virtual ~CHttpResponse() = 0;
};

class CHttpServer {
    typedef void(*ReqCb)(CHttpServer *server, CIncomingMessage *request, CHttpResponse *response);
public:
    /** ���ܵ�һ������'Expect: 100-continue'������ʱ���ã����û��ָ�����Զ�����'100 Continue' */
    ReqCb OnCheckContinue;
    /** ���յ�һ������Expectͷ��������100������ʱ���ã����û��ָ�����Զ�����'417 Expectation Failed' */
    ReqCb OnCheckExpectation;
    /** �յ�upgrade����ʱ���� */
    ReqCb OnUpgrade;
    /** ���յ�һ���������������ָ���Ļص����Ͳ���������� */
    ReqCb OnRequest;

    /** ����һ��ʵ�� */
    static CHttpServer* Create(CNet* net);

    /** �������������� */
    virtual bool Listen(std::string strIP, uint32_t nPort) = 0;
    /** �������ر� */
    virtual void Close() = 0;
    /** �������Ƿ��ڼ������� */
    virtual bool Listening() = 0;
protected:
    CHttpServer();
    virtual ~CHttpServer() = 0;
};
}; //namespace Http

}

