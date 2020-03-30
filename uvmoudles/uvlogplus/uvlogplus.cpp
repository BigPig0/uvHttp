#include "uvlogplus.h"
#include "uvlogprivate.h"
#include "uvlogconf.h"
#include "utilc_api.h"
#include <set>
#include <stdarg.h>

namespace uvLogPlus {
    class CUVLog : public CLog
    {
    public:
        CUVLog(Configuration *conf);
        ~CUVLog();
        virtual void Write(std::string name, Level level, const char *file, int line, const char *function, const char *fmt, ...);

    public:
        void OnUVThread();
    private:
        Configuration   *m_pConfig;
        uv_loop_t        m_uvLoop;
    };

    static void _uv_loop_thread_cb(void* arg) {
        CUVLog* log = (CUVLog*)arg;
        log->OnUVThread();
    }

    static void _uv_async_write_cb(uv_async_t* handle) {
        Appender* appender = (Appender*)handle->data;
        appender->Write();
    }

    CUVLog::CUVLog(Configuration *conf)
        : m_pConfig(conf)
    {
        m_uvLoop.data = this;
        int ret = uv_loop_init(&m_uvLoop);
        if(ret < 0) {
            printf("uv loop init failed: %s\n", uv_strerror(ret));
        }

        for(auto appender : m_pConfig->appenders) {
            appender.second->uv_async.data = this;
            uv_async_init(&m_uvLoop, &appender.second->uv_async, _uv_async_write_cb);
            appender.second->Init(&m_uvLoop);
        }
        
        uv_thread_t th;
        uv_thread_create(&th, _uv_loop_thread_cb, this);
    }

    CUVLog::~CUVLog() {
        
    }

    void CUVLog::Write(std::string name, Level level, const char *file, int line, const char *function, const char *fmt, ...) {
        bool useroot = true;
        std::set<Appender*> apds;

        //������־����
        auto fit = m_pConfig->loggers.find(name);
        if(fit != m_pConfig->loggers.end()) {
            bool hasapd = false;
            for(auto appenderName : fit->second->appender_ref) {
                auto fitapd = m_pConfig->appenders.find(appenderName);
                if(fitapd != m_pConfig->appenders.end()) {
                    hasapd = true;
                    apds.insert(fitapd->second);
                }
            }
            if(!fit->second->additivity || hasapd)
                useroot = false;
        }

        //��־������Ҫд��rootĬ�ϵ�appender����
        if(useroot) {
            for(auto appenderName : m_pConfig->root->appender_ref) {
                auto fitapd = m_pConfig->appenders.find(appenderName);
                if(fitapd != m_pConfig->appenders.end()) {
                    apds.insert(fitapd->second);
                }
            }
        }

        //��ʽ����־����
        std::string format_msg;
        {
            size_t size = 4096;
            std::string buffer(size, '\0');
            char* buffer_p = const_cast<char*>(buffer.data());
            int expected = 0;
            va_list ap;

            while (true) {
                va_start(ap, fmt);
                expected = vsnprintf(buffer_p, size, fmt, ap);
                va_end(ap);
                if (expected>-1 && expected<=static_cast<int>(size)) {
                    break;
                } else {
                    /* Else try again with more space. */
                    if (expected > -1)    /* glibc 2.1 */
                        size = static_cast<size_t>(expected + 1); /* precisely what is needed */
                    else           /* glibc 2.0 */
                        size *= 2;  /* twice the old size */

                    buffer.resize(size);
                    buffer_p = const_cast<char*>(buffer.data());
                }
            }

            // expected�������ַ�����β���ţ���ֵ���ڣ�strlen(buffer_p)
            format_msg = std::string(buffer_p, expected>0?expected:0);
        }

        //����д��ÿ��appender
        //�Ͱ汾vs��make_shared��֧�ֲ�����ô��Ĺ��캯��
        std::shared_ptr<LogMsg> msg(new LogMsg(
            gettid(),
            level,
            file,
            function,
            line,
            time(NULL),
            format_msg
        ));
        for(auto appender : apds) {
            std::shared_ptr<LogMsg> msgptr = msg;
            appender->msg_queue.enqueue(msgptr);
        }
        for(auto appender : apds) {
            uv_async_send(&appender->uv_async);
        }
    }

    void CUVLog::OnUVThread() {
        while (true) {
            uv_run(&m_uvLoop, UV_RUN_DEFAULT);
            Sleep(100);
        }
    }

    //////////////////////////////////////////////////////////////////////////

    CLog* CLog::Create() {
        int ret=-1,i=0;
        char *fname[] = {"uvLog_test.json","uvLog_test.jsn","uvLog_test.xml","uvLog.json","uvLog.jsn","uvLog.xml"};

        Configuration* conf = NULL;
        for (int i=0; ret < 6; i++) {
            uv_fs_t open_req;
            ret = uv_fs_open(NULL, &open_req, fname[i], O_RDONLY, 0, NULL);
            if(ret >= 0){
                conf = ConfigParse(open_req.result);
                uv_fs_close(NULL, &open_req, open_req.result, NULL);
                if(conf)
                    break;
            }
            // �ļ���ʧ��
            printf("uv fs open %s failed:%s\n", fname[i], uv_strerror(ret));
        }
        if(NULL == conf)
            return NULL;

        return new CUVLog(conf);
    }

    CLog* Create(std::string path) {
        Configuration* conf = ConfigParse(path);
        if(NULL == conf)
            return NULL;

        return new CUVLog(conf);
    }

    CLog* Create(const char *buff) {
        Configuration* conf = ConfigParse(buff);
        if(NULL == conf)
            return NULL;

        return new CUVLog(conf);
    }
}