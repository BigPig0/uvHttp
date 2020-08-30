/**
 * FTP��FILE TRANSFER PROTOCOL�����ļ�����Э��
 * PI��protocol interpreter����Э������� �û��ͷ���������������Э�飬���ǵľ���ʵ�ֱַ��Ϊ�û� PI ��USER-PI���ͷ�����PI��SERVER-PI��
 * ������PI��server-PI���������� PI �� L �˿ڡ��������û�Э����������������󲢽����������ӡ������û� PI���ձ�׼�� FTP ���������Ӧ������������� DTP
 * ������DTP��server-DTP�������ݴ�����̣���ͨ���ġ�������״̬�����á������������ݶ˿ڽ����������ӡ�����������ʹ洢���������ڷ������� PI �������´������ݡ��������� DTP Ҳ�������ڡ�������ģʽ�����������������ݶ˿ڽ������ӡ�
 * �û�PI��user-PI�����û�Э��������� U �˿ڽ����������� FTP ���̵Ŀ������ӣ������ļ�����ʱ�����û� DTP��
 * �û�DTP��user-DTP�������ݴ�����������ݶ˿ڡ������������� FTP ���̵����ӡ�
 * �������ӣ��û�PI �������PI ���������������Ӧ����Ϣ����ͨ����
 * �������ӣ�ͨ����������Э�̵�ģʽ�����ͽ������ݴ��䡣
 */
#pragma once
#include "uvnetpuclic.h"
#include "uvnettcp.h"
#include <string>
#include <stdint.h>

namespace uvNetPlus {
namespace Ftp {
class CUNFtpRequest : public CFtpRequest 
{
public:
    CUNFtpRequest();

    ~CUNFtpRequest();

    /** ɾ��ʵ�� */
    virtual void Delete();

    /**
     * �ı�������ϵĹ���Ŀ¼CWD
     */
    virtual void ChangeWorkingDirectory(std::string path, ResCB cb);

    /**
     * ��ȡ�������ļ��б�NLST
     */
    virtual void FileList(ResCB cb);

    /**
     * ��ȡ�ļ���Ϣ���ļ��б�LIST
     */
    virtual void List(ResCB cb);

    /**
     * �����ļ�
     */
    virtual void Download(string file, ResCB cb);

    /**
     * �ϴ��ļ�
     */
    virtual void Upload(string file, char *data, int size, ResCB cb);

    /* �յ������ݴ��� */
    void DoReceive(const char *data, int len);

    /** ���������� */
    void DoError(string err);

    /** �ͻ�������ȫ������ */
    void DoDrain();

private:
    CFtpMsg              msg;
    uv_mutex_t           mutex;         //һ��ʵ���Ķ�������ܲ���
    std::string          recvBuff;      //�������ݻ���
};

class CUNFtpClient: public CFtpClient
{

};

}
}