/**
 * ��Դ�����׵Ŀγ�Ϊ - ��0��1����дFAT32�ļ�ϵͳ��ÿ�����̶�Ӧһ����ʱ��������ע�͡�
 * ���ߣ�����ͭ
 * �γ���ַ��http://01ketang.cc
 * ��Ȩ��������Դ��ǿ�Դ�����ο���������������ǰ����ϵ���ߡ�
 */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "xfat.h"
#include "xdisk.h"

extern u8_t temp_buffer[512];      // todo: �����Ż�

// ���õ�.��..�ļ���             "12345678ext"
#define DOT_FILE                ".          "
#define DOT_DOT_FILE            "..         "

#define is_path_sep(ch)         (((ch) == '\\') || ((ch == '/')))       // �ж��Ƿ����ļ����ָ���
#define is_path_end(path)       (((path) == 0) || (*path == '\0'))      // �ж�·���Ƿ�Ϊ��
#define file_get_disk(file)     ((file)->xfat->disk_part->disk)         // ��ȡdisk�ṹ
#define xfat_get_disk(xfat)     ((xfat)->disk_part->disk)               // ��ȡdisk�ṹ
#define to_sector(disk, offset)     ((offset) / (disk)->sector_size)    // ����ϡת��Ϊ������
#define to_sector_offset(disk, offset)   ((offset) % (disk)->sector_size)   // ��ȡ�����е����ƫ��
#define to_sector_addr(disk, offset)    ((offset) / (disk)->sector_size * (disk)->sector_size)  // ȡOffset����������ʼ��ַ
#define to_cluster_offset(xfat, pos)      ((pos) % ((xfat)->cluster_byte_size)) // ��ȡ���е����ƫ��
#define to_cluster(xfat, pos)		((pos) / (xfat)->cluster_byte_size)
#define	to_cluseter_count(xfat, size)		(size ? to_cluster(xfat, size) + 1 : 0)


/**
 * ���غźʹ�ƫ��ת��Ϊ������
 * @param xfat
 * @param cluster
 * @param cluster_offset
 * @return
 */
u32_t to_phy_sector(xfat_t* xfat, u32_t cluster, u32_t cluster_offset) {
    xdisk_t* disk = xfat_get_disk(xfat);
    return cluster_fist_sector((xfat), (cluster)) + to_sector((disk), (cluster_offset));
}

/**
 * ��dbr�н�����fat������ò���
 * @param dbr ��ȡ���豸dbr
 * @return
 */
static xfat_err_t parse_fat_header (xfat_t * xfat, dbr_t * dbr) {
    xdisk_part_t * xdisk_part = xfat->disk_part;

    // ����DBR���������������õĲ���
    xfat->root_cluster = dbr->fat32.BPB_RootClus;
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

    xfat->sec_per_cluster = dbr->bpb.BPB_SecPerClus;
    xfat->total_sectors = dbr->bpb.BPB_TotSec32;
    xfat->cluster_byte_size = xfat->sec_per_cluster * dbr->bpb.BPB_BytsPerSec;

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

    // ��һ����ȫ����ȡFAT��: todo: �Ż�
    xfat->fat_buffer = (u8_t *)malloc(xfat->fat_tbl_sectors * xdisk->sector_size);
    err = xdisk_read_sector(xdisk, (u8_t *)xfat->fat_buffer, xfat->fat_start_sector, xfat->fat_tbl_sectors);
    if (err < 0) {
        return err;
    }

    return FS_ERR_OK;
}

/**
 * ��ȡָ���غŵĵ�һ���������
 * @param xfat xfat�ṹ
 * @param cluster_no  �غ�
 * @return ������
 */
u32_t cluster_fist_sector(xfat_t *xfat, u32_t cluster_no) {
    u32_t data_start_sector = xfat->fat_start_sector + xfat->fat_tbl_sectors * xfat->fat_tbl_nr;
    return data_start_sector + (cluster_no - 2) * xfat->sec_per_cluster;    // ǰ�����غű���
}

/**
 * ���ָ�����Ƿ���ã���ռ�û򻵴�
 * @param cluster �����Ĵ�
 * @return
 */
int is_cluster_valid(u32_t cluster) {
    cluster &= 0x0FFFFFFF;
    return (cluster < 0x0FFFFFF0) && (cluster >= 0x2);     // ֵ�Ƿ���ȷ
}

/**
 * ��ȡָ���ص���һ����
 * @param xfat xfat�ṹ
 * @param curr_cluster_no
 * @param next_cluster
 * @return
 */
xfat_err_t get_next_cluster(xfat_t * xfat, u32_t curr_cluster_no, u32_t * next_cluster) {
    if (is_cluster_valid(curr_cluster_no)) {
        cluster32_t * cluster32_buf = (cluster32_t *)xfat->fat_buffer;
        *next_cluster = cluster32_buf[curr_cluster_no].s.next;
    } else {
        *next_cluster = CLUSTER_INVALID;
    }

    return FS_ERR_OK;
}

/**
 * ���ָ���ص������������ݣ�������0
 * @param xfat
 * @param cluster
 * @param erase_state ����״̬���ֽ�ֵ
 * @return
 */
static xfat_err_t erase_cluster(xfat_t * xfat, u32_t cluster, u8_t erase_state) {
    u32_t i;
    u32_t sector = cluster_fist_sector(xfat, cluster);
    xdisk_t * xdisk = xfat_get_disk(xfat);

    // todo: �Ż���һ�οɷ�����������
    memset(temp_buffer, erase_state, xdisk->sector_size);
    for (i = 0; i < xfat->sec_per_cluster; i++) {
        xfat_err_t err = xdisk_write_sector(xdisk, temp_buffer, sector + i, 1);
        if (err < 0) {
            return err;
        }
    }
    return FS_ERR_OK;
}

/**
 * ������д�
 * @param xfat xfat�ṹ
 * @param curr_cluster ��ǰ�غ�
 * @param count Ҫ����Ĵغ�
 * @param start_cluster ����ĵ�һ�����ôغ�
 * @param r_allocated_count ��Ч���������
 * @param erase_cluster �Ƿ�ͬʱ�����ض�Ӧ��������
 * @return
 */
static xfat_err_t allocate_free_cluster(xfat_t * xfat, u32_t curr_cluster, u32_t count,
        u32_t * r_start_cluster, u32_t * r_allocated_count, u8_t en_erase, u8_t erase_data) {
    u32_t i;
    xfat_err_t err;
    xdisk_t * disk = xfat_get_disk(xfat);
    u32_t allocated_count = 0;
    u32_t start_cluster = 0;

    // todo:Ŀǰ���������ͷ��ʼ����, ����FAT32
    u32_t cluster_count = xfat->fat_tbl_sectors * disk->sector_size / sizeof(cluster32_t);
    u32_t pre_cluster = curr_cluster;
    cluster32_t * cluster32_buf = (cluster32_t *)xfat->fat_buffer;

    for (i = 2; (i < cluster_count) && (allocated_count < count); i++) {
        if (cluster32_buf[i].s.next == 0) {     // ע����fat32��4�ֽڴ�С
            if (is_cluster_valid(pre_cluster)) {
                cluster32_buf[pre_cluster].s.next = i;
            }

            pre_cluster = i;

            if (++allocated_count == 1) {
                start_cluster = i;
            }

            if (allocated_count >= count) {
                break;
            }
        }
    }

    if (allocated_count) {
        cluster32_buf[pre_cluster].s.next = CLUSTER_INVALID;

        // ������Ժ�Ҫע����մ���ԭ������
        if (en_erase) {
            // ����������еĴ�
            u32_t cluster = start_cluster;
            for (i = 0; i < allocated_count; i++) {
                err = erase_cluster(xfat, cluster, erase_data);
                if (err < 0) {
                    return err;
                }

                cluster = cluster32_buf[cluster].s.next;
            }
        }

        // FAT��������ж����ͬʱ����
        for (i = 0; i < xfat->fat_tbl_nr; i++) {
            u32_t start_sector = xfat->fat_start_sector + xfat->fat_tbl_sectors * i;
            err = xdisk_write_sector(disk, (u8_t *)xfat->fat_buffer, start_sector, xfat->fat_tbl_sectors);
            if (err < 0) {
                return err;
            }
        }
    }

    if (r_allocated_count) {
        *r_allocated_count = allocated_count;
    }

    if (r_start_cluster) {
        *r_start_cluster = start_cluster;
    }


    return FS_ERR_OK;
}

/**
 * ��ȡһ���ص����ݵ�ָ��������
 * @param xfat xfat�ṹ
 * @param buffer ���ݴ洢�Ļ�����
 * @param cluster ��ȡ����ʼ�غ�
 * @param count ��ȡ�Ĵ�����
 * @return
 */
xfat_err_t read_cluster(xfat_t *xfat, u8_t *buffer, u32_t cluster, u32_t count) {
    xfat_err_t err = 0;
    u32_t i = 0;
    u8_t * curr_buffer = buffer;
    u32_t curr_sector = cluster_fist_sector(xfat, cluster);

    for (i = 0; i < count; i++) {
        err = xdisk_read_sector(xfat_get_disk(xfat), curr_buffer, curr_sector, xfat->sec_per_cluster);
        if (err < 0) {
            return err;
        }

        curr_buffer += xfat->cluster_byte_size;
        curr_sector += xfat->sec_per_cluster;
    }

    return FS_ERR_OK;
}

/**
 * ����ص����ӹ�ϵ
 * @param xfat xfat�ṹ
 * @param cluster ���ô�֮��������������ν��, �����ô�֮����Ϊ����
 * @return
 */
