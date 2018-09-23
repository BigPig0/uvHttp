#ifndef _MEMFILE_
#define _MEMFILE_

#ifdef __cplusplus
extern "C" {
#endif

/** �ڴ��ļ����ݽṹ */
typedef struct _memfile_
{
    char*   _buffer;      //< ��������ָ��
    size_t  _bufLen;      //< ���������С

    size_t  _readPos;     //< ��ָ��λ��
    size_t  _writePos;    //< дָ��λ��
    size_t  _fileSize;    //< �ļ���С

    size_t  _maxSize;     //< ����������С
    size_t  _memInc;      //< ÿ�������ڴ�Ĵ�С
    bool_t _selfAlloc;      //< �������Ƿ����ڲ�����
}memfile_t;

extern memfile_t* create_memfile(size_t memInc, size_t maxSize);
extern memfile_t* create_memfile_sz(void* buf, size_t len);
extern void   destory_memfile(memfile_t* mf);
extern void   mf_trunc(memfile_t* mf, bool_t freeBuf);
extern bool_t mf_reserve(memfile_t* mf, size_t r, void **buf, size_t *len);
extern size_t mf_write(memfile_t* mf, const void *buf, size_t len);
extern size_t mf_putc(memfile_t* mf, const char ch);
extern size_t mf_puts(memfile_t* mf, const char *buf);
extern size_t mf_tellp(memfile_t* mf);
extern size_t mf_read(memfile_t* mf, void *buf, size_t size);
extern char   mf_getc(memfile_t* mf);
extern size_t mf_gets(memfile_t* mf, char *buf, size_t size);
extern size_t mf_seekg(memfile_t* mf, long offset, int origin);
extern size_t mf_seekp(memfile_t* mf, long offset, int origin);
extern size_t mf_tellg(memfile_t* mf);
extern void*  mf_buffer(memfile_t* mf);
extern bool_t mf_eof(memfile_t* mf);


#ifdef __cplusplus
}
#endif
#endif