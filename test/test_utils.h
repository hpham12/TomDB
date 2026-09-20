//
// Created by Hieu Pham on 9/20/26.
//

#ifndef TOMDB_TEST_UTILS_H
#define TOMDB_TEST_UTILS_H
#include <filesystem>
#include <random>
#include <string>

inline std::string generateRandomFilePath() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution distr(1, 10000);
    int randomNum = distr(gen);

    return std::filesystem::temp_directory_path() / ("tomdb_test" + std::to_string(randomNum) + ".data");
}

#endif //TOMDB_TEST_UTILS_H
