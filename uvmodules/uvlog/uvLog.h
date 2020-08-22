#ifndef _UV_LOG_H_
#define _UV_LOG_H_

#if ( defined _WIN32 )
#ifndef _UV_LOG_API
#ifdef UV_LOG_EXPORT
#define _UV_LOG_API		_declspec(dllexport)
#else
#define _UV_LOG_API		extern
#endif
#endif
#elif ( defined __unix ) || ( defined __linux__ )
#ifndef _UV_LOG_API
#define _UV_LOG_API        extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _level_ {
    All = 0,
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
    OFF
}level_t;

typedef struct _uv_log_handle_ uv_log_handle_t;

_UV_LOG_API int uv_log_init(uv_log_handle_t **h);

_UV_LOG_API int uv_log_init_conf(uv_log_handle_t **h, char *conf_path);

_UV_LOG_API int uv_log_init_conf_buff(uv_log_handle_t **h, char *conf_buff);

_UV_LOG_API int uv_log_close(uv_log_handle_t *h);

/**
 * ����־�ύ����
 * @param h ��־���
 * @param name ��־���ƣ���Ӧ�����е�logger�ڵ��name����
 * @param level ��־�ȼ�
 * @param file_name ���������ļ�����
 * @param func_name �������ڵĺ�������
 * @param line �������ڵ��ļ�����
 * @param msg �ܹ����и�ʽ������־����
 * @note ����������̰߳�ȫ��
 */
_UV_LOG_API void uv_log_write(uv_log_handle_t *h, char *name, int level, 
                              char *file_name, char *func_name, int line,
                              char* msg, ...);

#define UV_LOG_WRITE(h,name,level,...) uv_log_write(h,name,level,__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
#define UV_LOG_TRACE(h,name,...) UV_LOG_WRITE(h,name,Trace,##__VA_ARGS__)
#define UV_LOG_DEBUG(h,name,...) UV_LOG_WRITE(h,name,Debug,##__VA_ARGS__)
#define UV_LOG_INFO(h,name,...)  UV_LOG_WRITE(h,name,Info, ##__VA_ARGS__)
#define UV_LOG_WARN(h,name,...)  UV_LOG_WRITE(h,name,Warn, ##__VA_ARGS__)
#define UV_LOG_ERROR(h,name,...) UV_LOG_WRITE(h,name,Error,##__VA_ARGS__)
#define UV_LOG_FATAL(h,name,...) UV_LOG_WRITE(h,name,Fatal,##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif