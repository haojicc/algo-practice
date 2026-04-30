#include <iostream>
#include <vector>
#include "../array/two_sum.h"

using namespace std;

void test_two_sum() {
    vector<int> nums = {2, 7, 11, 15};
    int target = 9;

    auto result = twoSum(nums, target);

    cout << "[Two Sum] Result: ";
    for (auto i : result) cout << i << " ";
    cout << endl;
}