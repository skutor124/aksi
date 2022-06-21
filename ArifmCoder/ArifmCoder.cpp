#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <queue>
#include <unordered_map>

// Number of bits in a code value
#define code_value_bits 16



    unsigned int file_full_size = 0;
    unsigned int commpres_size = 0;
    struct stat sb {};
    struct stat se {};

    // Finding compression ratio
    if (!stat(input_name, &sb)) {
        file_full_size = sb.st_size;
    }
    else {
        perror("stat");
    }
    if (!stat(output_name, &se)) {
        commpres_size = se.st_size;
    }
    else {
        perror("stat");
    }

    return (commpres_size + 0.0) / file_full_size;
}

int main() {
    std::cout << coder() << std::endl; 
}