static xfat_err_t destroy_cluster_chain(xfat_t *xfat, u32_t cluster) {
    xfat_err_t err = FS_ERR_OK;
    u32_t i, write_back = 0;
    xdisk_t * disk = xfat_get_disk(xfat);
    u32_t curr_cluster = cluster;

    // ���ڻ������н�����ӹ�ϵ
    while (is_cluster_valid(curr_cluster)) {
        u32_t next_cluster;
        cluster32_t * cluster32_buf;

        // �Ȼ�ȡһ�´�
        err = get_next_cluster(xfat, curr_cluster, &next_cluster);
        if (err < 0) {
            return err;
        }

        // ��Ǹô�Ϊ����״̬
        cluster32_buf = (cluster32_t *)xfat->fat_buffer;
        cluster32_buf[curr_cluster].s.next = CLUSTER_FREE;

        curr_cluster = next_cluster;
        write_back = 1;
    }

    if (write_back) {
        for (i = 0; i < xfat->fat_tbl_nr; i++) {
            u32_t start_sector = xfat->fat_start_sector + xfat->fat_tbl_sectors * i;
            err = xdisk_write_sector(disk, (u8_t *)xfat->fat_buffer, start_sector, xfat->fat_tbl_sectors);
            if (err < 0) {
                return err;
            }
        }
    }

    // todo: �Ż�������Ҫȫ����д
    return err;
}

/**
 * ��ָ����name��FAT 8+3����ת��
 * @param dest_name
 * @param my_name
 * @return
 */
static xfat_err_t to_sfn(char* dest_name, const char* my_name) {
    int i, name_len;
    char * dest = dest_name;
    const char * ext_dot;
    const char * p;
    int ext_existed;

    memset(dest, ' ', SFN_LEN);

    // ������ͷ�ķָ���
    while (is_path_sep(*my_name)) {
        my_name++;
    }

    // �ҵ���һ��б��֮ǰ���ַ�������ext_dot��λ������Ҽ�¼��Ч����
    ext_dot = my_name;
    p = my_name;
    name_len = 0;
    while ((*p != '\0') && !is_path_sep(*p)) {
        if (*p == '.') {
            ext_dot = p;
        }
        p++;
        name_len++;
    }

    // ����ļ�����.��β����˼����û����չ����
    // todo: ���ļ�������?
    ext_existed = (ext_dot > my_name) && (ext_dot < (my_name + name_len - 1));

    // �������ƣ���������ַ�, ����.�ָ������12�ֽڣ�������������ֻӦ��
    p = my_name;
    for (i = 0; (i < SFN_LEN) && (*p != '\0') && !is_path_sep(*p); i++) {
        if (ext_existed) {
            if (p == ext_dot) {
                dest = dest_name + 8;
                p++;
                i--;
                continue;
            }
            else if (p < ext_dot) {
                *dest++ = toupper(*p++);
            }
            else {
                *dest++ = toupper(*p++);
            }
        }
        else {
            *dest++ = toupper(*p++);
        }
    }
    return FS_ERR_OK;
}


/**
 * ���sfn�ַ������Ƿ��Ǵ�д������м�������Сд������Ϊ��Сд
 * @param name
 * @return
 */
static u8_t get_sfn_case_cfg(const char * sfn_name) {
    u8_t case_cfg = 0;

    int name_len;
    const char * src_name = sfn_name;
    const char * ext_dot;
    const char * p;
    int ext_existed;

    // ������ͷ�ķָ���
    while (is_path_sep(*src_name)) {
        src_name++;
    }

    // �ҵ���һ��б��֮ǰ���ַ�������ext_dot��λ������Ҽ�¼��Ч����
    ext_dot = src_name;
    p = src_name;
    name_len = 0;
    while ((*p != '\0') && !is_path_sep(*p)) {
        if (*p == '.') {
            ext_dot = p;
        }
        p++;
        name_len++;
    }

    // ����ļ�����.��β����˼����û����չ����
    // todo: ���ļ�������?
    ext_existed = (ext_dot > src_name) && (ext_dot < (src_name + name_len - 1));
    for (p = src_name; p < src_name + name_len; p++) {
        if (ext_existed) {
            if (p < ext_dot) { // �ļ������岿�ִ�Сд�ж�
                case_cfg |= islower(*p) ? DIRITEM_NTRES_BODY_LOWER : 0;
            } else if (p > ext_dot) {
                case_cfg |= islower(*p) ? DIRITEM_NTRES_EXT_LOWER : 0;
            }
        } else {
            case_cfg |= islower(*p) ? DIRITEM_NTRES_BODY_LOWER : 0;
        }
    }

    return case_cfg;
}

/**
 * �ж������ļ����Ƿ�ƥ��
 * @param name_in_item fatdir�е��ļ�����ʽ
 * @param my_name Ӧ�ÿɶ����ļ�����ʽ
 * @return
 */
static u8_t is_filename_match(const char *name_in_dir, const char *to_find_name) {
    char temp_name[SFN_LEN];

    // FAT�ļ����ıȽϼ��ȣ�ȫ��ת���ɴ�д�Ƚ�
    // ����Ŀ¼�Ĵ�Сд���ã�����ת����8+3���ƣ��ٽ������ֽڱȽ�
    // ��ʵ����ʾʱ�������diritem->NTRes���д�Сдת��
    to_sfn(temp_name, to_find_name);
    return memcmp(temp_name, name_in_dir, SFN_LEN) == 0;
}

/**
 * ������ͷ�ķָ���
 * @param path Ŀ��·��
 * @return
 */
static const char * skip_first_path_sep (const char * path) {
    const char * c = path;

    // ������ͷ�ķָ���
    while (is_path_sep(*c)) {
        c++;
    }
    return c;
}

/**
 * ��ȡ��·��
 * @param dir_path ��һ��·��
 * @return
 */
const char * get_child_path(const char *dir_path) {
    const char * c = skip_first_path_sep(dir_path);

    // ������Ŀ¼
    while ((*c != '\0') && !is_path_sep(*c)) {
        c++;
    }

    return (*c == '\0') ? (const char *)0 : c + 1;
}

/**
 * ����diritem����ȡ�ļ�����
 * @param diritem �������diritem
 * @return
 */
static xfile_type_t get_file_type(const diritem_t *diritem) {
    xfile_type_t type;

    if (diritem->DIR_Attr & DIRITEM_ATTR_VOLUME_ID) {
        type = FAT_VOL;
    } else if (diritem->DIR_Attr & DIRITEM_ATTR_DIRECTORY) {
        type = FAT_DIR;
    } else {
        type = FAT_FILE;
    }

    return type;
}

/**
 * ������Ӧ��ʱ����Ϣ��dest��
 * @param dest ָ���洢��ʱ����Ϣ�ṹ
 * @param date fat��ʽ������
 * @param time fat��ʽ��ʱ��
 * @param mil_sec fat��ʽ��10����
 */
static void copy_date_time(xfile_time_t *dest, const diritem_date_t *date,
                           const diritem_time_t *time, const u8_t mil_sec) {
    if (date) {
        dest->year = (u16_t)(date->year_from_1980 + 1980);
        dest->month = (u8_t)date->month;
        dest->day = (u8_t)date->day;
    } else {
        dest->year = 0;
        dest->month = 0;
        dest->day = 0;
    }

    if (time) {
        dest->hour = (u8_t)time->hour;
        dest->minute = (u8_t)time->minute;
        dest->second = (u8_t)(time->second_2 * 2 + mil_sec / 100);
    } else {
        dest->hour = 0;
        dest->minute = 0;
        dest->second = 0;
    }
}

/**
 * ��fat_dir��ʽ���ļ����п������û��ɶ����ļ�����dest_name
 * @param dest_name ת������ļ����洢������
 * @param raw_name fat_dir��ʽ���ļ���
 */
static void sfn_to_myname(char *dest_name, const diritem_t * diritem) {
    int i;
    char * dest = dest_name, * raw_name = (char *)diritem->DIR_Name;
    u8_t ext_exist = raw_name[8] != 0x20;
    u8_t scan_len = ext_exist ? SFN_LEN + 1 : SFN_LEN;

    memset(dest_name, 0, X_FILEINFO_NAME_SIZE);

    // Ҫ���Ǵ�Сд���⣬����NTRes����ת������Ӧ�Ĵ�Сд
    for (i = 0; i < scan_len; i++) {
        if (*raw_name == ' ') {
            raw_name++;
        } else if ((i == 8) && ext_exist) {
           *dest++ = '.';
        } else {
            u8_t lower = 0;

            if (((i < 8) && (diritem->DIR_NTRes & DIRITEM_NTRES_BODY_LOWER))
                || ((i > 8) && (diritem->DIR_NTRes & DIRITEM_NTRES_EXT_LOWER))) {
                lower = 1;
            }

            *dest++ = lower ? tolower(*raw_name++) : toupper(*raw_name++);
        }
    }
    *dest = '\0';
}

/**
 * ����diritem��cluster
 * @param item Ŀ¼diritem
 * @param cluster �غ�
 */
static void set_diritem_cluster (diritem_t * item, u32_t cluster) {
    item->DIR_FstClusHI = (u16_t )(cluster >> 16);
    item->DIR_FstClusL0 = (u16_t )(cluster & 0xFFFF);
}

/**
 * ��ȡdiritem���ļ���ʼ�غ�
 * @param item
 * @return
 */
static u32_t get_diritem_cluster (diritem_t * item) {
    return (item->DIR_FstClusHI << 16) | item->DIR_FstClusL0;
}

/**
 * �ƶ��ص�λ��
 * @param xfat
 * @param curr_cluster ��ǰ�غ�
 * @param curr_offset ��ǰ��ƫ��
 * @param move_bytes �ƶ����ֽ�������ǰֻ֧�ֱ��ؼ����ڴ��ڵ��ƶ�)
 * @param next_cluster �ƶ���Ĵغ�
 * @param next_offset �ƶ���Ĵ�ƫ��
 * @return
 */
xfat_err_t move_cluster_pos(xfat_t* xfat, u32_t curr_cluster, u32_t curr_offset, u32_t move_bytes,
    u32_t* next_cluster, u32_t* next_offset) {
    if ((curr_offset + move_bytes) >= xfat->cluster_byte_size) {
        xfat_err_t err = get_next_cluster(xfat, curr_cluster, next_cluster);
        if (err < 0) {
            return err;
        }

        *next_offset = 0;
    }
    else {
        *next_cluster = curr_cluster;
        *next_offset = curr_offset + move_bytes;
    }

    return FS_ERR_OK;
}

