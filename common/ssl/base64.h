#ifndef BASE64_H__
#define BASE64_H__

#include "ExportDefine.h"
#include <iostream>
#include <string>
using namespace std;

class COMMON_API CBase64
{
public:
    /*����
    DataByte
        [in]��������ݳ���,���ֽ�Ϊ��λ
    */
    static std::string Encode(const unsigned char* Data,int DataByte);
    /*����
    DataByte
        [in]��������ݳ���,���ֽ�Ϊ��λ
    OutByte
        [out]��������ݳ���,���ֽ�Ϊ��λ,�벻Ҫͨ������ֵ����
        ������ݵĳ���
    */
    static std::string Decode(const char* Data,int DataByte,int& OutByte);
};

#endif


