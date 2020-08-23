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

        /**
     * �����ӳػ�ȡsocket�ɹ�
     * @param req ���ӳػ�ȡsocket������
     * @param skt ��ȡ����socketʵ��
     */
    static void OnPoolSocket(CTcpRequest* req, CTcpSocket* skt) {
        HttpConnReq    *httpconn = (HttpConnReq*)req->usr;
        CUNFtpRequest *ftp     = new CUNFtpRequest();

        skt->OnRecv     = OnClientRecv;
        skt->OnDrain    = OnClientDrain;
        skt->OnCLose    = OnClientClose;
        //skt->OnEnd      = OnClientEnd;
        skt->OnError    = OnClientError;
        skt->autoRecv   = true;
        skt->copy       = true;
        skt->userData   = http;

        http->host      = httpconn->host;
        http->port      = httpconn->port;
        http->usrData   = httpconn->usr;
        http->tcpSocket = skt;
        http->fd        = skt->fd;

        if(httpconn->cb)
            httpconn->cb(http, httpconn->usr, "");
        else if(httpconn->env && httpconn->env->OnRequest)
            httpconn->env->OnRequest(http, httpconn->usr, "");
        else
            skt->Delete();

        delete httpconn;
    }

    //�����ӳػ�ȡsocketʧ��
    static void OnPoolError(CTcpRequest* req, string error) {
        HttpConnReq *httpconn = (HttpConnReq*)req->usr;
        if(httpconn->cb)
            httpconn->cb(NULL, httpconn->usr, error);
        else if(httpconn->env && httpconn->env->OnRequest)
            httpconn->env->OnRequest(NULL, httpconn->usr, error);
        delete httpconn;
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

    bool CFtpClient::Request(std::string host, int port, std::string user, std::string pwd, void* usr /*= NULL*/, ReqCB cb /*= NULL*/) {
        HttpConnReq *req = new HttpConnReq();
        req->env  = this;
        req->host = host;
        req->port = port;
        req->usr  = usr;
        req->cb   = cb;
        return connPool->Request(host, port, "", req, true, true);
    }
}
}