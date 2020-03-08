/**
 * ��Դ�����׵Ŀγ�Ϊ - ��0��1����дFAT32�ļ�ϵͳ��ÿ�����̶�Ӧһ����ʱ��������ע�͡�
 * ���ߣ�����ͭ
 * �γ���ַ��http://01ketang.cc
 * ��Ȩ��������Դ��ǿ�Դ�����ο���������������ǰ����ϵ���ߡ�
 */
#ifndef XFAT_OBJ_H
#define XFAT_OBJ_H

#include "xtypes.h"

#define to_type(obj, type_p)    ((type_p *)(obj))
#define to_obj(obj)             ((xfat_obj_t *)(obj))

typedef enum _xfat_obj_type_t {
    XFAT_OBJ_DISK,
    XFAT_OBJ_FAT,
    XFAT_OBJ_FILE,
}xfat_obj_type_t;;

typedef struct _xfat_obj_t {
    xfat_obj_type_t type;
}xfat_obj_t;

xfat_err_t xfat_obj_init(xfat_obj_t* obj, xfat_obj_type_t type);

#endif
