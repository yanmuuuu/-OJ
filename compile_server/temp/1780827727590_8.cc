#include <bits/stdc++.h>

using namespace std;

class Solution {
public:
    vector<int> twoSum(vector<int>& nums, int target) {
        //code
    }
};
#ifndef COMPILER_ONLINE
#include "head.cpp"
#endif

void test1() {
    Solution s;
    vector<int> nums = {2, 7, 11, 15};
    int target = 9;
    vector<int> result = s.twoSum(nums, target);
    
    bool ok = (result.size() == 2 && result[0] != result[1] && nums[result[0]] + nums[result[1]] == target);
    if (ok) {
        cout << "test1 passed" << endl;
    } else {
        cout << "test1 not passed" << endl;
    }
}

void test2() {
    Solution s;
    vector<int> nums = {3, 3};
    int target = 6;
    vector<int> result = s.twoSum(nums, target);
    
    bool ok = (result.size() == 2 && result[0] != result[1] && nums[result[0]] + nums[result[1]] == target);
    if (ok) {
        cout << "test2 passed" << endl;
    } else {
        cout << "test2 not passed" << endl;
    }
}

void test3() {
    Solution s;
    vector<int> nums = {-1, -2, -3, -4, -5};
    int target = -8;
    vector<int> result = s.twoSum(nums, target);
    
    bool ok = (result.size() == 2 && result[0] != result[1] && nums[result[0]] + nums[result[1]] == target);
    if (ok) {
        cout << "test3 passed" << endl;
    } else {
        cout << "test3 not passed" << endl;
    }
}

void test4() {
    Solution s;
    vector<int> nums = {0, 4, 3, 0};
    int target = 0;
    vector<int> result = s.twoSum(nums, target);
    
    bool ok = (result.size() == 2 && result[0] != result[1] && nums[result[0]] + nums[result[1]] == target);
    if (ok) {
        cout << "test4 passed" << endl;
    } else {
        cout << "test4 not passed" << endl;
    }
}

void test5() {
    Solution s;
    vector<int> nums = {1000000, 2000000, 3000000, 4000000};
    int target = 5000000;
    vector<int> result = s.twoSum(nums, target);
    
    bool ok = (result.size() == 2 && result[0] != result[1] && nums[result[0]] + nums[result[1]] == target);
    if (ok) {
        cout << "test5 passed" << endl;
    } else {
        cout << "test5 not passed" << endl;
    }
}

int main() {
    test1();
    test2();
    test3();
    test4();
    test5();
    return 0;
}
