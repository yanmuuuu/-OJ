#ifndef COMPILER_ONLINE
#include "head.cpp"
#endif

using namespace std;

// 辅助函数：运行单个测试
void runTest(int testId, const string& s, int expected) {
    Solution sol;
    int result = sol.lengthOfLongestSubstring(s);
    bool ok = (result == expected);
    if (ok) {
        cout << "test" << testId << " passed" << endl;
    } else {
        cout << "test" << testId << " not passed (expected " << expected << ", got " << result << ")" << endl;
    }
}

void test1() {
    // 示例1: 基本功能，无重复
    runTest(1, "abcabcbb", 3);
}

void test2() {
    // 示例2: 全部相同字符
    runTest(2, "bbbbb", 1);
}

void test3() {
    // 示例3: 带空格和标点
    runTest(3, "pwwkew", 3);
}

void test4() {
    // 空字符串
    runTest(4, "", 0);
}

void test5() {
    // 单字符
    runTest(5, "a", 1);
}

void test6() {
    // 全部不同字符
    runTest(6, "abcdef", 6);
}

void test7() {
    // 重复字符在两端
    runTest(7, "abba", 2);
}

void test8() {
    // 复杂重复模式
    runTest(8, "dvdf", 3);
}

void test9() {
    // 包含数字和字母
    runTest(9, "tmmzuxt", 5);
}

void test10() {
    // 很长字符串，确保性能（可选，OJ 通常不测这个）
    // 这里只测一个简单情况
    string longStr(10000, 'a');
    runTest(10, longStr, 1);
}

int main() {
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();
    return 0;
}