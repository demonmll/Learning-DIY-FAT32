/**
 * ��Դ�����׵Ŀγ�Ϊ - ��0��1����дFAT32�ļ�ϵͳ��ÿ�����̶�Ӧһ����ʱ��������ע�͡�
 * ���ߣ�����ͭ
 * �γ���ַ��http://01ketang.cc
 * ��Ȩ��������Դ��ǿ�Դ�����ο���������������ǰ����ϵ���ߡ�
 */
#ifndef XFAT_H
#define XFAT_H

#include "xtypes.h"
#include "xdisk.h"

#pragma pack(1)

/**
 * FAT�ļ�ϵͳ��BPB�ṹ
 */
typedef struct _bpb_t {
    u8_t BS_jmpBoot[3];                 // ��ת����
    u8_t BS_OEMName[8];                 // OEM����
    u16_t BPB_BytsPerSec;               // ÿ�����ֽ���
    u8_t BPB_SecPerClus;                // ÿ��������
    u16_t BPB_RsvdSecCnt;               // ������������
    u8_t BPB_NumFATs;                   // FAT������
    u16_t BPB_RootEntCnt;               // ��Ŀ¼��Ŀ��
    u16_t BPB_TotSec16;                 // �ܵ�������
    u8_t BPB_Media;                     // ý������
    u16_t BPB_FATSz16;                  // FAT�����С
    u16_t BPB_SecPerTrk;                // ÿ�ŵ�������
    u16_t BPB_NumHeads;                 // ��ͷ��
    u32_t BPB_HiddSec;                  // ����������
    u32_t BPB_TotSec32;                 // �ܵ�������
} bpb_t;

/**
 * BPB�е�FAT32�ṹ
 */
typedef struct _fat32_hdr_t {
    u32_t BPB_FATSz32;                  // FAT����ֽڴ�С
    u16_t BPB_ExtFlags;                 // ��չ���
    u16_t BPB_FSVer;                    // �汾��
    u32_t BPB_RootClus;                 // ��Ŀ¼�Ĵغ�
    u16_t BPB_FsInfo;                   // fsInfo��������
    u16_t BPB_BkBootSec;                // ��������
    u8_t BPB_Reserved[12];
    u8_t BS_DrvNum;                     // �豸��
    u8_t BS_Reserved1;
    u8_t BS_BootSig;                    // ��չ���
    u32_t BS_VolID;                     // �����к�
    u8_t BS_VolLab[11];                 // �������
    u8_t BS_FileSysType[8];             // �ļ���������
} fat32_hdr_t;

/**
 * ������DBR����
 */
typedef struct _dbr_t {
    bpb_t bpb;                          // BPB�ṹ
    fat32_hdr_t fat32;                  // FAT32�ṹ
} dbr_t;

#define CLUSTER_INVALID                 0x0FFFFFFF          // ��Ч�Ĵغ�
#define FILE_DEFAULT_CLUSTER            0x00                // �ļ���ȱʡ�غ�

#define DIRITEM_NAME_FREE               0xE5                // Ŀ¼����������
#define DIRITEM_NAME_END                0x00                // Ŀ¼����������

#define DIRITEM_NTRES_BODY_LOWER        0x08                // �ļ���Сд
#define DIRITEM_NTRES_EXT_LOWER         0x10                // ��չ��Сд
#define DIRITEM_NTRES_ALL_UPPER         0x00                // �ļ���ȫ����д
#define DIRITEM_NTRES_CASE_MASK         0x18                // ��Сд����

#define DIRITEM_ATTR_READ_ONLY          0x01                // Ŀ¼�����ԣ�ֻ��
#define DIRITEM_ATTR_HIDDEN             0x02                // Ŀ¼�����ԣ�����
#define DIRITEM_ATTR_SYSTEM             0x04                // Ŀ¼�����ԣ�ϵͳ����
#define DIRITEM_ATTR_VOLUME_ID          0x08                // Ŀ¼�����ԣ���id
#define DIRITEM_ATTR_DIRECTORY          0x10                // Ŀ¼�����ԣ�Ŀ¼
#define DIRITEM_ATTR_ARCHIVE            0x20                // Ŀ¼�����ԣ��鵵
#define DIRITEM_ATTR_LONG_NAME          0x0F                // Ŀ¼�����ԣ����ļ���

