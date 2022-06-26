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

int fget_bit(unsigned char* input_bit, unsigned int* bit_len, FILE* input, unsigned int* useless_bit)//returns bits from a file
{
    if ((*bit_len) == 0)//if there are no written bits
    {
        (*input_bit) = fgetc(input);//then we read the new symbol
        if (feof(input))//if the file is over, then we write down useless bits
        {
            (*useless_bit)++;
            if ((*useless_bit) > 14)
            {
                throw invalid_argument("Can't decompress");
            }
        }
        (*bit_len) = 8;
    }
    int result = (*input_bit) & 1;//return last bit
    (*input_bit) >>= 1;
    (*bit_len)--;
    return result;
}

void decoder(const char* input_name = "encoded.txt",
    const char* output_name = "output.txt") {
    unsigned int* alfabet = new unsigned int[256];
    for (int i = 0; i < 256; i++) {
        alfabet[i] = 0;
    }
    FILE* input_file = fopen(input_name, "rb");  // Open input file
    if (input_file == nullptr) {
        throw std::invalid_argument("File not found.");
    }

    unsigned char col = 0;
    unsigned int col_letters = 0;
    col = fgetc(input_file);
    if (!feof(input_file)) {
        col_letters = (unsigned int)col;
    }

    unsigned char character = 0;
    // Reading the letters used and their freq
    for (int i = 0; i < col_letters; i++) {
        character = fgetc(input_file);
        if (!feof(input_file)) {
            fread(reinterpret_cast<char*>(&alfabet[character]),
                sizeof(unsigned short), 1, input_file);
        }
        else {
            throw invalid_argument("Can't decompress file.");
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
    cout << "------Alphabet------" << endl;
    for (auto pair : vec)
    {
        cout << pair.first << " " << pair.second << endl;
    }

    unsigned short* ranges = new unsigned short[vec.size() + 2];//table of intervals
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

    FILE* output = fopen(output_name, "wb +");
    if (output == nullptr)
    {
        throw invalid_argument("File not found.");
    }

    for (int i = 1; i <= 16; i++)
    {
        tmp = fget_bit(&input_bit, &bit_len, input_file, &useless_bit);//get code value
        code_value = 2 * code_value + tmp;
    }
    unsigned int diff = high_value - low_value + 1;
    for (;;)
    {
        unsigned int freq = static_cast<unsigned int>(((static_cast<unsigned int>(code_value) - low_value + 1) * divider - 1) / diff);//calc freq

        int j;

        for (j = 1; ranges[j] <= freq; j++) {}//find the symbol in the interval table
        high_value = low_value + ranges[j] * diff / divider - 1;//calc borders
        low_value = low_value + ranges[j - 1] * diff / divider;

        for (;;)
        {
            if (high_value < half) {}//if  upper bound in 1 half then do nothing

            else if (low_value >= half)//if lower boundary in the 2nd half, then shift the value of the boundaries and the value of the code by half
            {
                low_value -= half;
                high_value -= half;
                code_value -= half;
            }
            else if ((low_value >= first_qtr) && (high_value < third_qtr))//if both borders in the second quarter, then shift by a quarter
            {
                low_value -= first_qtr;
                high_value -= first_qtr;
                code_value -= first_qtr;
            }
            else
            {
                break;
            }

            low_value += low_value;//then change the border and values
            high_value += high_value + 1;
            tmp = 0;
            tmp = fget_bit(&input_bit, &bit_len, input_file, &useless_bit);
            code_value += code_value + tmp;
        }

        if (j == 1)//finish when reach the end of the array of intervals
        {
            break;
        }

        fputc(vec[j - 2].first, output);//writing decoded character
        diff = high_value - low_value + 1;
    }
    cout << "decoding correct\n";
    fclose(input_file);
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
    if (!checker()) {
        std::cout << "Files match" << std::endl;
    }
    else {
        std::cout << "Files doesn't match" << std::endl;
    }
}