#include <iostream>

void test1() {
    std::cout << "\033[1m\033[32mPassed: Test 1\033[0m" << std::endl;
}

int main() {
    test1();
}

