#ifndef _UV_NODE_
#define _UV_NODE_ 

#ifdef __cplusplus
extern "C" {
#endif

#include "public.h"


/**
 * ��ʼ������
 * @param uv �ⲿ����uv_loop_tѭ�������������nullʱ�����ڲ�����һ���Լ���uvloop
 * @return ����һ���������
 */
extern uv_node_t* uv_node_create(void* uv);

/**
 * �ر�һ���������
 * @param h ������
 * @note �ر�ǰ��Ҫȷ���û����µĹ���ȫ���ȹر�
 */
extern void uv_node_close(uv_node_t* h);

#ifdef __cplusplus
}
#endif

#endif