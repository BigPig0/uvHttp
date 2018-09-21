#ifndef _TYPEDEF_
#define _TYPEDEF_

#ifdef __cplusplus
extern "C" {
#endif

#include "cstl.h"
#include "uv.h"
#include "memfile.h"

#define SOCKET_RECV_BUFF_LEN 1024*1024

/** http�����ṹ */
typedef struct _http_ {
    config_t    conf;
    map_t*      agents;
	uv_loop_t*  uv;
    uv_timer_t  timeout_timer;
	bool_t      inner_uv;
	bool_t      is_run;
	uv_mutex_t  uv_mutex_h;
}http_t;

typedef struct _response_p_ response_p_t;
/** �������ݽṹ */
typedef struct _request_p_ {
	HTTP_METHOD    method;
	const char*    url;
	const char*    host;
    int            keep_alive;
	int            chunked;
	int            content_length;
	void*          user_data;
	response_p_t*  res;

    string_t*      str_host;	//����url��host���ɵ�host��ַ
	string_t*      str_addr;    //host��������
	string_t*      str_port;    //host�˿ڲ���
	string_t*      str_path;    //uri��path����
    struct sockaddr* addr;      //tcp���ӵ�ַ
    string_t*      str_header;  //�����û���д���������ɵ�httpͷ
    http_t*        handle;
    map_t*         headers;     //�û���д��httpͷ map<string,string>
	list_t*        body;        //�û���д��http������ list<membuff>
	uv_mutex_t     uv_mutex_h;

	request_cb     req_cb;
	response_data  res_data; 
	response_cb    res_cb;
}request_p_t;

typedef enum _response_step_
{
    response_step_notbegin = 0,
    response_step_protocol,     //�ҵ�http/1.x
    response_step_stause_begin, //
    response_step_status_ok,    //������״̬��
    response_step_status_desc,  //������״̬˵������
    response_step_header_key,   //������Ӧ��ͷ�ֶ�
    response_step_header_value, //������Ӧ��ͷֵ
    response_step_header_end    //httpͷ�������
}res_step_t;

/** Ӧ�����ݽṹ */
typedef struct _response_p_ {
	int           status;         //Ӧ��״̬��
	int           keep_alive;     //0��ʾConnectionΪclose����0��ʾkeep-alive
	int           chunked;        //0��ʾ��ʹ��chuncked����0��ʾTransfer-Encoding: "chunked"
	int           content_length; //���ݵĳ��ȣ�һ��chunk�����ݵĳ���
	request_p_t*  req;

    http_t*       handle;
    map_t*        headers;        //�������Ӧ��ͷ����

	res_step_t    parsed_headers; //Ӧ�����״̬
    memfile_t*    header_buff;    //��ʱ��Ž��յ�����
	int           recived_length; //���յ����ݳ��ȣ����յĸ�chunk�ĳ���
	//string_t*     chunk_left;     //һ�ν��յĻ�����ĩβchunk����û�н���ʱ������
	uv_mutex_t    uv_mutex_h;   
}response_p_t;

/** �ͻ������ݽṹ */
typedef struct _agent_
{
    http_t*     handle;
    list_t*     req_list;
    set_t*      sockets;		//�����е�����
    set_t*      free_sockets;   //���п�������
	list_t*     requests;       //��������
    bool_t      keep_alive;
	uv_mutex_t  uv_mutex_h;
}agent_t;

/** tcp����״̬ */
typedef enum _socket_status_
{
    socket_uninit = 0,
    socket_init,
    socket_connected,
    socket_send,
    socket_recv,
    socket_closed
}socket_status_t;

/** tcp�������ݽṹ */
typedef struct _socket_
{
    agent_t*        agent;
    request_p_t*    req;
    socket_status_t status;
	int             isbusy; //1λ��sockets���У�0λ��free_sockets����

    uv_tcp_t        uv_tcp_h;
    uv_connect_t    uv_connect_h;  
	uv_write_t      uv_write_h;
	uv_mutex_t      uv_mutex_h;

	char            buff[SOCKET_RECV_BUFF_LEN];
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
	uv_http_err_dns_parse,
    uv_http_err_connect,
    uv_http_err_remote_disconnect,
    uv_http_err_local_disconnect
}err_code_t;


#ifdef WIN32
#define fieldcmp _stricmp
#else
#include <strings.h>
#define fieldcmp strcasecmp
#endif

extern void agents_init(http_t* h);
extern void agents_destory(http_t* h);

extern int agent_request(request_p_t* req);
extern void agent_request_finish(bool_t ok, socket_t* socket);
extern void destory_request(request_p_t* req);
extern void generic_request_header(request_p_t* req);
extern int parse_dns(request_p_t* req);

extern socket_t* create_socket(agent_t* agent);
//extern void agent_free_socket(socket_t* socket);
extern void destory_socket(socket_t* socket);
extern void socket_run(socket_t* socket);

extern response_p_t* create_response(request_p_t* req);
extern void destory_response(response_p_t* res);
extern bool response_recive(response_p_t* res, char* data, int len);
extern void response_error(response_p_t* res, int code);
extern void string_map_compare(const void* cpv_first, const void* cpv_second, void* pv_output);

#ifdef __cplusplus
}
#endif
#endif