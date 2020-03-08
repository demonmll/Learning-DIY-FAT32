/**
 * ��Դ�����׵Ŀγ�Ϊ - ��0��1����дFAT32�ļ�ϵͳ��ÿ�����̶�Ӧһ����ʱ��������ע�͡�
 * ���ߣ�����ͭ
 * �γ���ַ��http://01ketang.cc
 * ��Ȩ��������Դ��ǿ�Դ�����ο���������������ǰ����ϵ���ߡ�
 */
#include "xfat_buf.h"
#include "xdisk.h"
#include "xfat.h"

/**
 * ��ȡbuf��״̬
 * @param buf
 * @param state
 */
void xfat_buf_set_state(xfat_buf_t * buf, u32_t state) {
    buf->flags &= ~XFAT_BUF_STATE_MSK;
    buf->flags |= state;
}

static xfat_err_t bpool_moveto_first(xfat_bpool_t* pool, xfat_buf_t* buf) {
    xfat_err_t err = FS_ERR_OK;

    if (pool->first == buf) {
        return err;
    }

    buf->pre->next = buf->next;
    buf->next->pre = buf->pre;
    if (pool->last == buf) {
        pool->last = buf->pre;
    }

    pool->last->next = buf;
    pool->first->pre = buf;
    buf->next = pool->first;
    buf->pre = pool->first->pre;
    pool->first = buf;

    return err;
}


/**
 * �ӻ����б��з���һ�������
 * @param pool
 * @param sector_no
 * @return
 */
static xfat_err_t bpool_find_buf(xfat_bpool_t* pool, u32_t sector_no, xfat_buf_t** buf) {
    xfat_buf_t* r_buf;
    u32_t size = pool->size;
	xfat_buf_t* free_buf = (xfat_buf_t *)0;
    xfat_err_t err;

    if (pool->first == (xfat_buf_t*)0) {
        return FS_ERR_NO_BUFFER;
    }

    // �ӱ�ͷ��ʼ���ң��ҵ���ͬ�����Ŀ飬�����ҵ����п�
    // �������϶�û�У��ҵ����δ��ʹ�õĿ飬��ʱ�϶��Ƕ������һ��
    r_buf = pool->first;
    while (size--) {
        switch (xfat_buf_state(r_buf)) {
        case XFAT_BUF_STATE_FREE:
            free_buf = r_buf;
            break;
        case XFAT_BUF_STATE_CLEAN:
        case XFAT_BUF_STATE_DIRTY:
            if (r_buf->sector_no == sector_no) {
                *buf = r_buf;
                err = bpool_moveto_first(pool, r_buf);
                return err;
            }
            break;
        }

        r_buf = r_buf->next;
    }

    if (free_buf != (xfat_buf_t*)0) {
        *buf = free_buf;
    }
    else {
        *buf = pool->last;
    }

    err = bpool_moveto_first(pool, *buf);
    return err;
}

/**
 * ��ȡָ��obj��Ӧ�Ļ����
 */
static xfat_bpool_t* get_obj_bpool(xfat_obj_t* obj) {
    if (obj->type == XFAT_OBJ_FILE) {
        xfile_t* file = to_type(obj, xfile_t);
        obj = file->xfat->disk_part->disk;
    }

    if (obj->type == XFAT_OBJ_FAT) {
        xfat_t* xfat = to_type(obj, xfat_t);
        obj = xfat->disk_part->disk;
    }

    if (obj->type == XFAT_OBJ_DISK) {
        xdisk_t* disk = to_type(obj, xdisk_t);
        return &disk->bpool;
    }


    return (xfat_bpool_t*)0;
}

/**
 * ��ȡobj��Ӧ��disk
 */
static xdisk_t * get_obj_disk(xfat_obj_t* obj) {
    xdisk_t* xdisk = (xdisk_t*)0;

    switch (obj->type) {
    case XFAT_OBJ_FAT:
        xdisk = ((xfat_t*)obj)->disk_part->disk;
        break;
    case XFAT_OBJ_FILE:
        xdisk = ((xfile_t*)obj)->xfat->disk_part->disk;
        break;
    case XFAT_OBJ_DISK:
        xdisk = (xdisk_t*)obj;
        break;
    }
    return xdisk;
}

/**
 * ��ʼ�����̻�����
 * @param pool
 * @param disk
 * @param buffer
 * @param buf_size
 * @return
 */
