#include "uvnethttp.h"
#include "util_def.h"
#include "Log.h"
#include "bnf.h"
#include "utilc_string.h"
#include "StringHandle.h"
#include <set>
#include <sstream>

namespace uvNetPlus {
namespace Http {
    //////////////////////////////////////////////////////////////////////////

    static const char *_methods[] = {
        "OPTIONS",
        "HEAD",
        "GET",
        "POST",
        "PUT",
        "DELETE",
        "TRACE",
        "CONNECT"
    };

    static const char *_versions[] = {
        "HTTP/1.0",
        "HTTP/1.1"
    };

    static const char* METHODS(METHOD m){
        return _methods[m];
    }

    static const METHOD METHODS(const char* m) {
        for(int i=0; i<8; ++i) {
            if(!strcasecmp(_methods[i], m))
                return (METHOD)i;
        }
        return (METHOD)-1;
    }

    static const char* VERSIONS(VERSION v) {
        return _versions[v];
    }

    static const VERSION VERSIONS(const char* v) {
        for(int i=0; i<2; ++i) {
            if(!strcasecmp(_versions[i], v))
                return (VERSION)i;
        }
        return (VERSION)-1;
    }

    static const char* STATUS_CODES(int c){
        switch (c) {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        case 307: return "Temporary Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Timeout";
        case 409: return "Confilict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Request Entity Too Large	";
        case 414: return "Request-URI Too Long	";
        case 415: return "Unsupported Media Type";
        case 416: return "Requested Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default: break;
        }
        return "Unknow status";
    }

    //////////////////////////////////////////////////////////////////////////
    /** Http�ͻ��˻��� */
    static void OnClientReady(CTcpSocket* skt){
        Log::debug("client ready");
    }

    static void OnClientConnect(CTcpSocket* skt, string err){
        //Log::debug("HttpReq:%x, %s", skt->userData, err.c_str());
        CUNHttpRequest* http = (CUNHttpRequest*)skt->userData;
        http->DoConnected(err);
    }

    static void OnClientRecv(CTcpSocket* skt, char *data, int len){
        CUNHttpRequest* http = (CUNHttpRequest*)skt->userData;
        http->DoReceive(data, len);
    }

    static void OnClientDrain(CTcpSocket* skt){
        Log::debug("client drain");
        CUNHttpRequest* http = (CUNHttpRequest*)skt->userData;
        http->DoDrain();
    }

    static void OnClientClose(CTcpSocket* skt){
        Log::debug("client close");
    }

    static void OnClientEnd(CTcpSocket* skt){
        Log::debug("client end");
    }

    static void OnClientError(CTcpSocket* skt, string err){
        CUNHttpRequest* http = (CUNHttpRequest*)skt->userData;
        Log::error("client error: %s ", err.c_str());
    }

    /**
     * �����ӳػ�ȡsocket�ɹ�
     * @param req ���ӳػ�ȡsocket������
     * @param skt ��ȡ����socketʵ��
     * @param connected false��ʾ�½�����socket����û�����ӵ�����ˣ�true��ʾ�ӳ���ȡ����socket���Ѿ�����������
     */
    static void OnPoolSocket(CTcpRequest* req, CTcpSocket* skt, bool connected) {
        Log::debug("get socket");
        CUNHttpRequest *http = (CUNHttpRequest*)req->usr;
        http->DoGetSocket(skt); 
        skt->userData  = http;
        //delete req;
        if(!connected) {
            //socket���½�����
            skt->OnReady   = OnClientReady;
            skt->OnConnect = OnClientConnect;
            skt->OnRecv    = OnClientRecv;
            skt->OnDrain   = OnClientDrain;
            skt->OnCLose   = OnClientClose;
            skt->OnEnd     = OnClientEnd;
            skt->OnError   = OnClientError;
            skt->autoRecv  = true;
            skt->copy      = true;
        } else {
            http->DoConnected("");
        }

    }

    //�����ӳػ�ȡsocketʧ��
    static void OnPoolError(CTcpRequest* req, string error) {
        CUNHttpRequest *http = (CUNHttpRequest*)req->usr;
        //Log::debug("HttpReq:%x  TCPREQ:%x", http, req);
        http->DoConnected(error);
    }

