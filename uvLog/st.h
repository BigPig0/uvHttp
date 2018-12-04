#ifndef _UV_LOG_ST_
#define _UV_LOG_ST_

#ifdef __cplusplus
extern "C" {
#endif

#include "cstl.h"

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

typedef struct _filter_list_ {
    level_t        level;
    filter_match_t on_match;
    filter_match_t mis_match;
    struct _filter_list_ *next;
}filter_list_t;

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

typedef struct _consol_ {
    char            *name;              //ָ��Appender������
    char            *pattern_layout;    //�����ʽ��������Ĭ��Ϊ:%m%n
    consol_target_t target;             //һ��ֻ����Ĭ��:SYSTEM_OUT
    filter_list_t   *filter;
}consol_t;

typedef struct _file_ {
    char            *name;              //ָ��Appender������
    char            *pattern_layout;    //�����ʽ��������Ĭ��Ϊ:%m%n
    char            *file_name;         //ָ�������־��Ŀ���ļ���ȫ·�����ļ���
    int             append;             //�Ƿ�׷�ӣ�Ĭ��false
    filter_list_t   *filter;
}file_t;

typedef struct _rolling_file_ {
    char            *name;              //ָ��Appender������
    char            *pattern_layout;    //�����ʽ��������Ĭ��Ϊ:%m%n
    char            *file_name;         //ָ�������־��Ŀ���ļ���ȫ·�����ļ���
    char            *filePattern;       //ָ���½���־�ļ������Ƹ�ʽ.
    filter_list_t   *filter;
    policies_t      policies;           //ָ��������־�Ĳ��ԣ�����ʲôʱ������½���־�ļ������־.
    default_rollover_strategy_t drs;    //����ָ��ͬһ���ļ���������м�����־�ļ�ʱ��ʼɾ����ɵģ������µ�(ͨ��max����)��
}rolling_file_t;

typedef struct _appender_ {
    appender_type_t     type;
    void*               app;            //consol_t file_t rolling_file_t
}appender_t;

typedef struct _logger_ {
    level_t         level;
    char*           name;               //ָ����Logger�����õ�����������ڵİ�ȫ·��,�̳���Root�ڵ�
    vector_t        *appender_ref;      //����ָ������־������ĸ�Appender,���û��ָ�����ͻ�Ĭ�ϼ̳���Root
    bool_t          additivity;         //appender_refָ����ֵ���Ƿ���Ȼ�����root
}logger_t;

typedef struct _configuration_ {
    level_t         status;             //����ָ��log4j����Ĵ�ӡ��־�ļ���
    int             monitorinterval;    //ָ��log4j�Զ��������õļ����ʱ�䣬��λ��s,��С��5s
    hash_map_t      *appenders;
    logger_t        root;
    hash_map_t      *loggers;
}configuration_t;

typedef struct _uv_log_handle_ {
    configuration_t config;
    uv_loop_t       uv;
}uv_log_handle_t;

#ifdef __cplusplus
}
#endif
#endif