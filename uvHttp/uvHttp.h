#ifndef _UV_HTTP_
#define _UV_HTTP_ 

#ifdef __cplusplus
extern "C" {
#endif

#include "public_def.h"


/**
 * ��ʼ��http����
 * @param cof ��http����������
 * @param uv �ⲿ����uv_loop_tѭ�������������nullʱ����http�����ڲ�����һ���Լ���uvloop
 * @return ����һ��http�������
 */
extern http_t* uvHttp(config_t cof, void* uv);

/**
 * �ر�һ��http�������
 * @param h ������
 */
extern void uvHttpClose(http_t* h);

/**
 * ����һ��http�ͻ�������
 * @param h http�������
 * @param req_cb �����ͺ�Ļص�����
 * @param res_data �յ�Ӧ���body�Ļص�
 * @param res_cb Ӧ�������ɺ�Ļص�
 * @return http���������þ����uvhttp������ɺ��Զ��ͷ�
 */
extern request_t* creat_request(http_t* h, request_cb req_cb, response_data res_data, response_cb res_cb);

/**
 * ���httpͷ����
 * @param req http�������
 * @param key ͷ������
 * @param value ͷ��ֵ
 */
extern void add_req_header(request_t* req, const char* key, const char* value);

/**
 * �����������http body���ݣ���POSTʱ����Ҫ
 * @param req http������
 * @param data body���ݡ�ָ��������е�����ά���ͷ�
 * @param len body�ĳ���
 * @return ������
 * @note  �÷�����request֮ǰʹ��,���Ե��ö�Σ�Ҳ����request�Ļص�������ʹ��request_write����д��
 */
extern int add_req_body(request_t* req, const char* data, int len);

/**
 * ��ȡӦ����httpЭ��汾
 * @param res httpӦ����
 */
extern char* get_res_version(response_t* res);

/**
 * ��ȡӦ���е�״̬��������
 * @param res httpӦ����
 */
extern char* get_res_status_des(response_t* res);

/**
 * ��ȡӦ���е�ͷ������
 * @param res httpӦ����
 */
extern int get_res_header_count(response_t* res);

/**
 * ��ȡӦ����ͷ��ָ��λ�õ�ͷ������
 * @param i ָ��λ��
 * @return Ӧ���ͷ������
 */
extern char* get_res_header_name(response_t* res, int i);

/**
 * ��ȡӦ����ͷ��ָ��λ�õ�ͷ���ֵ
 * @param i ָ��λ��
 * @return Ӧ���ͷ��ֵ
 */
extern char* get_res_header_value(response_t* res, int i);

/**
 * ����ͷ�����ƻ�ȡ��Ӧ��ֵ
 * @param key Ӧ���ͷ�������
 * @return Ӧ���ͷ��ֵ
 */
extern char* get_res_header(response_t* res, const char* key);

/**
 * ����http����
 * @param req http����ľ��
 * @return ������
 */
extern int request(request_t* req);

/**
 * ��������http�����body����
 * @param req http����ľ��
 * @param data body����
 * @param len body����
 * @note ���������request_cb�ص������������
 */
extern int request_write(request_t* req, char* data, int len);


#ifdef __cplusplus
}
#endif

#endif