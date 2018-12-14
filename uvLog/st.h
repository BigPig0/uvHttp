#ifndef _UV_LOG_ST_
#define _UV_LOG_ST_

#ifdef __cplusplus
extern "C" {
#endif

#include "cstl.h"
#include "uv.h"

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

typedef struct _uv_log_handle_ {
    configuration_t *config;
    uv_loop_t       uv;
}uv_log_handle_t;

extern filter_t*        create_filter();
extern consol_t*        create_appender_consol();
extern file_t*          create_appender_file();
extern rolling_file_t*  create_appender_rolling_file();
extern logger_t*        create_logger();
extern configuration_t* create_config();

extern void destory_config(configuration_t* conf);

#ifdef __cplusplus
}
#endif
#endif