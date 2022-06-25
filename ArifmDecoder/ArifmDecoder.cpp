#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sys/stat.h>

using namespace std;

#define code_value_bits 16

int fget_bit(unsigned char* input_bit, unsigned int* bit_len, FILE* input, unsigned int* useless_bit)
{
    if ((*bit_len) == 0)
    {
        (*input_bit) = fgetc(input);
        if (feof(input))
        {
            (*useless_bit)++;
            if ((*useless_bit) > 14)
            {
                puts("ArDecoder ERROR: Does not possible to decode\n");
                exit(1);
            }
        }
        (*bit_len) = 8;
    }
    int result = (*input_bit) & 1;
    (*input_bit) >>= 1;
    (*bit_len)--;
    return result;
}

void decoder(const char* input_text = "encoded.txt", const char* output_text = "output.txt")
{
    unsigned int* alfabet = new unsigned int[256];
    for (int i = 0; i < 256; i++)
    {
        alfabet[i] = 0;
    }

    FILE* input = fopen(input_text, "rb");
    if (input == nullptr)
    {
        puts("ArDecoder ERROR: No such file or directory\n");
        exit(1);
    }

    unsigned char col = 0;
    unsigned int col_letters = 0;
    col = fgetc(input);
    if (!feof(input))
    {
        col_letters = static_cast<unsigned int>(col);
    }

    unsigned char character = 0;

    for (int i = 0; i < col_letters; i++)
    {
        character = fgetc(input);
        if (!feof(input))
        {
            fread(reinterpret_cast<char*>(&alfabet[character]), sizeof(unsigned short), 1, input);
        }
        else
        {
            puts("ArDecoder ERROR: Does not possible to decode\n");
            exit(1);
        }
    }

    vector<pair<char, unsigned int>> vec;
    for (int i = 0; i < 256; i++)
    {
        if (alfabet[i] != 0)
        {
            vec.push_back(make_pair(static_cast<char>(i), alfabet[i]));
        }
    }

    sort(vec.begin(), vec.end(), [](const pair<char, unsigned int>& left, const pair<char, unsigned int>& right)
        {
            if (left.second != right.second)
            {
                return left.second >= right.second;
            }
            return left.first < right.first;
        });


    unsigned short* ranges = new unsigned short[vec.size() + 2];
    ranges[0] = 0;
    ranges[1] = 1;
    for (int i = 0; i < vec.size(); i++) {
        unsigned int b = vec[i].second;
        for (int j = 0; j < i; j++) {
            b += vec[j].second;
        }
        ranges[i + 2] = b;
    }

    if (ranges[vec.size()] > (1 << ((code_value_bits - 2)) - 1))
    {
        puts("ArDecoder ERROR: Symbols frequencies are too long\n");
        exit(1);
    }

    unsigned int low_value = 0;
    unsigned int high_value = ((static_cast<unsigned int>(1) << code_value_bits) - 1);
    unsigned int divider = ranges[vec.size() + 1];
    unsigned int first_qtr = (high_value + 1) / 4;
    unsigned int half = first_qtr * 2;
    unsigned int third_qtr = first_qtr * 3;

    unsigned int bit_len = 0;
    unsigned char input_bit = 0;
    unsigned int useless_bit = 0;
    unsigned short code_value = 0;
    int tmp = 0;

    FILE* output = fopen(output_text, "wb +");
    if (output == nullptr)
    {
        puts("ArDecoder ERROR: Does not open output file\n");
        exit(1);
    }

    for (int i = 1; i <= 16; i++)
    {
        tmp = fget_bit(&input_bit, &bit_len, input, &useless_bit);
        code_value = 2 * code_value + tmp;
    }
    unsigned int diff = high_value - low_value + 1;
    for (;;)
    {
        unsigned int freq = static_cast<unsigned int>(((static_cast<unsigned int>(code_value) - low_value + 1) * divider - 1) / diff);

        int j;

        for (j = 1; ranges[j] <= freq; j++) {}
        high_value = low_value + ranges[j] * diff / divider - 1;
        low_value = low_value + ranges[j - 1] * diff / divider;

        for (;;)
        {
            if (high_value < half) {}
            else if (low_value >= half)
            {
                low_value -= half;
                high_value -= half;
                code_value -= half;
            }
            else if ((low_value >= first_qtr) && (high_value < third_qtr))
            {
                low_value -= first_qtr;
                high_value -= first_qtr;
                code_value -= first_qtr;
            }
            else
            {
                break;
            }

            low_value += low_value;
            high_value += high_value + 1;
            tmp = 0;
            tmp = fget_bit(&input_bit, &bit_len, input, &useless_bit);
            code_value += code_value + tmp;
        }

        if (j == 1)
        {
            break;
        }

        fputc(vec[j - 2].first, output);
        diff = high_value - low_value + 1;
    }

    fclose(input);
    fclose(output);
}

// Checking for file matches
unsigned int checker(const char* before_name = "input.txt",
    const char* after_name = "output.txt") {
    unsigned int same = 0;
    FILE* before_file = fopen(before_name, "r");
    FILE* after_file = fopen(after_name, "r");

    unsigned char after_l = 0;
    unsigned char before_l = 0;
    while (!feof(after_file) && !feof(before_file)) {
        after_l = fgetc(after_file);
        before_l = fgetc(before_file);
        if (!feof(after_file) && !feof(before_file)) {
            if (after_l != before_l) {
                same++;
            }
        }
    }

    while (!feof(after_file)) {
        after_l = fgetc(after_file);
        if (!feof(after_file)) {
            same++;
        }
    }

    while (!feof(before_file)) {
        before_l = fgetc(before_file);
        if (!feof(before_file)) {
            same++;
        }
    }
    fclose(after_file);
    fclose(before_file);
    return same;
}

int main()
{
    decoder();
    std::cout << checker() << std::endl;
}