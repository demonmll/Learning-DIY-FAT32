/**
 * ��Դ�����׵Ŀγ�Ϊ - ��0��1����дFAT32�ļ�ϵͳ��ÿ�����̶�Ӧһ����ʱ��������ע�͡�
 * ���ߣ�����ͭ
 * �γ���ַ��http://01ketang.cc
 * ��Ȩ��������Դ��ǿ�Դ�����ο���������������ǰ����ϵ���ߡ�
 */
#include <stdio.h>
#include "xdisk.h"
#include "xfat.h"

static int array[32];

void print_int (int v) {
    printf("v = %d\n", v);
}

int main(void) {
    int * p_array = array;

    for (int i = 0; i < 100; i++) {
        print_int(i);
    }

    p_array[0] = 32;

    printf("Test End!\n");
    return 0;
}