#define DIRITEM_GET_FREE        (1 << 0)
#define DIRITEM_GET_USED        (1 << 2)
#define DIRITEM_GET_END         (1 << 3)
#define DIRITEM_GET_ALL         0xff

/**
 * FATĿ¼�����������
 */
typedef struct _diritem_date_t {
    u16_t day : 5;                  // ��
    u16_t month : 4;                // ��
    u16_t year_from_1980 : 7;       // ��
} diritem_date_t;

/**
 * FATĿ¼���ʱ������
 */
typedef struct _diritem_time_t {
    u16_t second_2 : 5;             // 2��
    u16_t minute : 6;               // ��
    u16_t hour : 5;                 // ʱ
} diritem_time_t;

/**
 * FATĿ¼��
 */
typedef struct _diritem_t {
    u8_t DIR_Name[8];                   // �ļ���
    u8_t DIR_ExtName[3];                // ��չ��
    u8_t DIR_Attr;                      // ����
    u8_t DIR_NTRes;
    u8_t DIR_CrtTimeTeenth;             // ����ʱ��ĺ���
    diritem_time_t DIR_CrtTime;         // ����ʱ��
    diritem_date_t DIR_CrtDate;         // ��������
    diritem_date_t DIR_LastAccDate;     // ����������
    u16_t DIR_FstClusHI;                // �غŸ�16λ
    diritem_time_t DIR_WrtTime;         // �޸�ʱ��
    diritem_date_t DIR_WrtDate;         // �޸�ʱ��
    u16_t DIR_FstClusL0;                // �غŵ�16λ
    u32_t DIR_FileSize;                 // �ļ��ֽڴ�С
} diritem_t;

/**
 * ������
 */
typedef union _cluster32_t {
    struct {
        u32_t next : 28;                // ��һ��
        u32_t reserved : 4;             // ������Ϊ0
    } s;
    u32_t v;
} cluster32_t;

#pragma pack()

/**
 * xfat�ṹ
 */
typedef struct _xfat_t {
    u32_t fat_start_sector;             // FAT����ʼ����
    u32_t fat_tbl_nr;                   // FAT������
    u32_t fat_tbl_sectors;              // ÿ��FAT���������
    u32_t sec_per_cluster;              // ÿ�ص�������
    u32_t root_cluster;                 // ��Ŀ¼��������
    u32_t cluster_byte_size;            // ÿ���ֽ���
    u32_t total_sectors;                // ��������

    u8_t * fat_buffer;             // FAT�����
    xdisk_part_t * disk_part;           // ��Ӧ�ķ�����Ϣ
} xfat_t;

/**
 * ʱ�������ṹ
 */
typedef struct _xfile_time_t {
    u16_t year;
    u8_t month;
    u8_t day;
    u8_t hour;
    u8_t minute;
    u8_t second;
}xfile_time_t;

/**
 * �ļ�����
 */
typedef enum _xfile_type_t {
    FAT_DIR,
    FAT_FILE,
    FAT_VOL,
} xfile_type_t;

#define XFILE_ATTR_READONLY         (1 << 0)        // �ļ�ֻ��

#define SFN_LEN                     11              // sfn�ļ�����

#define XFILE_LOCATE_NORMAL         (1 << 0)        // ������ͨ�ļ�
#define XFILE_LOCATE_DOT            (1 << 1)        // ����.��..�ļ�
#define XFILE_LOCATE_VOL            (1 << 2)        // ���Ҿ��
#define XFILE_LOCALE_SYSTEM         (1 << 3)        // ����ϵͳ�ļ�
#define XFILE_LOCATE_HIDDEN         (1 << 4)        // ���������ļ�
#define XFILE_LOCATE_ALL            0xFF            // ��������

/**
 * �ļ���Ϣ�ṹ
 */
