#pragma once
#include "uvHttp.h"
#include <string>
using namespace std;

class CHttpPlus;
class CRequest;

/** ����http����ص�����������Ϊ������������� */
typedef void(*request_cb_plus)(CRequest*, int);
/** �����յ�Ӧ������� */
typedef void(*response_data_plus)(CRequest*, char*, int);
/** ����Ӧ�������ɵĴ��� */
typedef void(*response_cb_plus)(CRequest*, int);

class CResponsePluse
{
    friend class CRequest;
public:
    ~CResponsePluse();

private:
    CResponsePluse();

};

class CRequest
{
    friend class CHttpPlus;
public:
    ~CRequest();
    void SetMethod(HTTP_METHOD eMethod);
    void SetURL(string strURL);
    void SetHost(string strHost);
    void SetKeepAlive(bool bKeepAlive);
    void SetChunked(bool bChunked);
    void SetContentLength(int nLength);
    void SetUserData(void* pData);
    void AddHeader(string strName, string strValue);
    void AddBody(char* pBuff, int nLen);

    int Request();

    void RequestCB(request_t* req, int code);
    void ResponseData(request_t* req, char* data, int len);
    void ResponseCB(request_t* req, int code);

private:
    CRequest();
    request_t*         m_pReq;
    string             m_strUrl;
    string             m_strHost;
    void*              m_pUserData;
	CResponsePluse*    m_pResponse;

    request_cb_plus    m_funReqCb;
    response_data_plus m_funResData;
    response_cb_plus   m_funResCb;
};

class CHttpPlus
{
public:
    CHttpPlus(config_t cof, void* uv);
    ~CHttpPlus();

	/**
	 * ����һ��http����
	 * req_cb ��������ɵĻص�����
	 * res_data ���յ�Ӧ������Ļص�����
	 * res_cb Ӧ�����ݽ�����ɺ�ĵ��÷���
	 */
    CRequest* CreatRequest(request_cb_plus req_cb, response_data_plus res_data, response_cb_plus res_cb);

private:
    http_t*     m_pHadle;
};
