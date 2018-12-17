#ifndef _UV_LOG_ST_
#define _UV_LOG_ST_

#ifdef __cplusplus
extern "C" {
#endif

#include "cstl.h"
#include "uv.h"
#include "uvLog.h"

typedef enum _appender_type_ {
    consol = 0,
    file,
    rolling_file
}appender_type_t;

typedef enum _consol_target_ {
    SYSTEM_OUT = 0,
    SYSTEM_ERR
}consol_target_t;

typedef enum _filter_match_ {
    ACCEPT = 0,     //����
    NEUTRAL,        //����
    DENY            //�ܾ�
}filter_match_t;

typedef struct _filter_ {
    level_t        level;
    filter_match_t on_match;
    filter_match_t mis_match;
}filter_t;

typedef struct _time_based_triggering_policy_ {
    int             interval;           //ָ����ù���һ�Σ�Ĭ����1 hour
    bool_t          modulate;           //��������ʱ�䣺��������������3am��interval��4����ô��һ�ι�������4am��������8am��12am...������7am
}time_based_triggering_policy_t;

typedef struct _size_based_triggering_policy_ {
    int             size;               //����ÿ����־�ļ��Ĵ�С
}size_based_triggering_policy_t;

typedef struct _policies_ {
    time_based_triggering_policy_t  time_policy;
    size_based_triggering_policy_t  size_policy;
}policies_t;

typedef struct _default_rollover_strategy_
{
    int             max;                //ָ��ͬһ���ļ���������м�����־�ļ�ʱ��ʼɾ����ɵ�,Ĭ��Ϊ���ͬһ�ļ�����7���ļ�
}default_rollover_strategy_t;

#define APPENDER_PUBLIC \
    appender_type_t type;\
    string_t        *name;\
    string_t        *pattern_layout;\
    list_t          *filter;
//type ָ��appender������
//name ָ��Appender������
//pattern_layout �����ʽ��������Ĭ��Ϊ:%m%n
//filter <filter_t*>��link list

typedef struct _appender_ {
    APPENDER_PUBLIC
}appender_t;

typedef struct _consol_ {
    APPENDER_PUBLIC
    consol_target_t target;             //һ��ֻ����Ĭ��:SYSTEM_OUT
}consol_t;

typedef struct _file_ {
    APPENDER_PUBLIC
    string_t        *file_name;         //ָ�������־��Ŀ���ļ���ȫ·�����ļ���
    bool_t          append;             //�Ƿ�׷�ӣ�Ĭ��false
}file_t;

typedef struct _rolling_file_ {
    APPENDER_PUBLIC
    string_t        *filePattern;       //ָ���½���־�ļ������Ƹ�ʽ.
    policies_t      policies;           //ָ��������־�Ĳ��ԣ�����ʲôʱ������½���־�ļ������־.
    default_rollover_strategy_t drs;    //����ָ��ͬһ���ļ���������м�����־�ļ�ʱ��ʼɾ����ɵģ������µ�(ͨ��max����)��
}rolling_file_t;

typedef struct _logger_ {
    level_t         level;
    string_t        *name;              //ָ����Logger�����õ�����������ڵİ�ȫ·��,�̳���Root�ڵ�
    list_t          *appender_ref;      //����ָ������־������ĸ�Appender,���û��ָ�����ͻ�Ĭ�ϼ̳���Root
    bool_t          additivity;         //appender_refָ����ֵ���Ƿ���Ȼ�����root
}logger_t;

typedef struct _configuration_ {
    level_t         status;             //����ָ��log4j����Ĵ�ӡ��־�ļ���
    int             monitorinterval;    //ָ��log4j�Զ��������õļ����ʱ�䣬��λ��s,��С��5s
    hash_map_t      *appenders;
    logger_t        *root;
    hash_map_t      *loggers;
}configuration_t;

typedef struct _task_log_msg {
    string_t    *name;
    level_t     level;
    char        *file_name;
    char        *func_name;
    int         line;
    string_t    *msg;
}task_log_msg_t;

typedef struct _task_queue {
    volatile int task_id;
    map_t        *queue;  //int:task_log_msg_t*
    uv_rwlock_t  lock;
}task_queue_t;

typedef struct _task_fifo_queue_ {
    volatile int last_queue_id; //ֻ����0��1
    volatile int queue_id; //ֻ����0��1
    task_queue_t queue[2];
}task_fifo_queue_t;

typedef struct _uv_log_handle_ {
    configuration_t *config;
    uv_loop_t       uv;
    uv_timer_t      task_timer;
    task_fifo_queue_t *task_queue;
}uv_log_handle_t;

extern filter_t*        create_filter();
extern consol_t*        create_appender_consol();
extern file_t*          create_appender_file();
extern rolling_file_t*  create_appender_rolling_file();
extern logger_t*        create_logger();
extern configuration_t* create_config();

extern void destory_config(configuration_t* conf);


extern task_log_msg_t* create_task_log_msg();
extern void destory_task_log_msg(task_log_msg_t *task);
extern task_fifo_queue_t* create_task_fifo_queue();
extern void destory_task_fifo_queue(task_fifo_queue_t *queue);
extern void add_task(task_fifo_queue_t *queue, task_log_msg_t* task);
typedef void (*task_func)(uv_log_handle_t* h, task_log_msg_t* task);
extern void get_task(uv_log_handle_t* h, task_fifo_queue_t *queue, task_func cb);

#ifdef __cplusplus
}
#endif
#endif