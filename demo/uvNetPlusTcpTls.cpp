#include "util.h"
#include "utilc.h"
#include "easylog.h"
#include "uvnetplus.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <thread>
//#include <windows.h>

using namespace std;
using namespace uvNetPlus;

//开启这个宏的时候，使用项目路径下的证书文件；不开启的时候使用tls库里的默认测试证书内容
//证书里的域名是localhost
#define USE_CRT_FILE
//开启这个宏的时候，客户端使用连接池
#define USE_TCP_POOL
//定义客户端请求次数
#define CLIENT_NUM 100
//定义服务端监听端口
const int svrport = 6789;


struct clientData {
    int tid;
    int finishNum;
    bool err;
    int ref;
};

void OnClientReady(CTcpSocket* skt){
    clientData* data = (clientData*)skt->userData;
    Log::debug("client %d ready", data->tid);
}

void OnClientConnect(CTcpSocket* skt, string err){
    clientData* data = (clientData*)skt->userData;
    if(err.empty()){
        char buff[1024] = {0};
        sprintf(buff, "client%d %d",data->tid, data->finishNum+1);
        skt->Send(buff, strlen(buff));
        skt->Send(" 111", 4);
    } else {
        Log::error("client%d connect err: %s", data->tid, err.c_str());
        skt->Delete();
    }
}

void OnClientRecv(CTcpSocket* skt, char *data, int len){
    clientData* c = (clientData*)skt->userData;
    c->finishNum++;
    Log::debug("client%d recv[%d]: %s", c->tid, c->finishNum, data);
#if 1
    skt->Delete();
#else
    if(c->finishNum < c->tid) {
        char buff[1024] = {0};
        sprintf(buff, "client%d %d",c->tid, c->finishNum+1);
        skt->Send(buff, strlen(buff));
        skt->Send(" AAA", 4);
    } else {
        skt->Delete();
    }
#endif
}

void OnClientDrain(CTcpSocket* skt){
    clientData* data = (clientData*)skt->userData;
    Log::debug("client%d drain", data->tid);
}

void OnClientClose(CTcpSocket* skt){
    clientData* data = (clientData*)skt->userData;
    Log::debug("client%d close", data->tid);
    delete data;
}

void OnClientEnd(CTcpSocket* skt){
    clientData* data = (clientData*)skt->userData;
    Log::debug("client%d end", data->tid);
}

void OnClientError(CTcpSocket* skt, string err){
    clientData* data = (clientData*)skt->userData;
    Log::error("client%d error: %s ", data->tid, err.c_str());
}

void testClient()
{
    CNet* net = CNet::Create();
    for (int i=0; i<CLIENT_NUM; i++) {
        clientData* data = new clientData;
        data->tid = i+1;
        data->finishNum = 0;
        CTcpSocket* client = CTcpSocket::Create(net, (void*)data);
#ifdef USE_CRT_FILE
        const char *caArray[] = {"./ssl/ca.crt", NULL};
        client->SetTlsCaFile(caArray);
#endif
        client->UseTls("localhost", true);
        client->OnReady = OnClientReady;
        client->OnRecv = OnClientRecv;
        client->OnDrain = OnClientDrain;
        client->OnCLose = OnClientClose;
        client->OnEnd = OnClientEnd;
        client->OnError = OnClientError;
        client->OnConnect = OnClientConnect;
        client->Connect("127.0.0.1", svrport );
    }
}

static void OnPoolSocket(CTcpRequest* req, CTcpSocket* skt){
    clientData* data = (clientData*)req->usr;
    //delete req;

    skt->OnRecv    = OnClientRecv;
    skt->OnDrain   = OnClientDrain;
    skt->OnCLose   = OnClientClose;
    skt->OnEnd     = OnClientEnd;
    skt->OnError   = OnClientError;
    skt->autoRecv  = true;
    skt->copy      = true;
    skt->userData  = data;

    //clientData* ddd = (clientData*)skt->userData;
    //if(ddd){
    //    data->finishNum = ddd->finishNum;
    //    delete ddd;
    //}
    skt->Send("123456789", 9);
    skt->Send("987654321", 9);
}