xfat_err_t xfat_bpool_init(xfat_obj_t* obj, u32_t sector_size, u8_t* buffer, u32_t buf_size) {
    u32_t buf_count = buf_size / (sizeof(xfat_buf_t) + sector_size);
    u32_t i;
    xfat_buf_t * buf_start = (xfat_buf_t *)buffer;
    u8_t * sector_buf_start = buffer + buf_count * sizeof(xfat_buf_t);      // ����������¶��봦��
    xfat_buf_t* buf;

    xfat_bpool_t* pool = get_obj_bpool(obj);
    if (pool == (xfat_bpool_t*)0) {
        return FS_ERR_PARAM;
    }

    if (buf_count == 0) {
        pool->first = pool->last = (xfat_buf_t*)0;
        pool->size = 0;
        return FS_ERR_OK;
    }

    // ͷ�巨��������
    buf = (xfat_buf_t*)buf_start++;
    buf->pre = buf->next = buf;
    buf->buf = sector_buf_start;
    pool->first = pool->last = buf;
    sector_buf_start += sector_size;

    for (i = 1; i < buf_count; i++, sector_buf_start += sector_size) {
        xfat_buf_t * buf = buf_start++;
        buf->next = pool->first;
        buf->pre = pool->first->pre;

        buf->next->pre = buf;
        buf->pre->next = buf;
        pool->first = buf;

        buf->sector_no = 0;
        buf->buf = sector_buf_start;
        buf->flags = XFAT_BUF_STATE_FREE;
    }

    pool->size = buf_count;
    return FS_ERR_OK;
}

/**
 * �Ի��巽ʽ��ȡ���̵�ָ������
 * @param disk
 * @param buf
 * @param sector_no
 * @return
 */
xfat_err_t xfat_bpool_read_sector(xfat_obj_t* obj, xfat_buf_t** buf, u32_t sector_no) {
    xfat_err_t err;
    xfat_buf_t* r_buf = (xfat_buf_t*)0;
    int need_read = 0;

    xfat_bpool_t* pool = get_obj_bpool(obj);
    if (pool == (xfat_bpool_t*)0) {
        return FS_ERR_OK;
    }

    err = bpool_find_buf(pool, sector_no, &r_buf);
    if (err < 0) {
        return err;
    }

    if ((sector_no == r_buf->sector_no) && (xfat_buf_state(r_buf) != XFAT_BUF_STATE_FREE)) {
        *buf = r_buf;
        return FS_ERR_OK;
    }

    // ��ͬ����
    switch (xfat_buf_state(r_buf)) {
    case XFAT_BUF_STATE_FREE:
    case XFAT_BUF_STATE_CLEAN:
        break;
    case XFAT_BUF_STATE_DIRTY:
        err = xdisk_write_sector(get_obj_disk(obj), r_buf->buf, r_buf->sector_no, 1);
        if (err < 0) {
            return err;
        }
    }

    err = xdisk_read_sector(get_obj_disk(obj), r_buf->buf, sector_no, 1);
    if (err < 0) {
        return err;
    }

    xfat_buf_set_state(r_buf, XFAT_BUF_STATE_CLEAN);
    r_buf->sector_no = sector_no;
    *buf = r_buf;
    return FS_ERR_OK;
}

/**
 * Ϊ�����������ռ�
 * @param disk
 * @param buf
 * @return
 */
xfat_err_t xfat_bpool_alloc(xfat_obj_t* obj, xfat_buf_t** buf, u32_t sector_no) {
    xfat_err_t err;
    xfat_buf_t* r_buf = (xfat_buf_t*)0;
    int need_read = 0;

    xfat_bpool_t* pool = get_obj_bpool(obj);
    if (pool == (xfat_bpool_t*)0) {
        return FS_ERR_OK;
    }

    err = bpool_find_buf(pool, sector_no, &r_buf);
    if (err < 0) {
        return err;
    }

    if ((sector_no == r_buf->sector_no) && (xfat_buf_state(r_buf) != XFAT_BUF_STATE_FREE)) {
        *buf = r_buf;
        return FS_ERR_OK;
    }

    // ��ͬ����
    switch (xfat_buf_state(r_buf)) {
    case XFAT_BUF_STATE_FREE:
    case XFAT_BUF_STATE_CLEAN:
        break;
    case XFAT_BUF_STATE_DIRTY:
        err = xdisk_write_sector(get_obj_disk(obj), r_buf->buf, r_buf->sector_no, 1);
        if (err < 0) {
            return err;
        }
    }

    xfat_buf_set_state(r_buf, XFAT_BUF_STATE_CLEAN);
    r_buf->sector_no = sector_no;
    *buf = r_buf;
    return FS_ERR_OK;
}