    CHttpClientEnv::CHttpClientEnv(CNet* net, uint32_t maxConns, uint32_t maxIdle, uint32_t timeOut, uint32_t maxRequest) {
        connPool = CTcpConnPool::Create(net, OnPoolSocket);
        connPool->maxConns = maxConns;
        connPool->maxIdle  = maxIdle;
        connPool->timeOut  = timeOut;
        connPool->maxRequest = maxRequest;
        connPool->OnError  = OnPoolError;
    }

    CHttpClientEnv::~CHttpClientEnv() {
        connPool->Delete();
    }

    CHttpRequest* CHttpClientEnv::Request() {
        CUNHttpRequest *req = new CUNHttpRequest(connPool);
        return req;
    }


    //////////////////////////////////////////////////////////////////////////
    /** ����������Ϣ */

    CHttpMsg::CHttpMsg()
        : m_bHeadersSent(false)
        , m_bFinished(false)
        , m_nContentLen(0)
        , tcpSocket(NULL){}

    CHttpMsg::~CHttpMsg(){}

    void CHttpMsg::WriteHead(std::string headers) {
        m_strHeaders = headers;
    }

    std::vector<std::string> CHttpMsg::GetHeader(std::string name) {
        std::vector<std::string> ret;
        auto first = m_Headers.lower_bound(name);
        auto last = m_Headers.upper_bound(name);
        for(;first != last; ++first){
            ret.push_back(first->second);
        }
        return ret;
    }

    std::vector<std::string> CHttpMsg::GetHeaderNames() {
        std::set<std::string> tmp;
        std::vector<std::string> ret;
        for(auto &it : m_Headers){
            tmp.insert(it.first);
        }
        for(auto &it : tmp){
            ret.push_back(it);
        }
        return ret;
    }

    bool CHttpMsg::HasHeader(std::string name) {
        return m_Headers.count(name) > 0;
    }

    void CHttpMsg::RemoveHeader(std::string name) {
        auto pos = m_Headers.equal_range(name);
        for(auto it=pos.first; it!=pos.second; ++it)
            m_Headers.erase(it);
    }

    void CHttpMsg::SetHeader(std::string name, std::string value) {
        if(!strcasecmp(name.c_str(), "Content-Length")) {
            m_nContentLen = stoi(value);
            return;
        }
        RemoveHeader(name);
        m_Headers.insert(make_pair(name, value));
    }

    void CHttpMsg::SetHeader(std::string name, char **values) {
        RemoveHeader(name);
        
        for(int i = 0; values[i]; i++) {
            m_Headers.insert(make_pair(name, values[i]));
        }
    }

    void CHttpMsg::SetContentLen(uint32_t len) {
        m_nContentLen = len;
    }

    bool CHttpMsg::Finished() {
        return m_bFinished;
    }

    std::string CHttpMsg::getImHeaderString() {
        std::stringstream ss;
        for(auto &h:m_Headers) {
            ss << h.first << ": " << h.second << "\r\n";
        }
        return ss.str();
    }

    //////////////////////////////////////////////////////////////////////////
    /** �ͻ��˷������� */

    CHttpRequest::CHttpRequest()
        : protocol(HTTP)
        , method(GET)
        , path("/")
        , version(HTTP1_1)
        , host("localhost")
        , port(80)
        , localport(0)
        , keepAlive(true)
        , chunked(false)
        , usrData(NULL)
        , autodel(true)
        , OnConnect(NULL)
        , OnInformation(NULL)
        , OnUpgrade(NULL)
        , OnResponse(NULL)
        , OnError(NULL)
    {}

    CHttpRequest::~CHttpRequest(){}

    CUNHttpRequest::CUNHttpRequest(CTcpConnPool *pool)
        : connPool(pool)
        , incMsg(NULL)
        , connected(false)
        , connecting(false)
        , parseHeader(false)
    {
        uv_mutex_init(&mutex);
    }

    CUNHttpRequest::~CUNHttpRequest(){
        Log::debug("~ClientRequest HttpReq:%x USER:%x",this, usrData);
        if(tcpSocket)
            tcpSocket->Delete();
        SAFE_DELETE(incMsg);
        uv_mutex_destroy(&mutex);
    }

    void CUNHttpRequest::Delete(){
        delete this;
    }

