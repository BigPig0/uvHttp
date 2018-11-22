#ifndef _H_FASTER_PARSE
#define _H_FASTER_PARSE

#ifdef __cplusplus
extern "C" {
#endif

#if ( defined _WIN32 )
#ifndef _THIRD_UTIL_API
#ifdef THIRD_UTIL_EXPORT
#define _THIRD_UTIL_API		_declspec(dllexport)
#else
#define _THIRD_UTIL_API		extern
#endif
#endif
#elif ( defined __unix ) || ( defined __linux__ )
#ifndef _THIRD_UTIL_API
#define _THIRD_UTIL_API		extern
#endif
#endif

typedef struct faster_attribute_s {
    char   *name;       //��������
    int    name_len;    //�������Ƴ���
    char   *value;      //����ֵ
    int    value_len;   //����ֵ����
    struct faster_attribute_s *next;    //��һ�����ԣ�NULL��ʾ����
}faster_attribute_t;

typedef struct faster_node_s {
    char   *name;       //�ڵ�����
    int    name_len;    //�ڵ����Ƴ���
    struct faster_attribute_s *attr;    //�ڵ�����
    struct faster_node_s  *parent;      //�ϼ��ڵ�
    struct faster_node_s  *next;        //�¼��ڵ�
    struct faster_node_s  *first_child; //�ӽڵ�
}faster_node_t;


_THIRD_UTIL_API int parse_json(faster_node_t** root, char* buff);

_THIRD_UTIL_API int parse_xml(faster_node_t** root, char* buff);

_THIRD_UTIL_API void free_faster_node(faster_node_t* root);

#ifdef __cplusplus
}
#endif
#endif