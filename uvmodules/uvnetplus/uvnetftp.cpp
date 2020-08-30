#include "uvnetftp.h"

namespace uvNetPlus {
namespace Ftp {
    const char* szFtpCmd[] = {
        "ABOR", //�ж��������ӳ���
        "ACCT", //ϵͳ��Ȩ�ʺ�
        "ALLO", //Ϊ�������ϵ��ļ��洢�������ֽ�
        "APPE", //����ļ���������ͬ���ļ�
        "CDUP", //�ı�������ϵĸ�Ŀ¼
        "CWD",  //�ı�������ϵĹ���Ŀ¼
        "DELE", //ɾ���������ϵ�ָ���ļ�
        "HELP", //����ָ��������Ϣ
        "LIST", //������ļ����г��ļ���Ϣ�������Ŀ¼���г��ļ��б�
        "MODE", //����ģʽ��S=��ģʽ��B=��ģʽ��C=ѹ��ģʽ��
        "MKD",  //�ڷ������Ͻ���ָ��Ŀ¼
        "NLST", //�г�ָ��Ŀ¼����
        "NOOP", //�޶������������Է������ϵĳ���
        "PASS", //ϵͳ��¼����
        "PASV", //����������ȴ���������
        "PORT", //IP ��ַ�����ֽڵĶ˿� ID
        "PWD",  //��ʾ��ǰ����Ŀ¼
        "QUIT", //�� FTP ���������˳���¼
        "REIN", //���³�ʼ����¼״̬����
        "REST", //���ض�ƫ���������ļ�����
        "RETR", //�ӷ��������һأ����ƣ��ļ�
        "RMD",  //�ڷ�������ɾ��ָ��Ŀ¼
        "RNFR", //�Ծ�·��������
        "RNTO", //����·��������
        "SITE", //�ɷ������ṩ��վ���������
        "SMNT", //����ָ���ļ��ṹ
        "STAT", //�ڵ�ǰ�����Ŀ¼�Ϸ�����Ϣ
        "STOR", //���棨���ƣ��ļ�����������
        "STOU", //�����ļ���������������
        "STRU", //���ݽṹ��F=�ļ���R=��¼��P=ҳ�棩
        "SYST", //���ط�����ʹ�õĲ���ϵͳ
        "TYPE", //�������ͣ�A=ASCII��E=EBCDIC��I=binary��
        "USER"  //ϵͳ��¼���û���
    };

    //////////////////////////////////////////////////////////////////////////
    CFtpRequest::CFtpRequest()
        : port(21)
        , keepAlive(true)
        , usrData(NULL)
        , autodel(true)
        , path("/")
    {}

    CFtpRequest::~CFtpRequest() {}

    CUNFtpRequest::CUNFtpRequest() {
        uv_mutex_init(&mutex);
    }

    CUNFtpRequest::~CUNFtpRequest() {
        uv_mutex_destroy(&mutex);
    }

    /** ɾ��ʵ�� */
    void CUNFtpRequest::Delete() {
        delete this;
    }

    /**
     * �ı�������ϵĹ���Ŀ¼CWD
     */
    void CUNFtpRequest::ChangeWorkingDirectory(std::string path, ResCB cb) {
        uv_mutex_lock(&mutex);
    }

    /**
     * ��ȡ�������ļ��б�NLST
     */
    void CUNFtpRequest::FileList(ResCB cb) {
        uv_mutex_lock(&mutex);
    }

    /**
     * ��ȡ�ļ���Ϣ���ļ��б�LIST
     */
    void CUNFtpRequest::List(ResCB cb) {
        uv_mutex_lock(&mutex);
        tcpSocket->Send(szFtpCmd[FTP_CMD_LIST], strlen(szFtpCmd[FTP_CMD_LIST]));
    }

    /**
     * �����ļ�
     */
    void CUNFtpRequest::Download(string file, ResCB cb) {
        uv_mutex_lock(&mutex);
    }

    /**
     * �ϴ��ļ�
     */
    void CUNFtpRequest::Upload(string file, char *data, int size, ResCB cb) {
        uv_mutex_lock(&mutex);
    }

