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
 * ��ȡ��չ�����µ��ӷ�������
 * @param disk ��չ�������ڵĴ洢�豸
 * @param start_sector ��չ�������ڵ���ʼ����
 * @param count ��ѯ�õ����ӷ�������
 * @return
 */
static xfat_err_t disk_get_extend_part_count(xdisk_t * disk, u32_t start_sector, u32_t * count) {
    int r_count = 0;
    u8_t * disk_buffer = temp_buffer;

    u32_t ext_start_sector = start_sector;
    do {
        mbr_part_t * part;

        // ��ȡ��չ������mbr
        int err = xdisk_read_sector(disk, disk_buffer, start_sector, 1);
        if (err < 0) {
            return err;
        }

        // ��ǰ������Ч�������˳�
        part = ((mbr_t *)disk_buffer)->part_info;
        if (part->system_id == FS_NOT_VALID) {
            break;
        }

        r_count++;

        // û�к�������, �����˳�
        if ((++part)->system_id != FS_EXTEND) {
            break;
        }

        // Ѱ����һ����
        start_sector = ext_start_sector + part->relative_sectors;
    } while (1);

    *count = r_count;
    return FS_ERR_OK;
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
    u8_t extend_part_flag = 0;
    u32_t start_sector[4];

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
        } else if (part->system_id == FS_EXTEND) {
            start_sector[i] = part->relative_sectors;
            extend_part_flag |= 1 << i;
        } else {
            r_count++;
        }
	}

	// ͳ�Ƹ�����չ�������ж��ٸ��ӷ���
    if (extend_part_flag) {
        for (i = 0; i < MBR_PRIMARY_PART_NR; i++) {
            if (extend_part_flag & (1 << i)) {
                u32_t ext_count = 0;
                err = disk_get_extend_part_count(disk, start_sector[i], &ext_count);
                if (err < 0) {
                    return err;
                }

                r_count += ext_count;
            }
        }
    }

    *count = r_count;
	return FS_ERR_OK;
}
