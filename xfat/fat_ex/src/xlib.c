/**
 * ��Դ�����׵Ŀγ�Ϊ - ��0��1����дFAT32�ļ�ϵͳ��ÿ�����̶�Ӧһ����ʱ��������ע�͡�
 * ���ߣ�����ͭ
 * �γ���ַ��http://01ketang.cc
 * ��Ȩ��������Դ��ǿ�Դ�����ο���������������ǰ����ϵ���ߡ�
 */
#include "xlib.h"

void * xlib_memset(void* dest, u8_t v, u32_t size) {
	u8_t* d = (u8_t*)dest;

	while (size--) {
		*d++ = v;
	}

	return dest;
}

int xlib_memcmp(const void* ptr1, const void* ptr2, u32_t n) {
	u8_t * d1 = (u8_t*)ptr1;
	u8_t* d2 = (u8_t*)ptr2;

	while (n && (*d1 == *d2)) { 
		n--; 
		d1++;
		d2++;
	}

	return n ? (*d1 - *d2) : 0;
}

void* xlib_memcpy(void* ptr1, const void* ptr2, u32_t n) {
	u8_t* d1 = (u8_t*)ptr1;
	u8_t* d2 = (u8_t*)ptr2;

	while (n--) {
		*d1++ = *d2++;
	}

	return ptr1;
}

char* xlib_strncpy(char* dest, const char* src, u32_t n) {
	char* r = dest;

	// �������ȵ��ַ������
	while (n--) {
		*dest++ = *src ? *src++ : '\0';
	}

	return r;
}

int xlib_strncmp(const char* str1, const char* str2, u32_t n) {
	// ���Ƚ�ǰn���ַ�
	while (n-- && *str1 && *str2) {}
	return 0;
}

int xlib_islower(int c) {
	return (c >= 'a') && (c <= 'z');
}

int xlib_toupper(int c) {
	return ((c >= 'a') && (c <= 'z')) ? (c - 'a' + 'A') : c;
}

int xlib_tolower(int c) {
	return ((c >= 'A') && (c <= 'Z')) ? (c - 'A' + 'a') : c;
}
