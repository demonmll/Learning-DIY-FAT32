/**
 * ��Դ�����׵Ŀγ�Ϊ - ��0��1����дFAT32�ļ�ϵͳ��ÿ�����̶�Ӧһ����ʱ��������ע�͡�
 * ���ߣ�����ͭ
 * �γ���ַ��http://01ketang.cc
 * ��Ȩ��������Դ��ǿ�Դ�����ο���������������ǰ����ϵ���ߡ�
 */
#ifndef XTYPES_H
#define XTYPES_H

#include <stdint.h>

typedef uint8_t u8_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;
typedef uint64_t u64_t;
typedef uint32_t xfile_size_t;
typedef int64_t xfile_ssize_t;

typedef enum _xfat_err_t {
    FS_ERR_EOF = 1,
    FS_ERR_OK = 0,
    FS_ERR_IO = -1,
    FS_ERR_PARAM = -2,
    FS_ERR_NONE = -3,
    FS_ERR_FSTYPE = -4,
    FS_ERR_READONLY = -5,
    FS_ERR_DISK_FULL = -6,
    FS_ERR_EXISTED = -7,
    FS_ERR_NAME_USED = -8,
    FS_ERR_NOT_EMPTY = -9,
    FS_ERR_NOT_MOUNT = -10,
}xfat_err_t;

#endif

