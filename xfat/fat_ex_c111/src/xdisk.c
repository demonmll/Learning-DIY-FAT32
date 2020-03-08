/**
 * ��Դ�����׵Ŀγ�Ϊ - ��0��1����дFAT32�ļ�ϵͳ��ÿ�����̶�Ӧһ����ʱ��������ע�͡�
 * ���ߣ�����ͭ
 * �γ���ַ��http://01ketang.cc
 * ��Ȩ��������Դ��ǿ�Դ�����ο���������������ǰ����ϵ���ߡ�
 */
#include "xfat.h"
#include "xdisk.h"

u8_t temp_buffer[512];

/**
 * ��ʼ�������豸
 * @param disk ��ʼ�����豸
 * @param name �豸������
 * @return
 */
xfat_err_t xdisk_open(xdisk_t *disk, const char * name, xdisk_driver_t * driver, void * init_data) {
    xfat_err_t err;

    disk->driver = driver;

    // �ײ�������ʼ��
    err = disk->driver->open(disk, init_data);
    if (err < 0) {
        return err;
    }

    disk->name = name;
    return FS_ERR_OK;
}

/**
 * �رմ洢�豸
 * @param disk
 * @return
 */
xfat_err_t xdisk_close(xdisk_t * disk) {
    xfat_err_t err;

    err = disk->driver->close(disk);
    if (err < 0) {
        return err;
    }

    return err;
}

/**
 * ���豸�ж�ȡָ����������������
 * @param disk ��ȡ�Ĵ���
 * @param buffer ��ȡ���ݴ洢�Ļ�����
 * @param start_sector ��ȡ����ʼ����
 * @param count ��ȡ����������
 * @return
 */
xfat_err_t xdisk_read_sector(xdisk_t *disk, u8_t *buffer, u32_t start_sector, u32_t count) {
    xfat_err_t err;

    if (start_sector + count >= disk->total_sector) {
        return FS_ERR_PARAM;
    }

    err = disk->driver->read_sector(disk, buffer, start_sector, count);
    return err;
}

/**
 * ���豸��дָ������������������
 * @param disk д��Ĵ洢�豸
 * @param buffer ����Դ������
 * @param start_sector д�����ʼ����
 * @param count д���������
 * @return
 */
xfat_err_t xdisk_write_sector(xdisk_t *disk, u8_t *buffer, u32_t start_sector, u32_t count) {
    xfat_err_t err;

    if (start_sector + count >= disk->total_sector) {
        return FS_ERR_PARAM;
    }

    err = disk->driver->write_sector(disk, buffer, start_sector, count);
    return err;
}

/**
 * ��ȡ�豸���ܵķ�������
 * @param disk ��ѯ�Ĵ洢�豸
 * @param count �������洢��λ��
 * @return
 */
xfat_err_t xdisk_get_part_count(xdisk_t *disk, u32_t *count) {
	int r_count = 0, i = 0;
    mbr_part_t * part;
    u8_t * disk_buffer = temp_buffer;

    // ��ȡmbr��
	int err = xdisk_read_sector(disk, disk_buffer, 0, 1);
	if (err < 0) {
		return err;
	}

	// ����ͳ��������������������ǳ��ĸ���������չ����
	part = ((mbr_t *)disk_buffer)->part_info;
	for (i = 0; i < MBR_PRIMARY_PART_NR; i++, part++) {
		if (part->system_id == FS_NOT_VALID) {
            continue;
        } else {
            r_count++;
        }
	}

    *count = r_count;
	return FS_ERR_OK;
}
