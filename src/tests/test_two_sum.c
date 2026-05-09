#include <stdio.h>
#include <stdlib.h>
#include "../array/two_sum.h"

void test_two_sum() {
    int nums[] = {2, 7, 11, 15};
    int size = sizeof(nums) / sizeof(nums[0]);
    int target = 9;
    int returnSize = 0;

    int* result = twoSum(nums, size, target, &returnSize);

    printf("[Two Sum] Result: ");
    for (int i = 0; i < returnSize; i++) {
        printf("%d ", result[i]);
    }
    printf("\n");

    // ⚠️ 释放内存（如果 twoSum 里 malloc 了）
    free(result);
}