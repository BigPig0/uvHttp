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

typedef struct fxml_attr_s {
    struct fxml_attr_s *next;    //��һ�����ԣ�NULL��ʾ����
    char   *name;       //��������
    int    name_len;    //�������Ƴ���
    char   *value;      //����ֵ
    int    value_len;   //����ֵ����
}fxml_attr_t;

typedef struct fxml_node_s {
    struct fxml_attr_s *attr;    //�ڵ�����
    struct fxml_node_s  *parent;      //�ϼ��ڵ�
    struct fxml_node_s  *next;        //�¼��ڵ�
    struct fxml_node_s  *first_child; //�ӽڵ�
    char   *name;       //�ڵ�����
    int    name_len;    //�ڵ����Ƴ���
    char   *content;    //Ҷ�ӽڵ�����
    int    content_len;
}fxml_node_t;


_THIRD_UTIL_API int parse_json(fxml_node_t** root, char* buff);

_THIRD_UTIL_API int parse_xml(fxml_node_t** root, char* buff);

_THIRD_UTIL_API void free_faster_node(fxml_node_t* root);

_THIRD_UTIL_API int parse_strcasecmp(char* buff, int len, char* str);

_THIRD_UTIL_API int parse_xmlnode_namecmp(fxml_node_t* node, char* str);

_THIRD_UTIL_API int parse_xmlattr_namecmp(fxml_attr_t* attr, char* str);

_THIRD_UTIL_API int parse_xmlattr_valuecmp(fxml_attr_t* attr, char* str);

#ifdef __cplusplus
}
#endif
#endif