#include <stdio.h>
#include <stdlib.h>
#include "utokyo_cs_2022.h"

/* 向上取整：(k*l)/m */
int multfrac(int k, int l, int m){
    return (k * l + (m - 1)) / m;
}

void compare_swap(int *p, int *q){
    if (*p > *q) {
        int tmp = *p;
        *p = *q;
        *q = tmp;
    }
}

void mysort(int a[], int i, int j, Params* params){
    int k = j - i;

    if (k < 4){
        if (k <= 1){
            return;
        }
        while (k--){
            for (int h = i; h + 1 < j; h++){
                compare_swap(a + h, a + (h + 1));
            }
        }
    } else {
        mysort(a, i, i + multfrac(k, params->x, params->w), params);
        mysort(a, j - multfrac(k, params->y, params->w), j, params);
        mysort(a, i, i + multfrac(k, params->z, params->w), params);
    }
}