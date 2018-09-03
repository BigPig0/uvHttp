#ifndef _TYPEDEF_
#define _TYPEDEF_

#ifdef __cplusplus
extern "C" {
#endif

#include "cstl.h"
#include "uv.h"
#include "stdbool.h"

/** http�����ṹ */
typedef struct _http_ {
    config_t    conf;
    map_t*      agents;
	uv_loop_t*  uv;
    uv_timer_t  timeout_timer;
	bool        inner_uv;
	bool        is_run;
}http_t;

typedef struct _response_p_ response_p_t;
/** �������ݽṹ */
typedef struct _request_p_ {
	const char*    url;
    const char*    method;
	const char*    host;
    int            keep_alive;
	int            content_length;
	void*          user_data;
	response_p_t*  res;

    string_t*      str_host;	//host��ַ
	string_t*      str_addr;    //host��������
	string_t*      str_port;    //host�˿ڲ���
	string_t*      str_path;    //uri��path����
    http_t*        handle;
    map_t*         headers;
	list_t*        body;

	request_cb     req_cb;
	response_data  res_data; 
	response_cb    res_cb;
}request_p_t;

/** Ӧ�����ݽṹ */
typedef struct _response_p_ {
	const char*   version;
	int           status;
	int           keep_alive;     //0��ʾConnectionΪclose����0��ʾkeep-alive
	int           chunked;        //POSTʹ�� 0��ʾ��ʹ��chuncked����0��ʾTransfer-Encoding: "chunked"
	int           content_length; //POSTʱ��Ҫ��ע���ݵĳ���
	request_p_t*  req;

    http_t*       handle;
    map_t*        headers;
}response_p_t;

/** �ͻ������ݽṹ */
typedef struct _agent_
{
    http_t*     handle;
    list_t*     req_list;
    set_t*      sockets;		//�����е�����
    set_t*      free_sockets;   //���п�������
	list_t*     requests;       //��������
    bool        keep_alive;
}agent_t;

/** tcp�������ݽṹ */
typedef struct _socket_
{

}socket_t;

/** �ڴ����ݽṹ */
typedef struct _membuff_
{
	unsigned char* data;
	unsigned int len;
}membuff_t;

/** ������ */
typedef enum _err_code_
{
    uv_http_ok = 0,
	uv_http_err_protocol,
	uv_http_err_dns_parse
}err_code_t;

#ifdef __cplusplus
}
#endif
#endif