/**
 * ��ȡ��һ����Ч��Ŀ¼��
 * @param xfat
 * @param curr_cluster ��ǰĿ¼���Ӧ�Ĵغ�
 * @param curr_offset  ��ǰĿ¼���Ӧ��ƫ��
 * @param next_cluster ��һĿ¼���Ӧ�Ĵغ�
 * @param next_offset  ��ǰĿ¼���Ӧ�Ĵ�ƫ��
 * @param temp_buffer �ش洢�Ļ�����
 * @param diritem ��һ����Ч��Ŀ¼��
 * @return
 */
xfat_err_t get_next_diritem(xfat_t* xfat, u8_t type, u32_t start_cluster, u32_t start_offset,
    u32_t* found_cluster, u32_t* found_offset, u32_t* next_cluster, u32_t* next_offset,
    u8_t* temp_buffer, diritem_t** diritem) {
    xfat_err_t err;
    diritem_t* r_diritem;

    while (is_cluster_valid(start_cluster)) {
        u32_t sector_offset;
        u32_t curr_sector;

        // Ԥ��ȡ��һλ�ã������������
        err = move_cluster_pos(xfat, start_cluster, start_offset, sizeof(diritem_t), next_cluster, next_offset);
        if (err < 0) {
            return err;
        }

        sector_offset = to_sector_offset(xfat_get_disk(xfat), start_offset);
        curr_sector = to_phy_sector(xfat, start_cluster, start_offset);

        // ע�⣡���������޸ģ��γ���Ƶ��û���޸ġ�
        // ������޸ģ�������ɾ��Ŀ¼�����������⡣��ǰЧ�ʻ��Щ�������Ժ���Ż�
        err = xdisk_read_sector(xfat_get_disk(xfat), temp_buffer, curr_sector, 1);
        if (err < 0) return err;

        r_diritem = (diritem_t*)(temp_buffer + sector_offset);
        switch (r_diritem->DIR_Name[0]) {
        case DIRITEM_NAME_END:
            if (type & DIRITEM_GET_END) {
                *diritem = r_diritem;
                *found_cluster = start_cluster;
                *found_offset = start_offset;
                return FS_ERR_OK;
            }
            break;
        case DIRITEM_NAME_FREE:
            if (type & DIRITEM_GET_FREE) {
                *diritem = r_diritem;
                *found_cluster = start_cluster;
                *found_offset = start_offset;
                return FS_ERR_OK;
            }
            break;
        default:
            if (type & DIRITEM_GET_USED) {
                *diritem = r_diritem;
                *found_cluster = start_cluster;
                *found_offset = start_offset;
                return FS_ERR_OK;
            }
            break;
        }

        start_cluster = *next_cluster;
        start_offset = *next_offset;
    }

    *diritem = (diritem_t*)0;
    return FS_ERR_EOF;
}

/**
 * ��dir_item����Ӧ���ļ���Ϣת������fs_fileinfo_t��
 * @param info ��Ϣ�洢��λ��
 * @param dir_item fat��diritem
 */
static void copy_file_info(xfileinfo_t *info, const diritem_t * dir_item) {
    sfn_to_myname(info->file_name, dir_item);
    info->size = dir_item->DIR_FileSize;
    info->attr = dir_item->DIR_Attr;
    info->type = get_file_type(dir_item);

    copy_date_time(&info->create_time, &dir_item->DIR_CrtDate, &dir_item->DIR_CrtTime, dir_item->DIR_CrtTimeTeenth);
    copy_date_time(&info->last_acctime, &dir_item->DIR_LastAccDate, (diritem_time_t *) 0, 0) ;
    copy_date_time(&info->modify_time, &dir_item->DIR_WrtDate, &dir_item->DIR_WrtTime, 0);
}

/**
 * ����ļ����������Ƿ�ƥ��
 * @param dir_item
 * @param locate_type
 * @return
 */
static u8_t is_locate_type_match (diritem_t * dir_item, u8_t locate_type) {
    u8_t match = 1;

    if ((dir_item->DIR_Attr & DIRITEM_ATTR_SYSTEM) && !(locate_type & XFILE_LOCALE_SYSTEM)) {
        match = 0;  // ����ʾϵͳ�ļ�
    } else if ((dir_item->DIR_Attr & DIRITEM_ATTR_HIDDEN) && !(locate_type & XFILE_LOCATE_HIDDEN)) {
        match = 0;  // ����ʾ�����ļ�
    } else if ((dir_item->DIR_Attr & DIRITEM_ATTR_VOLUME_ID) && !(locate_type & XFILE_LOCATE_VOL)) {
        match = 0;  // ����ʾ���
    } else if ((memcmp(DOT_FILE, dir_item->DIR_Name, SFN_LEN) == 0)
                || (memcmp(DOT_DOT_FILE, dir_item->DIR_Name, SFN_LEN) == 0)) {
        if (!(locate_type & XFILE_LOCATE_DOT)) {
            match = 0;// ����ʾdot�ļ�
        }
    } else if (!(locate_type & XFILE_LOCATE_NORMAL)) {
        match = 0;
    }
    return match;
}

/**
 * ����ָ��dir_item����������Ӧ�Ľṹ
 * @param xfat xfat�ṹ
 * @param locate_type ��λ��item����
 * @param dir_cluster dir_item���ڵ�Ŀ¼���ݴغ�
 * @param cluster_offset ���е�ƫ��
 * @param move_bytes ���ҵ���Ӧ��item���������ʼ�����ƫ��ֵ���ƶ��˶��ٸ��ֽڲŶ�λ����item
 * @param path �ļ���Ŀ¼������·��
 * @param r_diritem ���ҵ���diritem��
 * @return
 */
static xfat_err_t locate_file_dir_item(xfat_t *xfat, u8_t locate_type, u32_t *dir_cluster, u32_t *cluster_offset,
                                    const char *path, u32_t *move_bytes, diritem_t **r_diritem) {
    u32_t curr_cluster = *dir_cluster;
    xdisk_t * xdisk = xfat_get_disk(xfat);
    u32_t initial_sector = to_sector(xdisk, *cluster_offset);
    u32_t initial_offset = to_sector_offset(xdisk, *cluster_offset);
    u32_t r_move_bytes = 0;

    // cluster
    do {
        u32_t i;
        xfat_err_t err;
        u32_t start_sector = cluster_fist_sector(xfat, curr_cluster);

        for (i = initial_sector; i < xfat->sec_per_cluster; i++) {
            u32_t j;

            err = xdisk_read_sector(xdisk, temp_buffer, start_sector + i, 1);
            if (err < 0) {
                return err;
            }

            for (j = initial_offset / sizeof(diritem_t); j < xdisk->sector_size / sizeof(diritem_t); j++) {
                diritem_t *dir_item = ((diritem_t *) temp_buffer) + j;

                if (dir_item->DIR_Name[0] == DIRITEM_NAME_END) {
                    return FS_ERR_EOF;
                } else if (dir_item->DIR_Name[0] == DIRITEM_NAME_FREE) {
                    r_move_bytes += sizeof(diritem_t);
                    continue;
                } else if (!is_locate_type_match(dir_item, locate_type)) {
                    r_move_bytes += sizeof(diritem_t);
                    continue;
                }

                if ((path == (const char *) 0)
                    || (*path == 0)
                    || is_filename_match((const char *) dir_item->DIR_Name, path)) {

                    u32_t total_offset = i * xdisk->sector_size + j * sizeof(diritem_t);
                    *dir_cluster = curr_cluster;
                    *move_bytes = r_move_bytes + sizeof(diritem_t);
                    *cluster_offset = total_offset;
                    if (r_diritem) {
                        *r_diritem = dir_item;
                    }

                    return FS_ERR_OK;
                }

                r_move_bytes += sizeof(diritem_t);
            }
        }

        err = get_next_cluster(xfat, curr_cluster, &curr_cluster);
        if (err < 0) {
            return err;
        }

        initial_sector = 0;
        initial_offset = 0;
    }while (is_cluster_valid(curr_cluster));

    return FS_ERR_EOF;
}

/**
 * ��ָ��dir_cluster��ʼ�Ĵ����а��������ļ���
 * ���pathΪ�գ�����dir_cluster����һ���򿪵�Ŀ¼����
 * @param xfat xfat�ṹ
 * @param dir_cluster ���ҵĶ���Ŀ¼����ʼ����
 * @param file �򿪵��ļ�file�ṹ
 * @param path ��dir_cluster����Ӧ��Ŀ¼Ϊ��������·��
 * @return
 */
static xfat_err_t open_sub_file (xfat_t * xfat, u32_t dir_cluster, xfile_t * file, const char * path) {
    u32_t parent_cluster = dir_cluster;
    u32_t parent_cluster_offset = 0;

    path = skip_first_path_sep(path);

    // �������·����Ϊ�գ���鿴��Ŀ¼
    // ����ֱ����Ϊdir_clusterָ�����һ��Ŀ¼�����ڴ򿪸�Ŀ¼
    if ((path != 0) && (*path != '\0')) {
        diritem_t * dir_item = (diritem_t *)0;
        u32_t file_start_cluster = 0;
        const char * curr_path = path;

       // �ҵ�path��Ӧ����ʼ��
        while (curr_path != (const char *)0) {
            u32_t moved_bytes = 0;
            dir_item = (diritem_t *)0;

            // �ڸ�Ŀ¼�²���ָ��·����Ӧ���ļ�
            xfat_err_t err = locate_file_dir_item(xfat, XFILE_LOCATE_DOT | XFILE_LOCATE_NORMAL,
                    &parent_cluster, &parent_cluster_offset,curr_path, &moved_bytes, &dir_item);
            if (err < 0) {
                return err;
            }

            if (dir_item == (diritem_t *)0) {
                return FS_ERR_NONE;
            }

            curr_path = get_child_path(curr_path);
            if (curr_path != (const char *)0) {
                parent_cluster = get_diritem_cluster(dir_item);
                parent_cluster_offset = 0;
            } else {
                file_start_cluster = get_diritem_cluster(dir_item);

                // �����..�Ҷ�Ӧ��Ŀ¼����clusterֵΪ0���������ȷ��ֵ
                if ((memcmp(dir_item->DIR_Name, DOT_DOT_FILE, SFN_LEN) == 0) && (file_start_cluster == 0)) {
                    file_start_cluster = xfat->root_cluster;
                }
            }
        }

        file->size = dir_item->DIR_FileSize;
        file->type = get_file_type(dir_item);
        file->attr = (dir_item->DIR_Attr & DIRITEM_ATTR_READ_ONLY) ? XFILE_ATTR_READONLY : 0;
        file->start_cluster = file_start_cluster;
        file->curr_cluster = file_start_cluster;
        file->dir_cluster = parent_cluster;
        file->dir_cluster_offset = parent_cluster_offset;
    } else {
        file->size = 0;
        file->type = FAT_DIR;
        file->attr = 0;
        file->start_cluster = parent_cluster;
        file->curr_cluster = parent_cluster;
        file->dir_cluster = CLUSTER_INVALID;
        file->dir_cluster_offset = 0;
    }

    file->xfat = xfat;
    file->pos = 0;
    file->err = FS_ERR_OK;
    return FS_ERR_OK;
}

