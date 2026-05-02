#include "basic_sort.h"

/* 交换函数 */
static void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

/* 冒泡排序 */
void bubbleSort(int* nums, int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (nums[j] > nums[j + 1]) {
                swap(&nums[j], &nums[j + 1]);
            }
        }
    }
}

/* 选择排序 */
void selectionSort(int* nums, int size) {
    for (int i = 0; i < size - 1; i++) {
        int minIndex = i;

        for (int j = i + 1; j < size; j++) {
            if (nums[j] < nums[minIndex]) {
                minIndex = j;
            }
        }

        swap(&nums[i], &nums[minIndex]);
    }
}

/* 插入排序 */
void insertionSort(int* nums, int size) {
    for (int i = 1; i < size; i++) {
        int key = nums[i];
        int j = i - 1;

        while (j >= 0 && nums[j] > key) {
            nums[j + 1] = nums[j];
            j--;
        }

        nums[j + 1] = key;
    }
}