    /* �յ������ݴ��� */
    void CUNFtpRequest::DoReceive(const char *data, int len) {

    }

    /** ���������� */
    void CUNFtpRequest::DoError(string err) {

    }

    /** �ͻ�������ȫ������ */
    void CUNFtpRequest::DoDrain() {

    }

    //////////////////////////////////////////////////////////////////////////
    /** Http�ͻ��˻��� */
    static void OnClientRecv(CTcpSocket* skt, char *data, int len){
        CUNFtpRequest* req = (CUNFtpRequest*)skt->userData;
        req->DoReceive(data, len);
    }

    static void OnClientDrain(CTcpSocket* skt){
        //Log::debug("client drain");
        CUNFtpRequest* req = (CUNFtpRequest*)skt->userData;
        req->DoDrain();
    }

    static void OnClientClose(CTcpSocket* skt){
        //Log::debug("client close");
    }

    static void OnClientError(CTcpSocket* skt, string err){
        Log::error("client error: %s ", err.c_str());
        CUNFtpRequest* req = (CUNFtpRequest*)skt->userData;
        req->DoError(err);
    }

    //////////////////////////////////////////////////////////////////////////

    struct FtpConnReq {
        CFtpClient     *client;
        std::string     host;
        int             port;
        std::string     user;
        std::string     pwd;
        void           *usrData;
        CFtpClient::ReqCB cb;
    };

    /**
     * �����ӳػ�ȡsocket�ɹ�
     * @param req ���ӳػ�ȡsocket������
     * @param skt ��ȡ����socketʵ��
     */
    static void OnPoolSocket(CTcpRequest* req, CTcpSocket* skt) {
        FtpConnReq    *connReq = (FtpConnReq*)req->usr;
        CUNFtpRequest *ftp     = new CUNFtpRequest();

        skt->OnRecv     = OnClientRecv;
        skt->OnDrain    = OnClientDrain;
        skt->OnCLose    = OnClientClose;
        //skt->OnEnd      = OnClientEnd;
        skt->OnError    = OnClientError;
        skt->autoRecv   = true;
        skt->copy       = true;
        skt->userData   = ftp;

        ftp->host      = connReq->host;
        ftp->port      = connReq->port;
        ftp->usrData   = connReq->usrData;
        ftp->tcpSocket = skt;

        if(connReq->cb)
            connReq->cb(ftp, connReq->usrData, "");
        else if(connReq->client && connReq->client->OnRequest)
            connReq->client->OnRequest(ftp, connReq->usrData, "");
        else
            skt->Delete();

        delete connReq;
    }

    //�����ӳػ�ȡsocketʧ��
    static void OnPoolError(CTcpRequest* req, string error) {
        FtpConnReq *connReq = (FtpConnReq*)req->usr;
        if(connReq->cb)
            connReq->cb(NULL, connReq->usrData, error);
        else if(connReq->client && connReq->client->OnRequest)
            connReq->client->OnRequest(NULL, connReq->client, error);
        delete connReq;
    }

    CFtpClient::CFtpClient(CNet* net, uint32_t maxConns, uint32_t maxIdle, uint32_t timeOut, uint32_t maxRequest)
        : OnRequest(NULL)
    {
        connPool = CTcpConnPool::Create(net, OnPoolSocket);
        connPool->maxConns = maxConns;
        connPool->maxIdle  = maxIdle;
        connPool->timeOut  = timeOut;
        connPool->maxRequest = maxRequest;
        connPool->OnError  = OnPoolError;
    }

    CFtpClient::~CFtpClient() {
        connPool->Delete();
    }

    bool CFtpClient::Request(std::string host, int port, std::string user, std::string pwd, void* usrData /*= NULL*/, ReqCB cb /*= NULL*/) {
        FtpConnReq *req = new FtpConnReq();
        req->client  = this;
        req->host = host;
        req->port = port;
        req->user = user;
        req->pwd  = pwd;
        req->usrData = usrData;
        req->cb   = cb;
        return connPool->Request(host, port, "", req, true, true);
    }
}
}