/**
 * ��ָ�����ļ���Ŀ¼
 * @param xfat xfat�ṹ
 * @param file �򿪵��ļ���Ŀ¼
 * @param path �ļ���Ŀ¼���ڵ�����·�����ݲ�֧�����·��
 * @return
 */
xfat_err_t xfile_open(xfat_t * xfat, xfile_t * file, const char * path) {
	path = skip_first_path_sep(path);

	// ��Ŀ¼�������ϼ�Ŀ¼
	// ������.��ֱ�ӹ��˵�·��
	if (memcmp(path, "..", 2) == 0) {
		return FS_ERR_NONE;
	} else if (memcmp(path, ".", 1) == 0) {
		path++;
	}

    return open_sub_file(xfat, xfat->root_cluster, file, path);
}

/**
 * �ڴ򿪵�Ŀ¼�£�����Ӧ�����ļ���Ŀ¼
 * @param dir  �Ѿ��򿪵�Ŀ¼
 * @param sub_file �򿪵����ļ���Ŀ¼
 * @param sub_path ���Ѵ򿪵�Ŀ¼Ϊ��㣬���ļ���Ŀ¼������·��
 * @return
 */
xfat_err_t xfile_open_sub(xfile_t* dir, const char* sub_path, xfile_t* sub_file) {
    if (dir->type != FAT_DIR) {
        return FS_ERR_PARAM;
    }

    if (memcmp(sub_path, ".", 1) == 0) {
        return FS_ERR_PARAM;
    }

    return open_sub_file(dir->xfat, dir->start_cluster, sub_file, sub_path);
}

/**
 * ����ָ��Ŀ¼�µĵ�һ���ļ���Ϣ
 * @param file �Ѿ��򿪵��ļ�
 * @param info ��һ���ļ����ļ���Ϣ
 * @return
 */
xfat_err_t xdir_first_file (xfile_t * file, xfileinfo_t * info) {
    diritem_t * diritem = (diritem_t *)0;
    xfat_err_t err;
    u32_t moved_bytes = 0;
    u32_t cluster_offset;

    // ��������Ŀ¼������
    if (file->type != FAT_DIR) {
        return FS_ERR_PARAM;
    }

    // ���µ�������λ��
    file->curr_cluster = file->start_cluster;
    file->pos = 0;

    cluster_offset = 0;
    err = locate_file_dir_item(file->xfat, XFILE_LOCATE_NORMAL,
            &file->curr_cluster, &cluster_offset, "", &moved_bytes, &diritem);
    if (err < 0) {
        return err;
    }

    if (diritem == (diritem_t *)0) {
        return FS_ERR_EOF;
    }

    file->pos += moved_bytes;

    // �ҵ��󣬿����ļ���Ϣ
    copy_file_info(info, diritem);
    return err;
}

/**
 * ����ָ��Ŀ¼���������ļ��������ļ�����)
 * @param file �Ѿ��򿪵�Ŀ¼
 * @param info ��õ��ļ���Ϣ
 * @return
 */
xfat_err_t xdir_next_file (xfile_t * file, xfileinfo_t * info) {
    xfat_err_t err;
    diritem_t * dir_item = (diritem_t *)0;
    u32_t moved_bytes = 0;
    u32_t cluster_offset;

    // ������Ŀ¼
    if (file->type != FAT_DIR) {
        return FS_ERR_PARAM;
    }

    // �����ļ���Ŀ¼
    cluster_offset = to_cluster_offset(file->xfat, file->pos);
    err = locate_file_dir_item(file->xfat, XFILE_LOCATE_NORMAL,
            &file->curr_cluster, &cluster_offset, "", &moved_bytes, &dir_item);
    if (err != FS_ERR_OK) {
        return err;
    }

    if (dir_item == (diritem_t *)0) {
        return FS_ERR_EOF;
    }

    file->pos += moved_bytes;

    // �ƶ�λ�ú󣬿��ܳ�����ǰ�أ����µ�ǰ��λ��
    if (cluster_offset + sizeof(diritem_t) >= file->xfat->cluster_byte_size) {
        err = get_next_cluster(file->xfat, file->curr_cluster, &file->curr_cluster);
        if (err < 0) {
            return err;
        }
    }

    copy_file_info(info, dir_item);
    return err;
}

/**
 * ��ȡ�ļ���д�Ĵ�����
 * @param file
 * @return
 */
xfat_err_t xfile_error(xfile_t * file) {
    return file->err;
}

/**
 * ����ļ���д����״̬��
 * @param file
 */
void xfile_clear_err(xfile_t * file) {
    file->err = FS_ERR_OK;
}

/**
 * ȱʡ��ʼ��driitem
 * @param dir_item ����ʼ����diritem
 * @param is_dir �����Ƿ��ӦĿ¼��
 * @param name �������
 * @param cluster ���ݴص���ʼ�غ�
 * @return
 */
static xfat_err_t diritem_init_default(diritem_t * dir_item, xdisk_t * disk, u8_t is_dir, const char * name, u32_t cluster) {
    xfile_time_t timeinfo;

    xfat_err_t err = xdisk_curr_time(disk, &timeinfo);
    if (err < 0) {
        return err;
    }

    to_sfn((char *)dir_item->DIR_Name, name);
    set_diritem_cluster(dir_item, cluster);
    dir_item->DIR_FileSize = 0;
    dir_item->DIR_Attr = (u8_t)(is_dir ? DIRITEM_ATTR_DIRECTORY : 0);
    dir_item->DIR_NTRes = get_sfn_case_cfg(name);

    dir_item->DIR_CrtTime.hour = timeinfo.hour;
    dir_item->DIR_CrtTime.minute = timeinfo.minute;
    dir_item->DIR_CrtTime.second_2 = (u16_t)(timeinfo.second / 2);
    dir_item->DIR_CrtTimeTeenth = (u8_t)((timeinfo.second & 1) * 1000);

    dir_item->DIR_CrtDate.year_from_1980 = (u16_t)(timeinfo.year - 1980);
    dir_item->DIR_CrtDate.month = timeinfo.month;
    dir_item->DIR_CrtDate.day = timeinfo.day;

    dir_item->DIR_WrtTime = dir_item->DIR_CrtTime;
    dir_item->DIR_WrtDate = dir_item->DIR_CrtDate;
    dir_item->DIR_LastAccDate = dir_item->DIR_CrtDate;

    return FS_ERR_OK;
}

/**
 * ��ָ��Ŀ¼�´������ļ�����Ŀ¼
 * @param xfat xfat�ṹ
 * @param is_dir Ҫ���������ļ�����Ŀ¼
 * @param parent_dir_cluster ��Ŀ¼��ʼ���ݴغ�
 * @param file_cluster Ԥ�ȸ������ļ���Ŀ¼����ʼ���ݴغš�����ļ���Ŀ¼�Ѿ����ڣ��򷵻���Ӧ�Ĵغ�
 * @param child_name ������Ŀ¼���ļ�����
 * @return
 */
