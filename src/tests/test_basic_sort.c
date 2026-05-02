#include <stdio.h>
#include "../sort/basic_sort.h"

void printArray(int* nums, int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", nums[i]);
    }
    printf("\n");
}

void test_basic_sort() {
    int nums[] = {5, 2, 9, 1, 5, 6};
    int size = sizeof(nums) / sizeof(nums[0]);

    bubbleSort(nums, size);

    printf("Sorted: ");
    printArray(nums, size);
}