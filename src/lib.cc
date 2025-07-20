#include <iostream>

extern "C" [[gnu::used]] [[gnu::retain]] int32_t input()
{
    std::cout << "input: ";
    int value;
    std::cin >> value;
    return value;
}