static xfat_err_t create_sub_file (xfat_t * xfat, u8_t is_dir, u32_t parent_cluster,
                const char* child_name, u32_t * file_cluster) {
    xfat_err_t err;
    xdisk_t * disk = xfat_get_disk(xfat);
    diritem_t * target_item = (diritem_t *)0;
    u32_t curr_cluster = parent_cluster, curr_offset = 0;
    u32_t free_item_cluster = CLUSTER_INVALID, free_item_offset = 0;
    u32_t file_diritem_sector;
    u32_t found_cluster, found_offset;
    u32_t next_cluster, next_offset;
	u32_t file_first_cluster = FILE_DEFAULT_CLUSTER;

    // �����ҵ��������Ŀ¼ĩβ�������
    do {

        diritem_t* diritem = (diritem_t*)0;
        err = get_next_diritem(xfat, DIRITEM_GET_ALL, curr_cluster, curr_offset,
                                    &found_cluster, &found_offset, &next_cluster, &next_offset, temp_buffer, &diritem);
        if (err < 0) return err;

        if (diritem == (diritem_t*)0) {    // �Ѿ�������Ŀ¼����
            return FS_ERR_NONE;
        }

        if (diritem->DIR_Name[0] == DIRITEM_NAME_END) {        // ��Ч�������
            target_item = diritem;
            break;
        } else if (diritem->DIR_Name[0] == DIRITEM_NAME_FREE) {
            // ������, ��Ҫ������飬���Ƿ���ͬ����
            // ��¼�������λ��
            if (!is_cluster_valid(free_item_cluster)) {
                free_item_cluster = curr_cluster;
                free_item_offset = curr_offset;
            }
        } else if (is_filename_match((const char*)diritem->DIR_Name, child_name)) {
            // ��������ͬ����Ҫ����Ƿ���ͬ�����ļ���Ŀ¼
            int item_is_dir = diritem->DIR_Attr & DIRITEM_ATTR_DIRECTORY;
            if ((is_dir && item_is_dir) || (!is_dir && !item_is_dir)) { // ͬ������ͬ��
                *file_cluster = get_diritem_cluster(diritem);  // ����
                return FS_ERR_EXISTED;
            } else {    // ��ͬ���ͣ���Ŀ¼-�ļ�ͬ����ֱ�ӱ���
                return FS_ERR_NAME_USED;
            }
        }

        curr_cluster = next_cluster;
        curr_offset = next_offset;
    } while (1);

    // �����Ŀ¼�Ҳ�Ϊdot file�� Ԥ�ȷ���Ŀ¼��ռ�
    if (is_dir && strncmp(".", child_name, 1) && strncmp("..", child_name, 2)) {
        u32_t cluster_count;

        err = allocate_free_cluster(xfat, CLUSTER_INVALID, 1, &file_first_cluster, &cluster_count, 1, 0);
        if (err < 0) return err;

        if (cluster_count < 1) {
            return FS_ERR_DISK_FULL;
        }
	} else {
		file_first_cluster = *file_cluster;
	}

    // δ�ҵ����е����ҪΪ��Ŀ¼�����´أ��Է������ļ�/Ŀ¼
    if ((target_item == (diritem_t *)0) && !is_cluster_valid(free_item_cluster)) {
        u32_t parent_diritem_cluster;
        u32_t cluster_count;

        xfat_err_t err = allocate_free_cluster(xfat, found_cluster, 1, &parent_diritem_cluster, &cluster_count, 1, 0);
        if (err < 0)  return err;

        if (cluster_count < 1) {
            return FS_ERR_DISK_FULL;
        }

        // ��ȡ�½����еĵ�һ����������ȡtarget_item
        file_diritem_sector = cluster_fist_sector(xfat, parent_diritem_cluster);
        err = xdisk_read_sector(disk, temp_buffer, file_diritem_sector, 1);
        if (err < 0) {
            return err;
        }
        target_item = (diritem_t *)temp_buffer;     // ��ȡ�´���
    } else {    // �ҵ����л�ĩβ
        u32_t diritem_offset;
        if (is_cluster_valid(free_item_cluster)) {
            file_diritem_sector = cluster_fist_sector(xfat, free_item_cluster) + to_sector(disk, free_item_offset);
            diritem_offset = free_item_offset;
        } else {
            file_diritem_sector = cluster_fist_sector(xfat, found_cluster) + to_sector(disk, found_offset);
            diritem_offset = found_offset;
        }
        err = xdisk_read_sector(disk, temp_buffer, file_diritem_sector, 1);
        if (err < 0) {
            return err;
        }
        target_item = (diritem_t*)(temp_buffer + to_sector_offset(disk, diritem_offset));     // ��ȡ�´���
    }

    // ��ȡĿ¼��֮�󣬸����ļ���Ŀ¼������item
    err = diritem_init_default(target_item, disk, is_dir, child_name, file_first_cluster);
    if (err < 0) {
        return err;
    }

    // д������Ŀ¼��
    err = xdisk_write_sector(disk, temp_buffer, file_diritem_sector, 1);
    if (err < 0) {
        return err;
    }

    *file_cluster = file_first_cluster;
    return err;
}

/**
 * ����һ����Ŀ¼
 * @param xfat xfat�ṹ
 * @param parent_cluster ��Ŀ¼���ڵĸ�Ŀ¼����ʼ���ݴ�
 * @param fail_on_exist Ŀ¼�Ѿ�����ʱ���Ƿ���Ϊ��ʧ��
 * @param name ��Ŀ¼������
 * @param dir_cluster ������֮�󣬸���Ŀ¼��������ʼ�غ�
 * @return
 */
static xfat_err_t create_empty_dir (xfat_t * xfat, u8_t fail_on_exist, u32_t parent_cluster,
                                const char * name, u32_t * new_cluster) {
    u32_t dot_cluster;
    u32_t dot_dot_cluster;
    xfat_err_t err;

    // ���������ļ���ָ��Ŀ¼�����.��..��Ŀ¼
    err = create_sub_file(xfat, 1, parent_cluster, name, new_cluster);
    if ((err == FS_ERR_EXISTED) && !fail_on_exist) {
        return FS_ERR_OK;   // �����ļ��Ѵ��ڣ���ֱ���˳�
    } else if (err < 0) {
        return err;
    }

    // ���´�����Ŀ¼�´����ļ� . ����غ�Ϊ��ǰĿ¼�Ĵغ�
    dot_cluster = *new_cluster;
    err = create_sub_file(xfat, 1, *new_cluster, ".", &dot_cluster);
    if (err < 0) {
        return err;
    }

    // �����ļ� .. ����غ�Ϊ��Ŀ¼�Ĵغ�
    dot_dot_cluster = parent_cluster;
    err = create_sub_file(xfat, 1, *new_cluster, "..", &dot_dot_cluster);
    if (err < 0) {
        return err;
    }

    return FS_ERR_OK;
}

/**
 * ��ָ��·��������Ŀ¼, ���Ŀ¼�Ѿ����ڣ����ᱨ��
 * @param xfat
 * @param dir_path
 * @return
 */
xfat_err_t xfile_mkdir (xfat_t * xfat, const char * path) {
    u32_t parent_cluster;

    parent_cluster = xfat->root_cluster;

    // ���������ļ�·��, �𼶴�������Ŀ¼��
    while (!is_path_end(path)) {
        u32_t new_dir_cluster = FILE_DEFAULT_CLUSTER;
        const char * next_path = get_child_path(path);
        u8_t fail_on_exist = is_path_end(next_path);        // �м�Ŀ¼����������ʧ��

        xfat_err_t err = create_empty_dir(xfat, fail_on_exist, parent_cluster, path, &new_dir_cluster);
        if (err < 0) {
            return err;
        }

        path = get_child_path(path);
        parent_cluster = new_dir_cluster;
    }
    return FS_ERR_OK;
}

/**
 * ��ָ��·�������δ�������Ŀ¼����󴴽�ָ���ļ�
 * @param xfat xfat�ṹ
 * @param file_path �ļ�������·��
 * @return
 */
xfat_err_t xfile_mkfile (xfat_t * xfat, const char * path) {
    u32_t parent_cluster;

    // Ĭ�ϴӸ�Ŀ¼����
    parent_cluster = xfat->root_cluster;

    // �𼶴���Ŀ¼���ļ�
    while (!is_path_end(path)) {
        xfat_err_t err;
        u32_t file_cluster = FILE_DEFAULT_CLUSTER;
        const char * next_path = get_child_path(path);

        // û�к���·������ǰ�����ļ�
        if (is_path_end(next_path)) {
            err = create_sub_file(xfat, 0, parent_cluster, path, &file_cluster);
            return err;
        } else {
            // �ڴ˴���Ŀ¼, ���Ŀ¼�Ѿ����ڣ������
            err = create_empty_dir(xfat, 0, parent_cluster, path, &file_cluster);
            if (err < 0) {
                return err;
            }
            parent_cluster = file_cluster;
        }

        path = next_path;
    }
    return FS_ERR_OK;
}

static xfat_err_t move_file_pos(xfile_t* file, u32_t move_bytes) {
	u32_t to_move = move_bytes;
	u32_t cluster_offset;

	// ��Ҫ�����ļ��Ĵ�С
	if (file->pos + move_bytes >= file->size) {
		to_move = file->size - file->pos;
	}

	// �ؼ��ƶ���������Ҫ������
	cluster_offset = to_cluster_offset(file->xfat, file->pos);
	if (cluster_offset + to_move >= file->xfat->cluster_byte_size) {
		u32_t curr_cluster = file->curr_cluster;

		xfat_err_t err = get_next_cluster(file->xfat, curr_cluster, &curr_cluster);
		if (err != FS_ERR_OK) {
			file->err = err;
			return err;
		}

		if (is_cluster_valid(curr_cluster)) {
			file->curr_cluster = curr_cluster;
		}
	}

	file->pos += to_move;
	return FS_ERR_OK;
}

/**
 * ��ָ�����ļ��ж�ȡ��Ӧ������Ԫ������
 * @param buffer ���ݴ洢�Ļ�����
 * @param elem_size ÿ�ζ�ȡ��Ԫ���ֽڴ�С
 * @param count ��ȡ���ٸ�elem_size
 * @param file Ҫ��ȡ���ļ�
 * @return
 */
xfile_size_t xfile_read(void * buffer, xfile_size_t elem_size, xfile_size_t count, xfile_t * file) {
    xdisk_t * disk = file_get_disk(file);
    xfile_size_t r_count_readed = 0;
    xfile_size_t bytes_to_read = count * elem_size;
    u8_t * read_buffer = (u8_t *)buffer;

    // ֻ����ֱ�Ӷ���ͨ�ļ�
    if (file->type != FAT_FILE) {
        file->err = FS_ERR_FSTYPE;
        return 0;
    }

    // �Ѿ������ļ�βĩ������
    if (file->pos >= file->size) {
        file->err = FS_ERR_EOF;
        return 0;
    }

    // ������ȡ������Ҫ�����ļ�����
    if (file->pos + bytes_to_read > file->size) {
        bytes_to_read = file->size - file->pos;
    }


    while ((bytes_to_read > 0) && is_cluster_valid(file->curr_cluster)) {
        xfat_err_t err;
        xfile_size_t curr_read_bytes = 0;
        u32_t sector_count = 0;
		u32_t cluster_sector = to_sector(disk, to_cluster_offset(file->xfat, file->pos));  // ���е�������
		u32_t sector_offset = to_sector_offset(disk, file->pos);  // ����ƫ��λ��
		u32_t start_sector = cluster_fist_sector(file->xfat, file->curr_cluster) + cluster_sector;

        // ��ʼ�������߽����, ֻ��ȡ��ǰ����
        // ������ʼΪ0������ȡ����������ǰ������Ҳֻ��ȡ��ǰ����
        // �����������������Ҫ�ݴ浽�������У�Ȼ�󿽱����û�������
        if ((sector_offset != 0) || (!sector_offset && (bytes_to_read < disk->sector_size))) {
            sector_count = 1;
            curr_read_bytes = bytes_to_read;

            // ��ʼƫ�Ʒ�0�������������ֻ��ȡ��ǰ����
            if (sector_offset != 0) {
                if (sector_offset + bytes_to_read > disk->sector_size) {
                    curr_read_bytes = disk->sector_size - sector_offset;
                }
            }

            // ��ȡ��������Ȼ����м俽���������ݵ�Ӧ�û�������
            // todo: �������С������ʱ�����ܻ����¼���ͬһ����
            err = xdisk_read_sector(disk, temp_buffer, start_sector, 1);
            if (err < 0) {
                file->err = err;
                return 0;
            }

            memcpy(read_buffer, temp_buffer + sector_offset, curr_read_bytes);
            read_buffer += curr_read_bytes;
            bytes_to_read -= curr_read_bytes;
        } else {
            // ��ʼΪ0���Ҷ�ȡ������1��������������ȡ������
            sector_count = (u32_t)to_sector(disk, bytes_to_read);

            // �������һ�أ���ֻ��ȡ��ǰ��
            // todo: ����������Ż�һ�£�����������Ļ���ʵ���ǿ���������ص�
            if ((cluster_sector + sector_count) > file->xfat->sec_per_cluster) {
                sector_count = file->xfat->sec_per_cluster - cluster_sector;
            }

            err = xdisk_read_sector(disk, read_buffer, start_sector, sector_count);
            if (err != FS_ERR_OK) {
                file->err = err;
                return 0;
            }

            curr_read_bytes = sector_count * disk->sector_size;
            read_buffer += curr_read_bytes;
            bytes_to_read -= curr_read_bytes;
        }

        r_count_readed += curr_read_bytes;

		err = move_file_pos(file, curr_read_bytes);
		if (err) return 0;
	}

    file->err = file->size == file->pos;
    return r_count_readed / elem_size;
}