/**
 * �Ի��巽ʽдȡ���̵�ָ������
 * @param disk
 * @param buf
 * @param sector_no
 * @return
 */
xfat_err_t xfat_bpool_write_sector(xfat_obj_t* obj, xfat_buf_t* buf, u8_t is_through) {
    xfat_err_t err = FS_ERR_OK;

    if (is_through) {
        err = xdisk_write_sector(get_obj_disk(obj), buf->buf, buf->sector_no, 1);
        if (err < 0) {
            return err;
        }
        xfat_buf_set_state(buf, XFAT_BUF_STATE_CLEAN);
    } else {
        xfat_buf_set_state(buf, XFAT_BUF_STATE_DIRTY);
    }
    return err;
}

/**
 * ���뻺���������ݿ�
 * @param disk
 * @return
 */
xfat_err_t xfat_bpool_flush(xfat_obj_t* obj) {
    xfat_err_t err;
    xfat_buf_t* curr_buf;
    xfat_bpool_t* pool = get_obj_bpool(obj);
    u32_t size = pool->size;

    if (pool == (xfat_bpool_t*)0) {
        return FS_ERR_PARAM;
    }

    curr_buf = pool->first;
    while (size--) {
        switch (xfat_buf_state(curr_buf)) {
        case XFAT_BUF_STATE_FREE:
        case XFAT_BUF_STATE_CLEAN:
            break;
        case XFAT_BUF_STATE_DIRTY:
            err = xdisk_write_sector(get_obj_disk(obj), curr_buf->buf, curr_buf->sector_no, 1);
            if (err < 0) {
                return err;
            }
            xfat_buf_set_state(curr_buf, XFAT_BUF_STATE_CLEAN);
            break;
        }

        curr_buf = curr_buf->next;
    }

    return FS_ERR_OK;
}
/**
 * ��ָ��������Χ�ڵĻ����д������
 * @param obj
 * @param start_sector
 * @param count
 * @return
 */
xfat_err_t xfat_bpool_flush_sectors(xfat_obj_t* obj, u32_t start_sector, u32_t count) {
    xfat_err_t err;
    xfat_buf_t* curr_buf;
    xfat_bpool_t* pool = get_obj_bpool(obj);
    u32_t size = pool->size;
    u32_t end_sector = start_sector + count - 1;

    if (pool == (xfat_bpool_t*)0) {
        return FS_ERR_PARAM;
    }

    curr_buf = pool->first;
    while (size--) {
        switch (xfat_buf_state(curr_buf)) {
        case XFAT_BUF_STATE_FREE:
        case XFAT_BUF_STATE_CLEAN:
            break;
        case XFAT_BUF_STATE_DIRTY:
            if ((curr_buf->sector_no >= start_sector) && (curr_buf->sector_no <= end_sector)) {
                err = xdisk_write_sector(get_obj_disk(obj), curr_buf->buf, curr_buf->sector_no, 1);
                if (err < 0) {
                    return err;
                }
                xfat_buf_set_state(curr_buf, XFAT_BUF_STATE_CLEAN);
            }
            break;
        }

        curr_buf = curr_buf->next;
    }

    return FS_ERR_OK;
}

/**
 * ���ָ��������Χ�ڵĻ��棬���еĻ��潫ֱ�Ӷ���
 * @param obj
 * @param start_sector
 * @param count
 * @return
 */
xfat_err_t xfat_bpool_invalid_sectors(xfat_obj_t* obj, u32_t start_sector, u32_t count) {
    xfat_buf_t* curr_buf;
    xfat_bpool_t* pool = get_obj_bpool(obj);
    u32_t size = pool->size;
    u32_t end_sector = start_sector + count - 1;

    if (pool == (xfat_bpool_t*)0) {
        return FS_ERR_PARAM;
    }

    curr_buf = pool->first;
    while (size--) {
        switch (xfat_buf_state(curr_buf)) {
            case XFAT_BUF_STATE_FREE:
                break;
            case XFAT_BUF_STATE_CLEAN:
            case XFAT_BUF_STATE_DIRTY:
                if ((curr_buf->sector_no >= start_sector) && (curr_buf->sector_no <= end_sector)) {
                    xfat_buf_set_state(curr_buf, XFAT_BUF_STATE_FREE);
                }
                break;
        }

        curr_buf = curr_buf->next;
    }

    return FS_ERR_OK;
}

