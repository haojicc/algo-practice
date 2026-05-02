#ifndef UTOKYO_CS_2022_H
#define UTOKYO_CS_2022_H

/* 结构体定义 */
typedef struct {
    int x, y, z, w;
} Params;

/* 函数声明 */
int multfrac(int k, int l, int m);
void compare_swap(int *p, int *q);
void mysort(int a[], int i, int j, Params* params);

#endif