/**
 * �����Ѿ��򿪵��ļ���С
 * @param file �Ѿ��򿪵��ļ�
 * @param size �ļ���С
 * @return
 */
static xfat_err_t update_file_size(xfile_t * file, xfile_size_t size) {
    xfat_err_t err;
    diritem_t * dir_item;
    xdisk_t * disk = file_get_disk(file);
    u32_t sector = to_phy_sector(file->xfat, file->dir_cluster, file->dir_cluster_offset);
    u32_t offset = to_sector_offset(disk, file->dir_cluster_offset);

    // todo: ���Ż�
    err = xdisk_read_sector(disk, temp_buffer, sector, 1);
    if (err < 0) {
        file->err = err;
        return err;
    }

    dir_item = (diritem_t *)(temp_buffer + offset);
    dir_item->DIR_FileSize = size;

    // ע����´غţ����ʼ�ļ�����ʱ����Ĵؿ��ܲ�����Ч����Ҫ��������
    set_diritem_cluster(dir_item, file->start_cluster);
 
    err = xdisk_write_sector(disk, temp_buffer, sector, 1);
    if (err < 0) {
        file->err = err;
        return err;
    }

    file->size = size;
    return FS_ERR_OK;
}

/**
 * �ж��ļ���ǰָ���Ƿ��ǽ�β�ص�ĩ��
 */
static int is_fpos_cluster_end(xfile_t * file) {
    xfile_size_t cluster_offset = to_cluster_offset(file->xfat, file->pos);
    return (cluster_offset == 0) && (file->pos == file->size);
}

/**
 * �����ļ���С���������ļ����ݲ��֣���������mode������
 * @param file ��������ļ�
 * @param size �µ��ļ���С
 * @param mode ����ģʽ
 * @return
 */
static xfat_err_t expand_file(xfile_t * file, xfile_size_t size) {
    xfat_err_t err;
    xfat_t * xfat = file->xfat;
	u32_t curr_cluster_cnt = to_cluseter_count(xfat, file->size);
    u32_t expect_cluster_cnt = to_cluseter_count(xfat, size);

    // ������������Ҫ���ʱ���ڴ���֮����������
    if (curr_cluster_cnt < expect_cluster_cnt) {
        u32_t cluster_cnt = expect_cluster_cnt - curr_cluster_cnt;
        u32_t start_free_cluster = 0;
		u32_t curr_culster = file->curr_cluster;

        // �ȶ�λ���ļ������һ��, ����Ҫ��λ�ļ���С��Ϊ0�Ĵ�
        if (file->size > 0) {
			u32_t next_cluster = file->curr_cluster;

			do {
				curr_culster = next_cluster;

				err = get_next_cluster(xfat, curr_culster, &next_cluster);
				if (err) {
					file->err = err;
					return err;
				}
			} while (is_cluster_valid(next_cluster));
        }

        // Ȼ���ٴ����һ�ط���ռ�
        err = allocate_free_cluster(file->xfat, curr_culster, cluster_cnt, &start_free_cluster, 0, 0, 0);
        if (err) {
            file->err = err;
            return err;
        }

		if (!is_cluster_valid(file->start_cluster)) {
			file->start_cluster = start_free_cluster;
			file->curr_cluster = start_free_cluster;
		} else if (!is_cluster_valid(file->curr_cluster) || is_fpos_cluster_end(file)) {
			file->curr_cluster = start_free_cluster;
		}
    }

    // ����ٸ����ļ���С���Ƿ���ڹر��ļ�ʱ���У�
    err = update_file_size(file, size);
    return err;
}

/**
 * ��ָ���ļ���д������
 * @param buffer ���ݵĻ���
 * @param elem_size д���Ԫ���ֽڴ�С
 * @param count д����ٸ�elem_size
 * @param file ��д����ļ�
 * @return
 */
xfile_size_t xfile_write(void * buffer, xfile_size_t elem_size, xfile_size_t count, xfile_t * file) {
    xdisk_t * disk = file_get_disk(file);
    u32_t r_count_write = 0;
    xfile_size_t bytes_to_write = count * elem_size;
    xfat_err_t err;
    u8_t * write_buffer = (u8_t *)buffer;

     // ֻ����ֱ��д��ͨ�ļ�
    if (file->type != FAT_FILE) {
        file->err = FS_ERR_FSTYPE;
        return 0;
    }

    // ֻ���Լ��
    if (file->attr & XFILE_ATTR_READONLY) {
        file->err = FS_ERR_READONLY;
        return 0;
    }

    // �ֽ�Ϊ0������д��ֱ���˳�
    if (bytes_to_write == 0) {
        file->err = FS_ERR_OK;
        return 0;
    }

    // ��д�����������ļ���Сʱ��Ԥ�ȷ������дأ�Ȼ����д
    // ������д��ʱ���Ͳ��ؿ���дʱ�ļ���С������������
    if (file->size < file->pos + bytes_to_write) {
        err = expand_file(file, file->pos + bytes_to_write);
        if (err < 0) {
            file->err = err;
            return 0;
        }
    }

	while ((bytes_to_write > 0) && is_cluster_valid(file->curr_cluster)) {
		u32_t curr_write_bytes = 0;
        u32_t sector_count = 0;

		u32_t cluster_sector = to_sector(disk, to_cluster_offset(file->xfat, file->pos));  // ���е�����ƫ��
		u32_t sector_offset = to_sector_offset(disk, file->pos);  // ����ƫ��λ��
		u32_t start_sector = cluster_fist_sector(file->xfat, file->curr_cluster) + cluster_sector;

        // ��ʼ�������߽����, ֻдȡ��ǰ����
        // ������ʼΪ0����д����������ǰ������Ҳֻд��ǰ����
        // �����������������Ҫ�ݴ浽�������У�Ȼ�󿽱�����д��������
        if ((sector_offset != 0) || (!sector_offset && (bytes_to_write < disk->sector_size))) {
            sector_count = 1;
            curr_write_bytes = bytes_to_write;

            // ��ʼƫ�Ʒ�0�������������ֻд��ǰ����
            if (sector_offset != 0) {
                if (sector_offset + bytes_to_write > disk->sector_size) {
                    curr_write_bytes = disk->sector_size - sector_offset;
                }
            }

            // д��������д�벿�ֵ����壬����ٻ�д
            // todo: �������С������ʱ�����ܻ����¼���ͬһ����
            err = xdisk_read_sector(disk, temp_buffer, start_sector, 1);
            if (err < 0) {
                file->err = err;
                return 0;
            }

            memcpy(temp_buffer + sector_offset, write_buffer, curr_write_bytes);
            err = xdisk_write_sector(disk, temp_buffer, start_sector, 1);
            if (err < 0) {
                file->err = err;
                return 0;
            }

            write_buffer += curr_write_bytes;
            bytes_to_write -= curr_write_bytes;
        } else {
            // ��ʼΪ0����д������1������������д������
            sector_count = to_sector(disk, bytes_to_write);

            // �������һ�أ���ֻд��ǰ��
            // todo: ����������Ż�һ�£��������д�Ļ���ʵ���ǿ�����д��ص�
            if ((cluster_sector + sector_count) > file->xfat->sec_per_cluster) {
                sector_count = file->xfat->sec_per_cluster - cluster_sector;
            }

            err = xdisk_write_sector(disk, write_buffer, start_sector, sector_count);
            if (err != FS_ERR_OK) {
                file->err = err;
                return 0;
            }

            curr_write_bytes = sector_count * disk->sector_size;
            write_buffer += curr_write_bytes;
            bytes_to_write -= curr_write_bytes;
        }

        r_count_write += curr_write_bytes;

		err = move_file_pos(file, curr_write_bytes);
		if (err) return 0;
    }

    file->err = file->pos == file->size;
    return r_count_write / elem_size;
}

/**
 * �ļ��Ƿ��Ѿ���д��ĩβ
 * @param file ��ѯ���ļ�
 * @return
 */
xfat_err_t xfile_eof(xfile_t * file) {
    return (file->pos >= file->size) ? FS_ERR_EOF : FS_ERR_OK;
}

/**
 * ���ص�ǰ�ļ���λ��
 * @param file �Ѿ��򿪵��ļ�
 * @return
 */
xfile_size_t xfile_tell(xfile_t * file) {
    return file->pos;
}

