#pragma once
#include "uvlogpublic.h"
#include "lock_free/concurrentqueue.h"
#include "uv.h"
#include <string>
#include <list>
#include <unordered_map>
#include <memory>

namespace uvLogPlus {

    enum class AppenderType {
        consol = 0,         //����̨
        file,               //�ļ�
        rolling_file,       //�ļ���С����ָ���ߴ��ʱ�����һ���µ��ļ�
    };

    enum class ConsolTarget {
        SYSTEM_OUT = 0,
        SYSTEM_ERR
    };

    enum FilterMatch {
        ACCEPT = 0,     //����
        NEUTRAL,        //����
        DENY            //�ܾ�
    };

    /** һ����־���� */
    struct LogMsg {
        uint32_t    tid;            //�߳�ID
        Level       level;
        const char        *file_name;
        const char        *func_name;
        int         line;
        time_t      msg_time;
        std::string msg;
        LogMsg(uint32_t _tid, Level _level, const char *_file, const char *_func, int _line, time_t _t, std::string &_msg)
            : tid(_tid)
            , level(_level)
            , file_name(_file)
            , func_name(_func)
            , line(_line)
            , msg_time(_t)
            , msg(_msg){};
    };

    struct Filter {
        Level        level;
        FilterMatch  on_match;
        FilterMatch  mis_match;
    };

    struct TimeBasedTriggeringPolicy {
        int          interval;           //ָ����ù���һ�Σ�Ĭ����1 hour
        bool         modulate;           //��������ʱ�䣺��������������3am��interval��4����ô��һ�ι�������4am��������8am��12am...������7am
    };

    struct SizeBasedTriggeringPolicy {
        int          size;               //����ÿ����־�ļ��Ĵ�С
    };

    struct Policies {
        TimeBasedTriggeringPolicy  time_policy;
        SizeBasedTriggeringPolicy  size_policy;
    };

    /** appender�������� */
    class Appender {
    public:
        AppenderType   type;    // ָ��appender������
        std::string    name;    // ָ��Appender������
        std::string    pattern_layout;  // �����ʽ��������Ĭ��Ϊ:%m%n
        std::list<Filter>  filter;      //
        moodycamel::ConcurrentQueue<std::shared_ptr<LogMsg>> 
                       msg_queue;
        uv_loop_t     *uv_loop;
        uv_async_t     uv_async;

        Appender(){};
        virtual ~Appender(){};
        virtual void Init(uv_loop_t *uv) = 0;
        virtual void Write() = 0;
    };

    /** ����̨���appender */
    class ConsolAppender : public Appender {
    public:
        ConsolTarget    target;             //һ��ֻ����Ĭ��:SYSTEM_OUT
        bool            opening;            //���ڴ򿪿���̨
        bool            opened;             //����̨�Ѿ���
        uv_tty_t        tty_handle;         //����̨���

        ConsolAppender();
        virtual ~ConsolAppender();
        virtual void Init(uv_loop_t *uv);
        virtual void Write();
    };

    /** ���ļ����appender */
    class FileAppender : public Appender {
    public:
        std::string     file_name;         //ָ�������־��Ŀ���ļ���ȫ·�����ļ���
        bool            append;            //�Ƿ�׷�ӣ�Ĭ��false
        bool            opening;           //���ڴ��ļ�
        bool            opened;            //�ļ��Ѿ���
        uv_file         file_handle;       //�򿪵��ļ����

        FileAppender();
        virtual ~FileAppender();
        virtual void Init(uv_loop_t *uv);
        virtual void Write();
    };

    /** ��̬�ļ����appender */
    class RollingFileAppender : public Appender {
    public:
        std::string     file_name;         //ָ�������־��Ŀ���ļ���ȫ·�����ļ���
        std::string     filePattern;       //ָ���½���־�ļ������Ƹ�ʽ.
        Policies        policies;          //ָ��������־�Ĳ��ԣ�����ʲôʱ������½���־�ļ������־.
        int             max;               //����ָ��ͬһ���ļ���������м�����־�ļ�ʱ��ʼɾ����ɵģ������µ�
        bool            opening;           //���ڴ��ļ�
        bool            opened;            //�ļ��Ѿ���
        uv_file         file_handle;       //�򿪵��ļ����

        RollingFileAppender();
        virtual ~RollingFileAppender();
        virtual void Init(uv_loop_t *uv);
        virtual void Write();
    };

    struct Logger {
        Level                     level;
        std::string               name;
        std::list<std::string>    appender_ref;       //д����Щappender
        bool                      additivity;         //appender_ref��Ϊ��ʱ���Ƿ���Ȼ�����root
    };

    /** ȫ������ */
    struct Configuration {
        //int             monitorinterval;    //ָ��log4j�Զ��������õļ����ʱ�䣬��λ��s,��С��5s
        std::unordered_map<std::string, Appender*>
                        appenders;
        Logger         *root;
        std::unordered_map<std::string, Logger*>
                        loggers;
    };

    /** д��־���� */
    struct LogMsgReq {
        Appender        *appender;
        std::shared_ptr<LogMsg> item;
        uv_buf_t         buff;
    };
};