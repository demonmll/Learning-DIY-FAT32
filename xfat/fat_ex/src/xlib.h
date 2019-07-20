/**
 * ��Դ�����׵Ŀγ�Ϊ - ��0��1����дFAT32�ļ�ϵͳ��ÿ�����̶�Ӧһ����ʱ��������ע�͡�
 * ���ߣ�����ͭ
 * �γ���ַ��http://01ketang.cc
 * ��Ȩ��������Դ��ǿ�Դ�����ο���������������ǰ����ϵ���ߡ�
 */
#ifndef XFAT_LIB_H
#define XFAT_LIB_H

#include "xtypes.h"

#define xlib_assert(expr, ...)	do {	\
	int c = (expr);	\
	if (!c) {printf("\n---- error ---\n%s %d %s failed:"#expr"\n"#__VA_ARGS__"\n",  \
			__FILE__, __LINE__, __func__); exit(c);}	\
} while (0)

#define xlib_abort(expr, ...) do {	\
	int c = (expr);	\
	if (!c) {printf("\n---- error ---\n%s %d %s failed:"#expr"\n"#__VA_ARGS__"\n",  \
			__FILE__, __LINE__, __func__); exit(c);}	\
} while (0)

void * xlib_memset(void * dest, u8_t v, u32_t size);
int xlib_memcmp(const void* ptr1, const void* ptr2, u32_t n);
void* xlib_memcpy(void* ptr1, const void* ptr2, u32_t n);
char* xlib_strncpy(char* dest, const char* src, u32_t n);
int xlib_strncmp(const char* str1, const char* str2, u32_t n);

int xlib_islower(int c);
int xlib_toupper(int c);
int xlib_tolower(int c);

#endif

