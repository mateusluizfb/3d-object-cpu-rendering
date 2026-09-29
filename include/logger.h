#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

template <typename T>
void print(const T& arg) {
    std::cout << arg << std::endl;
}

#endif
