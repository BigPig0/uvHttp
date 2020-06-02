/**
 * �ļ�ϵͳ�����ķ���
 */

#ifndef _UTILC_FILE_SYS_
#define _UTILC_FILE_SYS_

#include "utilc_export.h"

#if defined(WINDOWS_IMPL)
#define LINE_SEPARATOR "\r\n"
#elif defined(APPLE_IMPL)
#define LINE_SEPARATOR "\r"
#else
#define LINE_SEPARATOR "\n"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ���һ���ļ����ϼ�Ŀ¼�Ƿ���ڣ�����������򴴽���
 * ����������һ��Ŀ¼������'\\'��'/'��β�����鲢������Ŀ¼��
 * ����������һ���ļ�����'\\'��'/'��β��ֻ�����ϼ�Ŀ¼���������ļ���
 * @return 0�ɹ�  -1ʧ��
 */
_UTILC_API int file_sys_check_path(const char *path);

/**
 * ����ļ��Ƿ����
 * @return 0���� -1������
 */
_UTILC_API int file_sys_exist(const char *path);

#ifdef __cplusplus
}
#endif
#endif