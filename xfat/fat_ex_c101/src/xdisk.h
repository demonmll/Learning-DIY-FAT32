 /**
 * ��Դ�����׵Ŀγ�Ϊ - ��0��1����дFAT32�ļ�ϵͳ��ÿ�����̶�Ӧһ����ʱ��������ע�͡�
 * ���ߣ�����ͭ
 * �γ���ַ��http://01ketang.cc
 * ��Ȩ��������Դ��ǿ�Դ�����ο���������������ǰ����ϵ���ߡ�
 */
#ifndef XDISK_H
#define	XDISK_H

#include "xtypes.h"

// ���ǰ������
struct _xdisk_t;

/**
 * ���������ӿ�
 */
typedef struct _xdisk_driver_t {
    xfat_err_t (*open) (struct _xdisk_t * disk, void * init_data);
    xfat_err_t (*close) (struct _xdisk_t * disk);
    xfat_err_t (*read_sector) (struct _xdisk_t *disk, u8_t *buffer, u32_t start_sector, u32_t count);
    xfat_err_t (*write_sector) (struct _xdisk_t *disk, u8_t *buffer, u32_t start_sector, u32_t count);
}xdisk_driver_t;

/**
 * �洢�豸����
 */
typedef struct _xdisk_t {
	u32_t sector_size;              // ���С
	u32_t total_sector;             // �ܵĿ�����
    xdisk_driver_t * driver;        // �����ӿ�
}xdisk_t;

#endif