    bool CUNHttpRequest::Write(const char* chunk, int len, DrainCB cb){
        if(cb)
            OnDrain = cb;
        Log::debug("lock %x", &mutex);
        uv_mutex_lock(&mutex);
        
        if(!m_bHeadersSent) {
            stringstream ss;
            ss << GetHeadersString() << "\r\n";
            if(chunked && method == HTTP1_1) {
                ss << std::hex << len << "\r\n";
                ss.write(chunk, len);
                ss << "\r\n";
            } else {
                ss.write(chunk, len);
            }
            string buff = ss.str();
            if(!connected) {
                sendBuffs.push_back(buff);
            } else {
                tcpSocket->Send(buff.c_str(), (int)buff.size());
            }
            m_bHeadersSent = true;
        } else {
            if(chunked && version == HTTP1_1) {
                stringstream ss;
                ss << std::hex << len << "\r\n";
                ss.write(chunk, len);
                ss << "\r\n";
                string buff = ss.str();
                if(!connected) {
                    sendBuffs.push_back(buff);
                } else {
                    tcpSocket->Send(buff.c_str(), (int)buff.size());
                }
            } else {
                if(!connected) {
                    string buff(chunk, len);
                    sendBuffs.push_back(buff);
                } else {
                    tcpSocket->Send(chunk, len);
                }
            }
        }

        // ���δ������δ��ʼ����
        if(!connected && !connecting) {
            connPool->Request(host, port, "", this, true, true);
            connecting = true;
        }
        uv_mutex_unlock(&mutex);
        Log::debug("unlock %x", &mutex);
        return true;
    }

    bool CUNHttpRequest::End() {
        Log::debug("lock %x", &mutex);
        uv_mutex_lock(&mutex);
        if(!m_bHeadersSent) {
            // ��������£������ܰ���body
            string buff = GetHeadersString() + "\r\n";
            if(!connected) {
                sendBuffs.push_back(buff);
            } else {
                tcpSocket->Send(buff.c_str(), (int)buff.size());
            }
            m_bHeadersSent = true;
        } else {
            if(chunked && method == HTTP1_1) {
                string buff = "0\r\n\r\n";
                if(!connected) {
                    sendBuffs.push_back(buff);
                } else {
                    tcpSocket->Send(buff.c_str(), (int)buff.size());
                }
            }
        }
        m_bFinished = true;

        // ���δ������δ��ʼ����
        if(!connected && !connecting) {
            connPool->Request(host, port, "", this, true, true);
            connecting = true;
        }
        uv_mutex_unlock(&mutex);
        Log::debug("unlock %x", &mutex);
        return true;
    }

    void CUNHttpRequest::End(const char* chunk, int len){
        Log::debug("lock %x", &mutex);
        uv_mutex_lock(&mutex);
        if(!chunked && !m_nContentLen) {
            m_nContentLen = len;
        }

        if(!m_bHeadersSent) {
            stringstream ss;
            ss << GetHeadersString() << "\r\n";
            if(chunked && method == HTTP1_1) {
                ss << std::hex << len << "\r\n";
                ss.write(chunk, len);
                ss << "\r\n0\r\n\r\n"; //chunk����
            } else {
                ss.write(chunk, len);
            }
            string buff = ss.str();
            if(!connected) {
                sendBuffs.push_back(buff);
            } else {
                tcpSocket->Send(buff.c_str(), (int)buff.size());
            }
            m_bHeadersSent = true;
        } else {
            if(chunked && version == HTTP1_1) {
                stringstream ss;
                ss << std::hex << len << "\r\n";
                ss.write(chunk, len);
                ss << "\r\n0\r\n\r\n";  //chunk����
                string buff = ss.str();
                if(!connected) {
                    sendBuffs.push_back(buff);
                } else {
                    tcpSocket->Send(buff.c_str(), (int)buff.size());
                }
            } else {
                if(!connected) {
                    string buff(chunk, len);
                    sendBuffs.push_back(buff);
                } else {
                    tcpSocket->Send(chunk, len);
                }
            }
        }
        m_bFinished = true;
        uv_mutex_unlock(&mutex);
        Log::debug("unlock %x", &mutex);

        // ���δ������δ��ʼ����
        if(!connected && !connecting) {
            Log::debug("1111 %x", &mutex);
            connPool->Request(host, port, "", this, true, true);
            Log::debug("2222 %x", &mutex);
            connecting = true;
        }
        //Log::debug("HttpReq:%x user:%x", this, usrData);
    }