//从连接池获取socket失败
static void OnPoolError(CTcpRequest* req, string error) {
    clientData* data = (clientData*)req->usr;
    Log::error("get pool socket fail %d %s",data->tid, error.c_str());
    delete data;
}

static void testTcpPool(){
    std::thread t([&](){
        CNet* net = CNet::Create();
        CTcpConnPool* pool = CTcpConnPool::Create(net, OnPoolSocket);
#ifdef USE_CRT_FILE
        const char *caArray[] = {"./ssl/ca.crt", NULL};
        pool->SetTlsCaFile(caArray);
        pool->verify = true;
#endif
        pool->useTls = true;
        pool->maxConns = 5;
        pool->maxIdle  = 5;
        pool->timeOut  = 2;
        pool->maxRequest = 0;
        pool->OnError  = OnPoolError;
        for (int i=0; i<CLIENT_NUM; i++) {
            Log::debug("new request %d", i);
            clientData* data = new clientData;
            data->tid = i+1;
            data->finishNum = 0;
            pool->Request("localhost", svrport, "", data, true, true);
        }
    });
    t.detach();
}

//////////////////////////////////////////////////////////////////////////

struct sclientData {
    int id;
};

void OnSvrCltRecv(CTcpSocket* skt, char *data, int len){
    sclientData *c = (sclientData*)skt->userData;
    Log::debug("server client%d recv: %s ", c->id, data);
    char buff[1024] = {0};
    sprintf(buff, "server client%d answer",c->id);
    skt->Send(buff, strlen(buff));
    skt->Send("123456", 6);
}

void OnSvrCltDrain(CTcpSocket* skt){
    sclientData *c = (sclientData*)skt->userData;
    Log::debug("server client%d drain", c->id);
}

void OnSvrCltClose(CTcpSocket* skt){
    sclientData *c = (sclientData*)skt->userData;
    Log::debug("server client%d close", c->id);
    delete c;
}

void OnSvrCltEnd(CTcpSocket* skt){
    sclientData *c = (sclientData*)skt->userData;
    Log::debug("server client%d end", c->id);
}

void OnSvrCltError(CTcpSocket* skt, string err){
    sclientData *c = (sclientData*)skt->userData;
    Log::error("server client%d error:%s ",c->id, err.c_str());
}

void OnTcpConnection(CTcpServer* svr, std::string err, CTcpSocket* client) {
    if(!err.empty()){
        Log::error("tcp server error: %s ", err.c_str());
        return;
    }
    Log::debug("tcp server recv new connection");
    static int sid = 1;
    sclientData *c = new sclientData();
    c->id = sid++;
    client->userData = c;
    client->OnRecv = OnSvrCltRecv;
    client->OnDrain = OnSvrCltDrain;
    client->OnCLose = OnSvrCltClose;
    client->OnEnd = OnSvrCltEnd;
    client->OnError = OnSvrCltError;
}

void OnTcpSvrClose(CTcpServer* svr, std::string err) {
    if(err.empty()) {
        Log::debug("tcp server closed");
    } else {
        Log::error("tcp server closed with error: %s", err.c_str());
    }
}

void OnTcpError(CTcpServer* svr, std::string err) {
    Log::error("tcp server error: %s", err.c_str());
}

void OnTcpListen(CTcpServer* svr, std::string err) {
    if(err.empty()) {
        Log::debug("tcp server start success");
#ifdef USE_TCP_POOL
        testTcpPool();
#else
        testClient();
#endif
    } else {
        Log::error("tcp server start failed: %s ", err.c_str());
    }
}

void testServer()
{
    CNet* net = CNet::Create();
    CTcpServer* svr = CTcpServer::Create(net, OnTcpConnection);
#ifdef USE_CRT_FILE
    svr->SetCAFile("./ssl/ca.crt");
    svr->SetCrtFile("./ssl/server.crt");
    svr->SetKeyFile("./ssl/server.key");
#endif
    svr->UseTls();
    svr->OnClose = OnTcpSvrClose;
    svr->OnError = OnTcpError;
    svr->OnListen = OnTcpListen;
    svr->Listen("0.0.0.0", svrport);
}

int main()
{
    Log::open(Log::Print::both, uvLogPlus::Level::Debug, "./log/uvNetTcpTls/log.txt");
    testServer();

	sleep(INFINITE);
	return 0;
}

