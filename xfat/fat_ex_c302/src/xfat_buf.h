/**
 * ��Դ�����׵Ŀγ�Ϊ - ��0��1����дFAT32�ļ�ϵͳ��ÿ�����̶�Ӧһ����ʱ��������ע�͡�
 * ���ߣ�����ͭ
 * �γ���ַ��http://01ketang.cc
 * ��Ȩ��������Դ��ǿ�Դ�����ο���������������ǰ����ϵ���ߡ�
 */
#ifndef Xbuf_H
#define Xbuf_H

#include "xtypes.h"

#define XFAT_BUF_STATE_FREE        (0 << 0)			// �������У�δ��д���κ�����
#define XFAT_BUF_STATE_CLEAN       (1 << 0)			// �����ɾ���δ��д����
#define XFAT_BUF_STATE_DIRTY       (2 << 0)			// �����࣬�Ѿ���д�����ݣ�δ��д������
#define XFAT_BUF_STATE_MSK         (3 << 0)         // д״̬����

/**
 * ���̻�����
 */
typedef struct _xfat_buf_t {
    u8_t * buf;                         // ���ݻ�����
    u32_t sector_no;                    // ������
    u32_t flags;                        // ��ر��

    struct _xfat_buf_t * next;
    struct _xfat_buf_t * pre;
}xfat_buf_t;

#define xfat_buf_state(buf)       (buf->flags & XFAT_BUF_STATE_MSK)

void xfat_buf_set_state(xfat_buf_t * buf, u32_t state);

#endif //Xbuf_H