    void CUNHttpRequest::DoGetSocket(CTcpSocket *skt) {
        tcpSocket  = skt;
        incMsg     = new IncomingMessage();
    }

    void CUNHttpRequest::DoConnected(string err) {
        if(err.empty()){
            uv_mutex_lock(&mutex);
            connected  = true;
            connecting = false;
            for(auto &buf : sendBuffs) {
                tcpSocket->Send(buf.c_str(), buf.size());
            }
            sendBuffs.clear();
            uv_mutex_unlock(&mutex);
        } else {
            //connected  = false;
            //connecting = false;
            //Log::debug("HttpReq:%x", this);
            Log::debug("lock %x", &mutex);
            uv_mutex_lock(&mutex);
            uv_mutex_unlock(&mutex);
            Log::debug("unlock %x", &mutex);
            if(OnError)
                OnError(this, err);
            if(autodel)
                Delete();
        }
    }

    void CUNHttpRequest::DoReceive(const char *data, int len){
        if(data == NULL || len == 0)
            return;

        recvBuff.append(data, len);
        // httpͷ����
        if(!parseHeader && recvBuff.find("\r\n\r\n") != std::string::npos) {
            if(!ParseHeader()) {
                Log::error("error response");
                return;
            }
        }

        //http���ݽ���
        ParseContent();

        // ���յ������ݿ������׻ص�
        if(OnResponse)
            OnResponse(this, incMsg);
        if(autodel && incMsg->complete)
            Delete();
    }

    void CUNHttpRequest::DoError(string err) {
        if(OnError) {
            OnError(this, err);
        }
    }

    void CUNHttpRequest::DoDrain(){
        if(!m_bFinished && OnDrain)
            OnDrain(this);
    }

    std::string CUNHttpRequest::GetAgentName() {
        stringstream ss;
        ss << host << ":" << port;
        if(!localaddr.empty())
            ss << ":" << localaddr;
        if(!localport)
            ss << ":" << localport;
        return ss.str();
    }

    std::string CUNHttpRequest::GetHeadersString() {
        if(!m_strHeaders.empty())
            return m_strHeaders;
		//Log::debug("request %s %s %s HttpReq:%x", METHODS(method), path.c_str(), VERSIONS(version), this);

        stringstream ss;
        ss << METHODS(method) << " " << path << " " << VERSIONS(version) << "\r\n"
            << "Host: " << host << "\r\nContent-Length: "
            << m_nContentLen << "\r\n";
        if(keepAlive && version == HTTP1_1)
            ss << "Connection: keep-alive\r\n";
        else
            ss << "Connection: close\r\n";
        if(chunked)
            ss << "Transfer-Encoding: chunked\r\n";
        for(auto &it:m_Headers) {
            ss << it.first << ": " << it.second << "\r\n";
        }
        return ss.str();
    }

    bool CUNHttpRequest::ParseHeader() {
        size_t pos1 = recvBuff.find("\r\n");        //��һ�еĽ�β
        size_t pos2 = recvBuff.find("\r\n\r\n");    //ͷ�Ľ�βλ��
        string statusline = recvBuff.substr(0, pos1);  //��һ�е�����

		size_t hpos1 = statusline.find(" ");
		size_t hpos2 = statusline.find(" ", pos1+1);

        
		incMsg->version = VERSIONS(statusline.substr(0,hpos1).c_str());
		incMsg->statusCode = stoi(statusline.substr(hpos1+1,hpos2-hpos1));
		incMsg->statusMessage = statusline.substr(hpos2+1, statusline.size()-hpos2-1);
        incMsg->rawHeaders = recvBuff.substr(pos1+2, pos2-pos1);
        recvBuff = recvBuff.substr(pos2+4, recvBuff.size()-pos2-4);

        if(incMsg->version == HTTP1_0)
            incMsg->keepAlive = false;
        else
            incMsg->keepAlive = true;

        vector<string> headers = StringHandle::StringSplit(incMsg->rawHeaders, "\r\n");
        for(auto &hh : headers) {
            string name, value;
            bool b = false;
            for(auto &c:hh){
                if(!b) {
                    if(c == ':'){
                        b = true;
                    } else {
                        name.push_back(c);
                    }
                } else {
                    if(!value.empty() || c != ' ')
                        value.push_back(c);
                }
            }
            // ���ؼ�ͷ
            if(!strcasecmp(name.c_str(), "Connection")) {
                if(!strcasecmp(value.c_str(), "Keep-Alive"))
                    incMsg->keepAlive = true;
                else if(!strcasecmp(value.c_str(), "Close"))
                    incMsg->keepAlive = false;
            } else if(!strcasecmp(name.c_str(), "Content-Length")) {
                incMsg->contentLen = stoi(value);
            } else if(!strcasecmp(name.c_str(), "Transfer-Encoding")) {
                if(incMsg->version != HTTP1_0 && !strcasecmp(value.c_str(), "chunked"))
                    incMsg->chunked = true;
            }
            //����ͷ
            incMsg->headers.insert(make_pair(name, value));
        }
        parseHeader = true;
        return true;
    }