/**
 * �����ļ���ǰ�Ķ�дλ��
 * @param file �Ѿ��򿪵��ļ�
 * @param offset �����originָ��λ�õ�ƫ����
 * @param origin ������ĸ�λ�ü���ƫ����
 * @return
 */
xfat_err_t xfile_seek(xfile_t * file, xfile_ssize_t offset, xfile_orgin_t origin) {
    xfat_err_t err = FS_ERR_OK;
    xfile_ssize_t final_pos;
    xfile_size_t offset_to_move;
    u32_t curr_cluster, curr_pos;

    // ��ȡ���յĶ�λλ��
    switch (origin) {
    case XFAT_SEEK_SET:
        final_pos = offset;
        break;
    case XFAT_SEEK_CUR:
        final_pos = file->pos + offset;
        break;
    case XFAT_SEEK_END:
        final_pos = file->size + offset;
        break;
    default:
        final_pos = -1;
        break;
    }

    // �����ļ���Χ
    if ((final_pos < 0) || (final_pos >= file->size)) {
        return FS_ERR_PARAM;
    }

    // ����ڵ�ǰҪ������ƫ����
    offset = final_pos - file->pos;
    if (offset > 0) {
        curr_cluster = file->curr_cluster;
        curr_pos = file->pos;
        offset_to_move = (xfile_size_t)offset;
    } else {
        curr_cluster = file->start_cluster;
        curr_pos = 0;
        offset_to_move = (xfile_size_t)final_pos;
    }

    while (offset_to_move > 0) {
        u32_t cluster_offset = to_cluster_offset(file->xfat, curr_pos);
        xfile_size_t curr_move = offset_to_move;

        // ��������ǰ��
        if (cluster_offset + curr_move < file->xfat->cluster_byte_size) {
            curr_pos += curr_move;
            break;
        }

        // ������ǰ�أ�ֻ�ڵ�ǰ�����ƶ�
        curr_move = file->xfat->cluster_byte_size - cluster_offset;
        curr_pos += curr_move;
        offset_to_move -= curr_move;

        // ������һ��: �Ƿ�Ҫ�жϺ������Ƿ���ȷ��
        err = get_next_cluster(file->xfat, curr_cluster, &curr_cluster);
        if (err < 0) {
            file->err = err;
            return err;
        }
    }

    file->pos = curr_pos;
    file->curr_cluster = curr_cluster;
    return FS_ERR_OK;
}

/**
 * ��ȡ�ļ��Ĵ�С
 * @param file �Ѿ��򿪵��ļ�
 * @param size �ļ���С
 * @return
 */
xfat_err_t xfile_size(xfile_t * file, xfile_size_t * size) {
    *size = file->size;
    return FS_ERR_OK;
}


/**
 * �ļ�������
 * @param xfat
 * @param path ��Ҫ�������ļ�����·��
 * @param new_name �ļ��µ�����
 * @return
 */
xfat_err_t xfile_rename(xfat_t* xfat, const char* path, const char* new_name) {
    diritem_t* diritem = (diritem_t*)0;
    u32_t curr_cluster, curr_offset;
    u32_t next_cluster, next_offset;
    u32_t found_cluster, found_offset;
    const char * curr_path;

    curr_cluster = xfat->root_cluster;
    curr_offset = 0;
    for (curr_path = path; curr_path != '\0'; curr_path = get_child_path(curr_path)) {
        do {
            xfat_err_t err = get_next_diritem(xfat, DIRITEM_GET_USED, curr_cluster, curr_offset,
                    &found_cluster, &found_offset , &next_cluster, &next_offset, temp_buffer, &diritem);
            if (err < 0) {
                return err;
            }

            if (diritem == (diritem_t*)0) {    // �Ѿ�������Ŀ¼����
                return FS_ERR_NONE;
            }

            if (is_filename_match((const char*)diritem->DIR_Name, curr_path)) {
                // �ҵ����Ƚ���һ����Ŀ¼
                if (get_file_type(diritem) == FAT_DIR) {
                    curr_cluster = get_diritem_cluster(diritem);
                    curr_offset = 0;
                }
                break;
            }

            curr_cluster = next_cluster;
            curr_offset = next_offset;
        } while (1);
    }

    if (diritem && !curr_path) {
        // ���ַ�ʽֻ������SFN�ļ���������
        u32_t dir_sector = to_phy_sector(xfat, found_cluster, found_offset);
        to_sfn((char *)diritem->DIR_Name, new_name);

        // �����ļ�����ʵ��������������ô�Сд
        diritem->DIR_NTRes &= ~DIRITEM_NTRES_CASE_MASK;
        diritem->DIR_NTRes |= get_sfn_case_cfg(new_name);

        return xdisk_write_sector(xfat_get_disk(xfat), temp_buffer, dir_sector, 1);
    }

    return FS_ERR_OK;
}
/**
 * ����diritem����Ӧ��ʱ�䣬�����ļ�ʱ���޸ĵĻص�����
 * @param xfat xfat�ṹ
 * @param dir_item Ŀ¼�ṹ��
 * @param arg1 �޸ĵ�ʱ������
 * @param arg2 �µ�ʱ��
 * @return
 */
static xfat_err_t set_file_time (xfat_t *xfat, const char * path, stime_type_t time_type, xfile_time_t * time) {
    diritem_t* diritem = (diritem_t*)0;
    u32_t curr_cluster, curr_offset;
    u32_t next_cluster, next_offset;
    u32_t found_cluster, found_offset;
    const char * curr_path;

    curr_cluster = xfat->root_cluster;
    curr_offset = 0;
    for (curr_path = path; curr_path != '\0'; curr_path = get_child_path(curr_path)) {
        do {
            xfat_err_t err = get_next_diritem(xfat, DIRITEM_GET_USED, curr_cluster, curr_offset,
                &found_cluster, &found_offset, &next_cluster, &next_offset, temp_buffer, &diritem);
            if (err < 0) {
                return err;
            }

            if (diritem == (diritem_t*)0) {    // �Ѿ�������Ŀ¼����
                return FS_ERR_NONE;
            }

            if (is_filename_match((const char*)diritem->DIR_Name, curr_path)) {
                // �ҵ����Ƚ���һ����Ŀ¼
                if (get_child_path(curr_path)) {
                    curr_cluster = get_diritem_cluster(diritem);
                    curr_offset = 0;
                }
                break;
            }

            curr_cluster = next_cluster;
            curr_offset = next_offset;
        } while (1);
    }

    if (diritem && !curr_path) {
        // ���ַ�ʽֻ������SFN�ļ���������
        u32_t dir_sector = to_phy_sector(xfat, curr_cluster, curr_offset);

        // �����ļ�����ʵ��������������ô�Сд
        switch (time_type) {
            case XFAT_TIME_CTIME:
                diritem->DIR_CrtDate.year_from_1980 = (u16_t) (time->year - 1980);
                diritem->DIR_CrtDate.month = time->month;
                diritem->DIR_CrtDate.day = time->day;
                diritem->DIR_CrtTime.hour = time->hour;
                diritem->DIR_CrtTime.minute = time->minute;
                diritem->DIR_CrtTime.second_2 = (u16_t) (time->second / 2);
                diritem->DIR_CrtTimeTeenth = (u8_t) (time->second % 2 * 1000 / 100);
                break;
            case XFAT_TIME_ATIME:
                diritem->DIR_LastAccDate.year_from_1980 = (u16_t) (time->year - 1980);
                diritem->DIR_LastAccDate.month = time->month;
                diritem->DIR_LastAccDate.day = time->day;
                break;
            case XFAT_TIME_MTIME:
                diritem->DIR_WrtDate.year_from_1980 = (u16_t) (time->year - 1980);
                diritem->DIR_WrtDate.month = time->month;
                diritem->DIR_WrtDate.day = time->day;
                diritem->DIR_WrtTime.hour = time->hour;
                diritem->DIR_WrtTime.minute = time->minute;
                diritem->DIR_WrtTime.second_2 = (u16_t) (time->second / 2);
                break;
        }

        return xdisk_write_sector(xfat_get_disk(xfat), temp_buffer, dir_sector, 1);
    }

    return FS_ERR_OK;

}

/**
 * �����ļ��ķ���ʱ��
 * @param xfat xfat�ṹ
 * @param path �ļ�������·��
 * @param time �ļ����·���ʱ��
 * @return
 */
xfat_err_t xfile_set_atime (xfat_t * xfat, const char * path, xfile_time_t * time) {
    xfat_err_t err = set_file_time(xfat, path, XFAT_TIME_ATIME, time);
    return err;
}

/**
 * �����ļ����޸�ʱ��
 * @param xfat xfat�ṹ
 * @param path �ļ�������·��
 * @param time �µ��ļ��޸�ʱ��
 * @return
 */
xfat_err_t xfile_set_mtime (xfat_t * xfat, const char * path, xfile_time_t * time) {
    xfat_err_t err = set_file_time(xfat, path, XFAT_TIME_MTIME, time);
    return err;
}

/**
 * �����ļ��Ĵ���ʱ��
 * @param xfat fsfa�ṹ
 * @param path �ļ�������·��
 * @param time �µ��ļ�����ʱ��
 * @return
 */
xfat_err_t xfile_set_ctime (xfat_t * xfat, const char * path, xfile_time_t * time) {
    xfat_err_t err = set_file_time(xfat, path, XFAT_TIME_CTIME, time);
    return err;
}

/**
 * ɾ��ָ��·�����ļ�
 * @param xfat xfat�ṹ
 * @param file_path �ļ���·��
 * @return
 */
