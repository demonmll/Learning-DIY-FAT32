/**
 * ��Դ�����׵Ŀγ�Ϊ - ��0��1����дFAT32�ļ�ϵͳ��ÿ�����̶�Ӧһ����ʱ��������ע�͡�
 * ���ߣ�����ͭ
 * �γ���ַ��http://01ketang.cc
 * ��Ȩ��������Դ��ǿ�Դ�����ο���������������ǰ����ϵ���ߡ�
 */
#include "xfat.h"
#include "xdisk.h"

extern u8_t temp_buffer[512];      // todo: �����Ż�

/**
 * ��dbr�н�����fat������ò���
 * @param dbr ��ȡ���豸dbr
 * @return
 */
static xfat_err_t parse_fat_header (xfat_t * xfat, dbr_t * dbr) {
    xdisk_part_t * xdisk_part = xfat->disk_part;

    // ����DBR���������������õĲ���
    xfat->fat_tbl_sectors = dbr->fat32.BPB_FATSz32;

    // �����ֹFAT����ֻˢ��һ��FAT��
    // disk_part->start_blockΪ�÷����ľ������������ţ����Բ���Ҫ�ټ���Hidden_sector
    if (dbr->fat32.BPB_ExtFlags & (1 << 7)) {
        u32_t table = dbr->fat32.BPB_ExtFlags & 0xF;
        xfat->fat_start_sector = dbr->bpb.BPB_RsvdSecCnt + xdisk_part->start_sector + table * xfat->fat_tbl_sectors;
        xfat->fat_tbl_nr = 1;
    } else {
        xfat->fat_start_sector = dbr->bpb.BPB_RsvdSecCnt + xdisk_part->start_sector;
        xfat->fat_tbl_nr = dbr->bpb.BPB_NumFATs;
    }

    xfat->total_sectors = dbr->bpb.BPB_TotSec32;

    return FS_ERR_OK;
}

/**
 * ��ʼ��FAT��
 * @param xfat xfat�ṹ
 * @param disk_part �����ṹ
 * @return
 */
xfat_err_t xfat_open(xfat_t * xfat, xdisk_part_t * xdisk_part) {
    dbr_t * dbr = (dbr_t *)temp_buffer;
    xdisk_t * xdisk = xdisk_part->disk;
    xfat_err_t err;

    xfat->disk_part = xdisk_part;

    // ��ȡdbr������
    err = xdisk_read_sector(xdisk, (u8_t *) dbr, xdisk_part->start_sector, 1);
    if (err < 0) {
        return err;
    }

    // ����dbr�����е�fat�����Ϣ
    err = parse_fat_header(xfat, dbr);
    if (err < 0) {
        return err;
    }

    return FS_ERR_OK;
}