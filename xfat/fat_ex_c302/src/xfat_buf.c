/**
 * ��Դ�����׵Ŀγ�Ϊ - ��0��1����дFAT32�ļ�ϵͳ��ÿ�����̶�Ӧһ����ʱ��������ע�͡�
 * ���ߣ�����ͭ
 * �γ���ַ��http://01ketang.cc
 * ��Ȩ��������Դ��ǿ�Դ�����ο���������������ǰ����ϵ���ߡ�
 */
#include "xfat_buf.h"

/**
 * ��ȡbuf��״̬
 * @param buf
 * @param state
 */
void xfat_buf_set_state(xfat_buf_t * buf, u32_t state) {
    buf->flags &= ~XFAT_BUF_STATE_MSK;
    buf->flags |= state;
}