    bool CUNHttpRequest::ParseContent() {
        if(incMsg->chunked) {
            size_t pos = recvBuff.find("\r\n");
            int len = htoi(recvBuff.substr(0, pos).c_str());
            if(recvBuff.size() - pos - 4 >= len) {
                // ����������
                if(len==0)
                    incMsg->complete = true;
                incMsg->contentLen = len;
                incMsg->content = recvBuff.substr(pos+2, len);
                recvBuff = recvBuff.substr(pos+len+4,recvBuff.size()-pos-len-4);
                return true;
            }
            return false;
        }

        //chunked falseʱ�����
        if(incMsg->contentLen == (uint32_t)-1) {
            // û�����ó��ȣ�����ֹͣ�������ݡ��ⲻ�Ǳ�׼Э�飬�Զ���Ĵ���
            incMsg->content = recvBuff;
            recvBuff.clear();
            return true;
        }

        if(recvBuff.size() >= incMsg->contentLen) {
            incMsg->content = recvBuff.substr(0, incMsg->contentLen);
            recvBuff = recvBuff.substr(incMsg->contentLen, recvBuff.size()-incMsg->contentLen);
            incMsg->complete = true;
            return true;
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    /** ���������Ӧ�����ݲ����� */

    CHttpResponse::CHttpResponse()
        : sendDate(true)
        , statusCode(200)
        , version(HTTP1_1)
        , keepAlive(true)
        , chunked(false)
        , OnClose(NULL)
        , OnFinish(NULL)
    {}

    CHttpResponse::~CHttpResponse() {}

    ServerResponse::ServerResponse()
    {}

    ServerResponse::~ServerResponse(){}

    void ServerResponse::AddTrailers(std::string key, std::string value) {
        m_Trailers.insert(make_pair(key, value));
    }

    void ServerResponse::WriteContinue() {

    }

    void ServerResponse::WriteProcessing() {

    }

    void ServerResponse::WriteHead(int statusCode, std::string statusMessage, std::string headers) {
        stringstream ss;
        ss << VERSIONS(version) << " " << statusCode << " ";
        if(!statusMessage.empty())
            ss << statusMessage;
        else
            ss << STATUS_CODES(statusCode);
        ss << "\r\n"
            << headers;
        CHttpMsg::WriteHead(ss.str());
    }

    void ServerResponse::Write(const char* chunk, int len, ResCb cb) {
        if(!m_bHeadersSent) {
            stringstream ss;
            ss << GetHeadersString() << "\r\n";
            if(chunked && version == HTTP1_1) {
                ss << std::hex << len << "\r\n";
                ss.write(chunk, len);
                ss << "\r\n";
            } else {
                ss.write(chunk, len);
            }
            string buff = ss.str();
            //Log::debug(buff.c_str());
            tcpSocket->Send(buff.c_str(), (uint32_t)buff.size());
            m_bHeadersSent = true;
        } else {
            if(chunked && version == HTTP1_1) {
                stringstream ss;
                ss << std::hex << len << "\r\n";
                ss.write(chunk, len);
                ss << "\r\n";
                string buff = ss.str();
                tcpSocket->Send(buff.c_str(), (uint32_t)buff.size());
            } else {
                tcpSocket->Send(chunk, len);
            }
        }
    }

    void ServerResponse::End() {
        m_bFinished = true;
    }

    void ServerResponse::End(const char* data, int len, ResCb cb) {
        if(!chunked && !m_nContentLen) {
            m_nContentLen = len;
        }
        Write(data, len, cb);
        End();
    }

    std::string ServerResponse::GetHeadersString() {
        if(!m_strHeaders.empty())
            return m_strHeaders;

        stringstream ss;
        ss << VERSIONS(version) << " " << statusCode << " ";
        if(!statusMessage.empty())
            ss << statusMessage;
        else
            ss << STATUS_CODES(statusCode);
        ss << "\r\nContent-Length: "
            << m_nContentLen << "\r\n";
        for(auto &it:m_Headers) {
            ss << it.first << ": " << it.second << "\r\n";
        }
        return ss.str();
    }

    //////////////////////////////////////////////////////////////////////////
    /** ����˽��յ��������ͻ��˽��յ���Ӧ�� */

    CIncomingMessage::CIncomingMessage()
        : aborted(false)
        , complete(false)
        , keepAlive(true)
        , chunked(false)
        , contentLen(-1)
    {}

    CIncomingMessage::~CIncomingMessage(){}

    IncomingMessage::IncomingMessage(){}

    IncomingMessage::~IncomingMessage(){}


    //////////////////////////////////////////////////////////////////////////
    /** http���� */

    CHttpServer::CHttpServer()
        : OnCheckContinue(NULL)
        , OnCheckExpectation(NULL)
        , OnUpgrade(NULL)
        , OnRequest(NULL)
    {}

    CHttpServer::~CHttpServer(){}

    CHttpServer* CHttpServer::Create(CNet* net){
        Server *svr = new Server(net);
        return svr;
    }

    CSvrConn::CSvrConn()
        : inc(nullptr)
        , res(nullptr)
        , parseHeader(false)
    {}

    bool CSvrConn::ParseHeader() {
        size_t pos1 = buff.find("\r\n");        //��һ�еĽ�β
        size_t pos2 = buff.find("\r\n\r\n");    //ͷ�Ľ�βλ��
        string reqline = buff.substr(0, pos1);  //��һ�е�����
        vector<string> reqlines = StringHandle::StringSplit(reqline, ' ');
        if(reqlines.size() != 3)
            return false;

        inc = new IncomingMessage();
        res = new ServerResponse();
        res->tcpSocket = client;

        inc->method = METHODS(reqlines[0].c_str());
        inc->url = reqlines[1];
        inc->version = VERSIONS(reqlines[2].c_str());
        inc->rawHeaders = buff.substr(pos1+2, pos2-pos1);
        buff = buff.substr(pos2+4, buff.size()-pos2-4);

        if(inc->version == HTTP1_0)
            inc->keepAlive = false;
        else
            inc->keepAlive = true;

        vector<string> headers = StringHandle::StringSplit(inc->rawHeaders, "\r\n");
        for(auto &hh : headers) {
            string name, value;
            bool b = false;
            for(auto &c:hh){
                if(!b) {
                    if(c == ':'){
                        b = true;
                    } else {
                        name.push_back(c);
                    }
                } else {
                    if(!value.empty() || c != ' ')
                        value.push_back(c);
                }
            }
            // ���ؼ�ͷ
            if(!strcasecmp(name.c_str(), "Connection")) {
                if(!strcasecmp(value.c_str(), "Keep-Alive"))
                    inc->keepAlive = true;
                else if(!strcasecmp(value.c_str(), "Close"))
                    inc->keepAlive = false;
            } else if(!strcasecmp(name.c_str(), "Content-Length")) {
                inc->contentLen = stoi(value);
            } else if(!strcasecmp(name.c_str(), "Transfer-Encoding")) {
                if(inc->version != HTTP1_0 && !strcasecmp(value.c_str(), "chunked"))
                    inc->chunked = true;
            }
            //����ͷ
            inc->headers.insert(make_pair(name, value));
        }
        return true;
    }

    bool CSvrConn::ParseContent() {
        if(inc->chunked) {
            size_t pos = buff.find("\r\n");
            int len = htoi(buff.substr(0, pos).c_str());
            if(buff.size() - pos - 4 >= len) {
                // ����������
                if(len==0)
                    inc->complete = true;
                inc->contentLen = len;
                inc->content = buff.substr(pos+2, len);
                buff = buff.substr(pos+len+4,buff.size()-pos-len-4);
                return true;
            }
            return false;
        }

        //chunked falseʱ�����
        if(inc->contentLen == (uint32_t)-1) {
            // û�����ó��ȣ�����ֹͣ�������ݡ��ⲻ�Ǳ�׼Э�飬�Զ���Ĵ���
            inc->content = buff;
            buff.clear();
            return true;
        }

        if(buff.size() >= inc->contentLen) {
            inc->content = buff.substr(0, inc->contentLen);
            buff = buff.substr(inc->contentLen, buff.size()-inc->contentLen);
            inc->complete = true;
            return true;
        }

        return false;
    }

    void Server::OnListen(CTcpServer* svr, std::string err) {
        Server *http = (Server*)svr->userData;
        if(err.empty()) {
            //���������ɹ�
            Log::debug("Http server listen %d begining...", http->m_nPort);
        } else {
            //����ʧ��
            Log::error("Http server listen failed: %s", err.c_str());
        }
    }

    void Server::OnSvrCltRecv(CTcpSocket* skt, char *data, int len){
        CSvrConn *c = (CSvrConn*)skt->userData;
        c->buff.append(data, len);
        // httpͷ����
        if(!c->parseHeader && c->buff.find("\r\n\r\n") != std::string::npos) {
            if(!c->ParseHeader()) {
                Log::error("error request");
                return;
            }
        }
        //http���ݽ���
        if(c->inc->method == POST){
            while(c->ParseContent()) {
                // ���յ������ݿ������׻ص�
                if(c->http->OnRequest && !c->inc->content.empty())
                    c->http->OnRequest(c->http, c->inc, c->res);
                //�Ƿ�������
                if(c->inc->complete) {
                    //һ�����������ɣ�����
                    SAFE_DELETE(c->inc);
                    SAFE_DELETE(c->res);
                    c->parseHeader = false;
                    if(!c->buff.empty()){
                        // �Ѿ����յ���һ������
                        string tmp = c->buff;
                        c->buff.clear();
                        OnSvrCltRecv(skt, (char*)tmp.c_str(), (int)tmp.size());
                    }
                }
            }
        } else {
            c->inc->complete = true;
            // ���յ������ݿ������׻ص�
            if(c->http->OnRequest)
                c->http->OnRequest(c->http, c->inc, c->res);
            //һ�����������ɣ�����
            SAFE_DELETE(c->inc);
            SAFE_DELETE(c->res);
            c->parseHeader = false;
            if(!c->buff.empty()){
                // �Ѿ����յ���һ������
                string tmp = c->buff;
                c->buff.clear();
                OnSvrCltRecv(skt, (char*)tmp.c_str(), (int)tmp.size());
            }
        }
    }

    void Server::OnSvrCltDrain(CTcpSocket* skt){
        CSvrConn *c = (CSvrConn*)skt->userData;
        Log::debug("server client drain");
    }

    void Server::OnSvrCltClose(CTcpSocket* skt){
        CSvrConn *c = (CSvrConn*)skt->userData;
        Log::debug("server client close");
        delete c;
    }

    void Server::OnSvrCltEnd(CTcpSocket* skt){
        CSvrConn *c = (CSvrConn*)skt->userData;
        Log::debug("server client end");
    }

    void Server::OnSvrCltError(CTcpSocket* skt, string err){
        CSvrConn *c = (CSvrConn*)skt->userData;
        Log::error("server client error:%s ", err.c_str());
    }

    void Server::OnTcpConnection(CTcpServer* svr, std::string err, CTcpSocket* client) {
        Log::debug("Http server accept new connection");
        Server *http = (Server*)svr->userData;
        static int sid = 1;
        CSvrConn *c = new CSvrConn();
        c->http   = http;
        c->server = svr;
        c->client = client;
        client->userData = c;
        client->OnRecv = OnSvrCltRecv;
        client->OnDrain = OnSvrCltDrain;
        client->OnCLose = OnSvrCltClose;
        client->OnEnd = OnSvrCltEnd;
        client->OnError = OnSvrCltError;
    }

    Server::Server(CNet* net)
        : m_nPort(0)
    {
        m_pTcpSvr = CTcpServer::Create(net, OnTcpConnection, this);
        m_pTcpSvr->OnListen = OnListen;
    }

    Server::~Server(){}

    bool Server::Listen(std::string strIP, uint32_t nPort){
        m_nPort = nPort;
        return m_pTcpSvr->Listen(strIP, nPort);
    }

    void Server::Close() {

    }

    bool Server::Listening() {
        return m_pTcpSvr->Listening();
    }

    //////////////////////////////////////////////////////////////////////////




}
}