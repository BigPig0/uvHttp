#ifndef _UV_HTTP_PRIVATE_DEF_
#define _UV_HTTP_PRIVATE_DEF_

#ifdef __cplusplus
extern "C" {
#endif

#include "cstl.h"
#include "uv.h"
#include "utilc.h"

#define SOCKET_RECV_BUFF_LEN 1024*1024

typedef enum _uv_async_event_ {
    ASYNC_EVENT_THREAD_ID = 0,  //��ȡevent loop���߳�ID
    ASYNC_EVENT_DNS_LOOKUP,     //��ѯdns
    ASYNC_EVENT_TCP_SERVER_INIT,//tcp server��ʼ��
}uv_async_event_t;

typedef struct _uv_async_event_params_ {
    uv_async_event_t    event;
    void                *params;
}uv_async_event_params_t;

/** �����ṹ */
typedef struct _uv_node_ {
	uv_loop_t*  uv;
    uv_timer_t  timeout_timer;
	bool_t      inner_uv;
	bool_t      is_run;
    uv_thread_t loop_tid;   //event loopִ���߳�id���ⲿ���ýӿ�����������̣߳���Ҫʹ��uv_ansyc����
    uv_async_t  uv_async_h;
    list_t      *async_event;
	uv_mutex_t  uv_mutex_h;
    void        *http_global_agent;
}uv_node_t;

typedef struct _dns_lookup_query_ dns_lookup_query_t;

typedef enum _request_step_
{
    request_step_init,          //�������󣬿�ʼ����dns
    request_step_dns,           //����dns��ɣ�����agent����
    request_step_send,          //�����Ѿ����ͣ��ȴ�Ӧ��
    request_step_response       //�յ�Ӧ����δ��ɼ����ȴ�Ӧ��
}req_step_t;

typedef struct _response_p_ response_p_t;
/** �������ݽṹ */
typedef struct _request_p_ {
    REQUEST_PUBLIC
    response_p_t* res;

    string_t*      str_host;	//����url��host���ɵ�host��ַ
	string_t*      str_addr;    //host��������
	string_t*      str_port;    //host�˿ڲ���
	string_t*      str_path;    //uri��path����
    struct sockaddr* addr;      //tcp���ӵ�ַ
    string_t*      str_header;  //�����û���д���������ɵ�httpͷ
    uv_node_t*        handle;
    map_t*         headers;     //�û���д��httpͷ map<string,string>
	list_t*        body;        //�û���д��http������ list<membuff>
	uv_mutex_t     uv_mutex_h;
    int            try_times;   //ʧ�����Դ���
    req_step_t     req_step;    //��������
    time_t         req_time;    //���»��ʱ���

	request_cb     req_cb;
	response_data  res_data;
	response_cb    res_cb;
}request_p_t;

typedef enum _response_step_
{
    response_step_protocol = 0, //δ��ʼ,�����Э�� [http|https]
	response_step_version,      //�����Э��汾 [1.0|1.1|2]
    response_step_status_code,  //�����״̬��
    response_step_status_desc,  //�����״̬˵������
    response_step_header_key,   //�����Ӧ��ͷ�ֶ�
    response_step_header_value, //�����Ӧ��ͷֵ
    response_step_body,         //httpͷ�������,�����������
    response_step_chunk_len,    //�����chunk����
    response_step_chunk_buff    //�����chunk����
}res_step_t;

/** Ӧ�����ݽṹ */
typedef struct _response_p_ {
    //public
    RESPONSE_PUBLIC
    request_p_t* req;

    //private
    uv_node_t*       handle;
    map_t*        headers;        //�������Ӧ��ͷ����
    string_t*     vesion;         //httpЭ��汾
    string_t*     status_desc;    //״̬˵��

	res_step_t    parse_step;     //Ӧ�����״̬
    memfile_t*    header_buff;    //��ʱ��Ž��յ�����
	int           recived_length; //���յ����ݳ��ȣ����յĸ�chunk�ĳ���
    int           chunk_length;   //��ǰ������chunk�ĳ���
	//string_t*     chunk_left;     //һ�ν��յĻ�����ĩβchunk����û�н���ʱ������
	//uv_mutex_t    uv_mutex_h;   
}response_p_t;

/** �ͻ������ݽṹ */
typedef struct _agent_
{
    uv_node_t*     handle;
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
    int             uv_req_num; //�������������ȫ����ɲ����ͷ�

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
    uv_http_err_send_failed,
    uv_http_err_remote_disconnect,
    uv_http_err_local_disconnect
}err_code_t;

//async
extern void send_async_event(uv_node_t* h, uv_async_event_t type, void *p);
extern void dns_lookup_async(void *p);
extern void net_create_server_async(void* p);

extern void agents_init(uv_node_t* h);
extern void agents_destory(uv_node_t* h);

extern int agent_request(request_p_t* req);
extern void agent_request_finish(bool_t ok, socket_t* socket);
extern void agent_socket_disconnect(socket_t* socket);
extern void destory_request(request_p_t* req);
extern void generic_request_header(request_p_t* req);
extern int parse_dns(request_p_t* req);

extern socket_t* create_socket(agent_t* agent);
//extern void agent_free_socket(socket_t* socket);
extern void destory_socket(socket_t* socket);
extern void socket_run(socket_t* socket);

extern response_p_t* create_response(request_p_t* req);
extern void destory_response(response_p_t* res);
extern bool_t response_recive(response_p_t* res, char* data, int len, int errcode);
extern void response_error(response_p_t* res, int code);
extern void string_map_compare(const void* cpv_first, const void* cpv_second, void* pv_output);

#ifdef __cplusplus
}
#endif
#endif