typedef struct _xfileinfo_t {
#define X_FILEINFO_NAME_SIZE        32
    char file_name[X_FILEINFO_NAME_SIZE];       // �ļ���

    u32_t size;                                 // �ļ��ֽڴ�С
    u16_t attr;                                 // �ļ�����
    xfile_type_t type;                          // �ļ�����
    xfile_time_t create_time;                       // ����ʱ��
    xfile_time_t last_acctime;                      // ������ʱ��
    xfile_time_t modify_time;                       // ����޸�ʱ��
} xfileinfo_t;

/**
 * �ļ�����
 */
typedef struct _xfile_t {
    xfat_t *xfat;                   // ��Ӧ��xfat�ṹ

    u32_t size;                     // �ļ���С
    u16_t attr;                     // �ļ�����
    xfile_type_t type;              // �ļ�����
    u32_t pos;                      // ��ǰλ��
    xfat_err_t err;                  // ��һ�εĲ���������

    u32_t start_cluster;            // ��������ʼ�غ�
    u32_t curr_cluster;             // ��ǰ�غ�
	u32_t dir_cluster;              // ���ڵĸ�Ŀ¼����������ʼ�غ�
	u32_t dir_cluster_offset;       // ���ڵĸ�Ŀ¼��������Ĵ�ƫ��
} xfile_t;

/**
 * �ļ�seek�Ķ�λ����
 */
typedef enum _xfile_orgin_t {
    XFAT_SEEK_SET,                    // �ļ���ͷ
    XFAT_SEEK_CUR,                    // ��ǰλ��
    XFAT_SEEK_END,                    // �ļ���β
}xfile_orgin_t;

/**
 * �ļ��޸ĵ�ʱ������
 */
typedef enum _stime_type_t {
    XFAT_TIME_ATIME,                  // �޸ķ���ʱ��
    XFAT_TIME_CTIME,                  // �޸Ĵ���ʱ��
    XFAT_TIME_MTIME,                  // �޸��޸�ʱ��
}stime_type_t;

u32_t cluster_fist_sector(xfat_t *xfat, u32_t cluster_no);
xfat_err_t is_cluster_valid(u32_t cluster);
xfat_err_t get_next_cluster(xfat_t *xfat, u32_t curr_cluster_no, u32_t *next_cluster);
xfat_err_t read_cluster(xfat_t *xfat, u8_t *buffer, u32_t cluster, u32_t count);

xfat_err_t xfat_open(xfat_t * xfat, xdisk_part_t * xdisk_part);

xfat_err_t xfile_open(xfat_t * xfat, xfile_t *file, const char *path);
xfat_err_t xfile_open_sub(xfile_t * dir, const char * sub_path, xfile_t* sub_file);
xfat_err_t xfile_close(xfile_t *file);
xfat_err_t xdir_first_file(xfile_t *file, xfileinfo_t *info);
xfat_err_t xdir_next_file(xfile_t *file, xfileinfo_t *info);
xfat_err_t xfile_error(xfile_t * file);
void xfile_clear_err(xfile_t * file);

xfat_err_t xfile_mkfile (xfat_t * xfat, const char * path);
xfile_size_t xfile_read(void * buffer, xfile_size_t elem_size, xfile_size_t count, xfile_t * file);
xfile_size_t xfile_write(void * buffer, xfile_size_t elem_size, xfile_size_t count, xfile_t * file);

xfat_err_t xfile_eof(xfile_t * file);
xfile_size_t xfile_tell(xfile_t * file);
xfat_err_t xfile_seek(xfile_t * file, xfile_ssize_t offset, xfile_orgin_t origin);

xfat_err_t xfile_size(xfile_t * file, xfile_size_t * size);

xfat_err_t xfile_rename(xfat_t * xfat, const char * path, const char * new_name);
xfat_err_t xfile_set_atime (xfat_t * xfat, const char * path, xfile_time_t * time);
xfat_err_t xfile_set_mtime (xfat_t * xfat, const char * path, xfile_time_t * time);
xfat_err_t xfile_set_ctime (xfat_t * xfat, const char * path, xfile_time_t * time);

#endif /* XFAT_H */
