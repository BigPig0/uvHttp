#ifndef _PUBLIC_TYPE_
#define _PUBLIC_TYPE_

#ifdef __cplusplus
extern "C" {
#endif



typedef enum _HTTP_METHOD_
{
	METHOD_OPTIONS = 0,
	METHOD_HEAD,
	METHOD_GET,
	METHOD_POST,
	METHOD_PUT,
	METHOD_DELETE,
	METHOD_TRACE,
	METHOD_CONNECT
}HTTP_METHOD;

typedef struct _config_
{
	int  keep_alive_secs;
	int  max_sockets;
	int  max_free_sockets;
}config_t;

typedef struct _http_ http_t;
typedef struct _response_ response_t;

typedef struct _request_ {
	HTTP_METHOD method;		    //http���󷽷�
	const char* url;		    //һ��������http��ַ("http://"����ʡ��)
	const char* host;			//http��������ʹ��host��ΪĿ���ַ����hostΪ��ʱ��url�л�ȡĿ���ַ
	int         keep_alive;     //0��ʾConnectionΪclose����0��ʾkeep-alive
	int         chunked;        //POSTʹ�� 0��ʾ��ʹ��chuncked����0��ʾTransfer-Encoding: "chunked"
	int         content_length; //POSTʱ��Ҫ��ע���ݵĳ���
	void*       user_data;      //�����û�����
	response_t* res;
}request_t;

typedef struct _response_ {
	int         status;
	int         keep_alive;     //0��ʾConnectionΪclose����0��ʾkeep-alive
	int         chunked;        //POSTʹ�� 0��ʾ��ʹ��chuncked����0��ʾTransfer-Encoding: "chunked"
	int         content_length; //POSTʱ��Ҫ��ע���ݵĳ���
	request_t*  req;
}response_t;

/** ����http����ص�����������Ϊ������������� */
typedef void(*request_cb)(request_t*, int);
/** �����յ�Ӧ������� */
typedef void(*response_data)(request_t*, char*, int);
/** ����Ӧ�������ɵĴ��� */
typedef void(*response_cb)(request_t*, int);



/** ���ݴ������ȡ��ϸ�Ĵ�����Ϣ */
extern const char* uvhttp_err_msg(int err);

/** �����Ĺ��߷��� */
extern char* url_encode(char* src);
extern char* url_decode(char* src);


#ifdef __cplusplus
}
#endif
#endif
