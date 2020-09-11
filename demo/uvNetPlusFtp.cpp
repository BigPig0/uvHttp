#include "util.h"
#include "utilc_api.h"
#include "uv.h"
#include "uvnetplus.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <thread>
using namespace std;
using namespace uvNetPlus;
using namespace uvNetPlus::Ftp;

CNet* net = NULL;

//��ȡ�ļ��б�
static void OnFtpList(CFtpRequest *req, list<CFtpFile> files) {
    for(auto f:files) {
        Log::warning("%s %s %s", f.permission.c_str(), f.date.c_str(), f.name.c_str());
    }
}

//�л�Ŀ¼
static void OnChangeWorkingDirectory(CFtpRequest *req) {
    Log::debug("change working directory success");
    req->List(OnFtpList);
}

//��ȡ�ļ��б�
static void OnNameList(CFtpRequest *req, std::list<std::string> names) {
    for(auto str:names) {
        Log::warning(str.c_str());
    }
}

//�ϴ��ļ�
static void OnUpload(CFtpRequest *req) {
    Log::debug("upload success");
}

//�����ļ�
static void OnDownload(CFtpRequest *req, char* data, uint32_t size) {
    FILE *f = fopen("d:\\test2.jpg", "wb+");
    fwrite(data, 1, size, f);
    fclose(f);
}

//�����ļ���
static void OnMakeDirectory(CFtpRequest *req) {
    Log::debug("make directory success");
}

//ɾ���ļ���
static void OnRemoveDirectory(CFtpRequest *req) {
    Log::debug("remove directory success");
}

//ɾ���ļ�
static void OnDeleteFile(CFtpRequest *req) {
    Log::debug("delete file sucess");
}

//�쳣��֧�ص�
static void OnFtpCb(CFtpRequest *req, CFtpMsg *msg) {
    Log::error("%s", msg->replyStr.c_str());
}

// ��½�ɹ�
static void OnFtpLogin(CFtpRequest *req) {
    req->ChangeWorkingDirectory("/var/ftp/vioupload", OnChangeWorkingDirectory);
    /*
    req->NameList(OnNameList);
    */
    /*
    req->List(OnFtpList);
    */
    /*
    char* data = (char*)calloc(1, 1024*1024*10); //10M�ռ�
    FILE *f = fopen("d:\\test.jpg", "rb+");
    int len = fread(data, 1, 1024*1024*10, f);
    req->Upload("testxxx.jpg", data, len, OnUpload);
    */
    /*
    //req->Download("testxxx.jpg", OnDownload);
    req->Download("notexist.jpg", OnDownload);
    */
    /*
    req->MakeDirectory("123/456", OnMakeDirectory);
    */
    /*
    req->RmDirectory("123", OnRemoveDirectory);
    */
    /*
    req->DelFile("testxxx.jpg", OnDeleteFile);
    */
}

int main()
{
    Log::open(Log::Print::both, Log::Level::debug, "./log/unNetPlusFtp.txt");
    net = CNet::Create();
    CFtpClient* ftpClient = CFtpClient::Create(net);
    ftpClient->Request("192.168.2.111", 21, "ftp", "123", OnFtpLogin, OnFtpCb);

    sleep(INFINITE);
}