xfat_err_t xfile_rmfile(xfat_t * xfat, const char * path) {
    diritem_t* diritem = (diritem_t*)0;
    u32_t curr_cluster, curr_offset;
    u32_t found_cluster, found_offset;
    u32_t next_cluster, next_offset;
    const char* curr_path;

    curr_cluster = xfat->root_cluster;
    curr_offset = 0;
    for (curr_path = path; curr_path != '\0'; curr_path = get_child_path(curr_path)) {
        do {
            xfat_err_t err = get_next_diritem(xfat, DIRITEM_GET_USED, curr_cluster, curr_offset,
                &found_cluster, &found_offset, &next_cluster, &next_offset, temp_buffer, &diritem);
            if (err < 0) {
                return err;
            }

            if (diritem == (diritem_t*)0) {    // �Ѿ�������Ŀ¼����
                return FS_ERR_NONE;
            }

            if (is_filename_match((const char*)diritem->DIR_Name, curr_path)) {
                // �ҵ����Ƚ���һ����Ŀ¼
                if (get_child_path(curr_path)) {
                    curr_cluster = get_diritem_cluster(diritem);
                    curr_offset = 0;
                }
                break;
            }

            curr_cluster = next_cluster;
            curr_offset = next_offset;
        } while (1);
    }

    if (diritem && !curr_path) {
        xfat_err_t err;

        // �������ô�ɾ��Ŀ¼
        if (diritem->DIR_Attr & DIRITEM_ATTR_DIRECTORY) {
            return FS_ERR_PARAM;
        }

        // ���ַ�ʽֻ������SFN�ļ���������
        u32_t dir_sector = to_phy_sector(xfat, found_cluster, found_offset);

        diritem->DIR_Name[0] = DIRITEM_NAME_FREE;

        err = xdisk_write_sector(xfat_get_disk(xfat), temp_buffer, dir_sector, 1);
        if (err < 0) return err;

        err = destroy_cluster_chain(xfat, get_diritem_cluster(diritem));
        if (err < 0) return err;

        return FS_ERR_OK;
    }

    return FS_ERR_NONE;
}
/**
 * �ж�ָ��Ŀ¼���Ƿ�������(���ļ�)
 * @param file ����Ŀ¼
 * @return
 */
static xfat_err_t dir_has_child(xfat_t * xfat, u32_t dir_cluster, int * has_child) {
    u32_t curr_cluster = dir_cluster, curr_offset = 0;
    u32_t found_cluster, found_offset;
    u32_t next_cluster, next_offset;
    diritem_t* diritem = (diritem_t*)0;

    do {
        xfat_err_t err = get_next_diritem(xfat, DIRITEM_GET_USED, curr_cluster, curr_offset,
            &found_cluster, &found_offset, &next_cluster, &next_offset, temp_buffer, &diritem);
        if (err < 0) {
            return err;
        }

        if (diritem == (diritem_t*)0) {
            *has_child = 0;
            break;
        }

        if (is_locate_type_match(diritem, XFILE_LOCATE_NORMAL)) {
            *has_child = 1;
            break;
        }

        curr_cluster = next_cluster;
        curr_offset = next_offset;
    } while (1);

    return FS_ERR_OK;
}

/**
 * ɾ��ָ��·����Ŀ¼(����ɾ��Ŀ¼Ϊ�յ�Ŀ¼)
 * @param xfat xfat�ṹ
 * @param file_path Ŀ¼��·��
 * @return
 */
xfat_err_t xfile_rmdir (xfat_t * xfat, const char * path) {
    diritem_t* diritem = (diritem_t*)0;
    u32_t curr_cluster, curr_offset;
    u32_t found_cluster, found_offset;
    u32_t next_cluster, next_offset;
    const char* curr_path;

    // ��λpath����Ӧ��λ�ú�dirite m
    curr_cluster = xfat->root_cluster;
    curr_offset = 0;
    for (curr_path = path; curr_path != '\0'; curr_path = get_child_path(curr_path)) {
        do {
            xfat_err_t err = get_next_diritem(xfat, DIRITEM_GET_USED, curr_cluster, curr_offset,
                &found_cluster, &found_offset, &next_cluster, &next_offset, temp_buffer, &diritem);
            if (err < 0) {
                return err;
            }

            if (diritem == (diritem_t*)0) {    // �Ѿ�������Ŀ¼����
                return FS_ERR_NONE;
            }

            if (is_filename_match((const char*)diritem->DIR_Name, curr_path)) {
                // �ҵ����Ƚ���һ����Ŀ¼
                if (get_child_path(curr_path)) {
                    curr_cluster = get_diritem_cluster(diritem);
                    curr_offset = 0;
                }
                break;
            }

            curr_cluster = next_cluster;
            curr_offset = next_offset;
        } while (1);
    }

    if (diritem && !curr_path) {
        xfat_err_t err;
        int has_child;
        u32_t dir_sector;

        if (get_file_type(diritem) != FAT_DIR) {
            return FS_ERR_PARAM;
        }

        dir_sector = to_phy_sector(xfat, found_cluster, found_offset);
        err = dir_has_child(xfat, get_diritem_cluster(diritem), &has_child);
        if (err < 0) return err;

        if (has_child) {
            return FS_ERR_NOT_EMPTY;
        }

        // dir_has_child���ƻ��������������������¼���һ��
        err = xdisk_read_sector(xfat_get_disk(xfat), temp_buffer, dir_sector, 1);
        if (err < 0) return err;

        diritem = (diritem_t*)(temp_buffer + to_sector_offset(xfat_get_disk(xfat), found_offset));
        diritem->DIR_Name[0] = DIRITEM_NAME_FREE;

        err = xdisk_write_sector(xfat_get_disk(xfat), temp_buffer, dir_sector, 1);
        if (err < 0) return err;

        err = destroy_cluster_chain(xfat, get_diritem_cluster(diritem));
        if (err < 0) return err;

        return FS_ERR_OK;
    }

    return FS_ERR_NONE;
}

/**
 * �ݹ�ɾ���������ļ���Ŀ¼
 * @param xfat
 * @param parent_cluster 
 * @return
 */
static xfat_err_t rmdir_all_children(xfat_t* xfat, u32_t parent_cluster) {
    diritem_t* diritem = (diritem_t*)0;
    u32_t curr_cluster = parent_cluster, curr_offset = 0;
    u32_t found_cluster, found_offset;
    u32_t next_cluster, next_offset;

    do {
        xfat_err_t err = get_next_diritem(xfat, DIRITEM_GET_USED, curr_cluster, curr_offset,
            &found_cluster, &found_offset, &next_cluster, &next_offset, temp_buffer, &diritem);
        if (err < 0) {
            return err;
        }

        if (diritem == (diritem_t*)0) {    // �Ѿ�������Ŀ¼����
            return FS_ERR_OK;
        }

        if (diritem->DIR_Name[0] == DIRITEM_NAME_END) {
            return FS_ERR_OK;
        }

        if (is_locate_type_match(diritem, XFILE_LOCATE_NORMAL)) {
            u32_t dir_cluster = get_diritem_cluster(diritem);
            u32_t dir_sector = to_phy_sector(xfat, found_cluster, found_offset);

            diritem->DIR_Name[0] = DIRITEM_NAME_FREE;
            err = xdisk_write_sector(xfat_get_disk(xfat), temp_buffer, dir_sector, 1);
            if (err < 0) return err;

            if (get_file_type(diritem) == FAT_DIR) {
                // ������ܻ�Ļ��棬�������ʹ��dir_cluster
                err = rmdir_all_children(xfat, dir_cluster);
                if (err < 0) return err;
            }

            // ע�⣬��������Ƶ�Ĳ�����ͬ����Ƶ�е���get_diritem_cluster()�ᵼ��bug
            err = destroy_cluster_chain(xfat, dir_cluster);
            if (err < 0) return err;
        }

        curr_cluster = next_cluster;
        curr_offset = next_offset;
    } while (1);

    return FS_ERR_OK;
}

/**
 * ɾ��ָ��·����Ŀ¼(����ɾ��Ŀ¼Ϊ�յ�Ŀ¼)
 * @param xfat xfat�ṹ
 * @param file_path Ŀ¼��·��
 * @return
 */
xfat_err_t xfile_rmdir_tree(xfat_t* xfat, const char* path) {
    diritem_t* diritem = (diritem_t*)0;
    u32_t curr_cluster, curr_offset;
    u32_t found_cluster, found_offset;
    u32_t next_cluster, next_offset;
    const char* curr_path;

    // ��λpath����Ӧ��λ�ú�diritem
    curr_cluster = xfat->root_cluster;
    curr_offset = 0;
    for (curr_path = path; curr_path != '\0'; curr_path = get_child_path(curr_path)) {
        do {
            xfat_err_t err = get_next_diritem(xfat, DIRITEM_GET_USED, curr_cluster, curr_offset,
                &found_cluster, &found_offset, &next_cluster, &next_offset, temp_buffer, &diritem);
            if (err < 0) {
                return err;
            }

            if (diritem == (diritem_t*)0) {    // �Ѿ�������Ŀ¼����
                return FS_ERR_NONE;
            }

            if (is_filename_match((const char*)diritem->DIR_Name, curr_path)) {
                // �ҵ����Ƚ���һ����Ŀ¼
                if (get_child_path(curr_path)) {
                    curr_cluster = get_diritem_cluster(diritem);
                    curr_offset = 0;
                }
                break;
            }

            curr_cluster = next_cluster;
            curr_offset = next_offset;
        } while (1);
    }

    if (diritem && !curr_path) {
        xfat_err_t err;
        u32_t dir_sector;
        u32_t diritem_cluster = get_diritem_cluster(diritem);

        if (get_file_type(diritem) != FAT_DIR) {
            return FS_ERR_PARAM;
        }

        dir_sector = to_phy_sector(xfat, found_cluster, found_offset);
        diritem->DIR_Name[0] = DIRITEM_NAME_FREE;
        err = xdisk_write_sector(xfat_get_disk(xfat), temp_buffer, dir_sector, 1);
        if (err < 0) return err;

        err = rmdir_all_children(xfat, diritem_cluster);
        if (err < 0) return err;

        err = destroy_cluster_chain(xfat, diritem_cluster);
        if (err < 0) return err;

        return FS_ERR_OK;
    }

    return FS_ERR_NONE;
}

/**
 * �ر��Ѿ��򿪵��ļ�
 * @param file ���رյ��ļ�
 * @return
 */
xfat_err_t xfile_close(xfile_t *file) {
    // ����������ļ��ķ���ʱ�䣬дʱ������ݺ͵�����������ֵ���޸ĵ�
    return FS_ERR_OK;
}
