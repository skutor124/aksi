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
//returns the index of the symbol
int indexOFsymbol(char symbol, vector<pair<char, unsigned int>> vec)
{
    for (int i = 0; i < vec.size(); i++)
    {
        if (symbol == vec[i].first)
        {
            return i + 2;
        }
    }

    return -1;
}
//puts a bit in the file
void fput_bit(unsigned int bit, unsigned int* bit_len, unsigned char* file_bit, FILE* output)
{
    (*file_bit) = (*file_bit) >> 1;
    if (bit) (*file_bit) |= (1 << 7);

    (*bit_len)--;

    if ((*bit_len) == 0)
    {
        fputc((*file_bit), output);
        (*bit_len) = 8;
    }
}
//the function enters a sequence of bits into a file
void bitsANDfollow(unsigned int bit, unsigned int* follow_bits, unsigned int* bit_len, unsigned char* write_bit, FILE* output_file)
{
    fput_bit(bit, bit_len, write_bit, output_file);

    for (; *follow_bits > 0; (*follow_bits)--)
    {
        fput_bit(!bit, bit_len, write_bit, output_file);
    }
}
//coder function
void coder(const char* input_text = "input.txt", const char* output_text = "encoded.txt")
{
    uint64_t* alfabet = new uint64_t[256];
    for (int i = 0; i < 256; i++) {
        alfabet[i] = 0;
    }

    FILE* input_file = fopen(input_text, "rb");
    if (input_file == nullptr) {
        throw std::invalid_argument("File not found.");
    }

    unsigned char character = 0;

    while (!feof(input_file))// Read from input file
    {
        character = fgetc(input_file);
        if (!feof(input_file))
        {
            alfabet[character]++;
        }
    }

    fclose(input_file);
    //creating a vector: pair - symbol, freq
    vector<pair<char, unsigned int>> vec;
    for (int i = 0; i < 256; i++)
    {
        if (alfabet[i] != 0)
        {
            vec.push_back(make_pair(static_cast<char>(i), alfabet[i]));//for all the characters that are in the text, fill in these pairs
        }
    }
    //sort them
    sort(vec.begin(), vec.end(), [](const pair<char, unsigned int>& left, const pair<char, unsigned int>& right)
        {
            if (left.second != right.second)//if frq = then cmp
            {
                return left.second >= right.second;
            }

            return left.first < right.first;//symbols
        });//have a sorted table
    cout << "------Alphabet------" << endl;
    for (auto pair : vec)
    {
        cout << pair.first << " " << pair.second << endl;
    }


    unsigned short* ranges = new unsigned short[vec.size() + 2];//creating int array with intervals
    ranges[0] = 0;
    ranges[1] = 1;
    for (int i = 0; i < vec.size(); i++)
    {
        unsigned int b = vec[i].second;
        for (int j = 0; j < i; j++)
        {
            b += vec[j].second;
        }
        ranges[i + 2] = b;
    }

    if (ranges[vec.size()] > (1 << ((code_value_bits - 2)) - 1))//the values are not higher than numbers otherwise we will not be able to encode
    {
        puts("ArCoder ERROR: Symbols frequencies are too long");
        exit(1);
    }

    input_file = fopen(input_text, "rb");
    FILE* output_file = fopen(output_text, "wb +");

    char count_letters = vec.size();//remember the number of simols used, necessary for the header
    fputc(count_letters, output_file);

    // Writing the letters used and their freq, necessary for the header
    for (int i = 0; i < 256; i++) {
        if (alfabet[i] != 0) {
            fputc(static_cast<char>(i), output_file);
            fwrite(reinterpret_cast<const char*>(&alfabet[i]),
                sizeof(unsigned short), 1, output_file);
        }
    }

    unsigned int low_value = 0;
    unsigned int high_value = ((static_cast<unsigned int>(1) << code_value_bits) - 1);

    unsigned int divider = ranges[vec.size() + 1];
    unsigned int diff = high_value - low_value + 1;
    unsigned int first_qtr = (high_value + 1) / 4;
    unsigned int half = first_qtr * 2;
    unsigned int third_qtr = first_qtr * 3;

    unsigned int follow_bits = 0;
    unsigned int bit_len = 8;
    unsigned char write_bit = 0;

    int j = 0;

    

    // Compressing the file
    while (!feof(input_file))
    {
        character = fgetc(input_file);

        if (!feof(input_file))
        {
            j = indexOFsymbol(character, vec);//get symbol index

            high_value = low_value + ranges[j] * diff / divider - 1;//calc new low and high bord
            low_value = low_value + ranges[j - 1] * diff / divider;

            for (;;)
            {
                if (high_value < half)//if the high bound in 1 half then write 0
                {
                    bitsANDfollow(0, &follow_bits,
                        &bit_len, &write_bit, output_file);
                }
                else if (low_value >= half)//if the low bound in 2 half then write 1
                {
                    bitsANDfollow(1, &follow_bits,
                        &bit_len, &write_bit, output_file);
                    low_value -= half;//change borders
                    high_value -= half;
                }
                else if ((low_value >= first_qtr) && (high_value < third_qtr))//if both boundaries in 2 quarters then write the bit conditionally
                {
                    follow_bits++;//change borders
                    low_value -= first_qtr;
                    high_value -= first_qtr;
                }
                else
                {
                    break;
                }

                low_value += low_value;//and here change borders
                high_value += high_value + 1;
            }
        }
        //last symb
        else
        {
            high_value = low_value + ranges[1] * diff / divider - 1;
            low_value = low_value + ranges[0] * diff / divider;

            for (;;)
            {
                if (high_value < half)
                {
                    bitsANDfollow(0, &follow_bits,
                        &bit_len, &write_bit, output_file);
                }
                else if (low_value >= half)
                {
                    bitsANDfollow(1, &follow_bits,
                        &bit_len, &write_bit, output_file);
                    low_value -= half;
                    high_value -= half;
                }
                else if ((low_value >= first_qtr) && (high_value < third_qtr))
                {
                    follow_bits++;
                    low_value -= first_qtr;
                    high_value -= first_qtr;
                }
                else
                {
                    break;
                }

                low_value += low_value;
                high_value += high_value + 1;
            }

            follow_bits++;
            //writing the final bit to a file,
            if (low_value < first_qtr)//if the lower bound in 1 quarter then 0,
            {
                bitsANDfollow(0, &follow_bits,
                    &bit_len, &write_bit, output_file);
            }
            else
            {
                bitsANDfollow(1, &follow_bits,//
                    &bit_len, &write_bit, output_file);
            }

            write_bit >>= bit_len;// write remaining
            fputc(write_bit, output_file);
        }
        diff = high_value - low_value + 1;
    }
    cout << "coding correct\n";
    fclose(input_file);
    fclose(output_file);
}

// Finding compression ratio
float compressRatio(const char* input_name = "input.txt", const char* output_name = "encoded.txt") {
    uint64_t file_full_size = 0;
    uint64_t compress_size = 0;

    struct stat sb {};
    struct stat se {};

    if (!stat(input_name, &sb)) {
        file_full_size = sb.st_size;
    }
    else {
        perror("STAT");
    }
    if (!stat(output_name, &se)) {
        compress_size = se.st_size;
    }
    else {
        perror("STAT");
    }

    //std::cout << "Compress ratio is: " << (compress_size + 0.0) / file_full_size << "\n";
    return (compress_size + 0.0) / file_full_size;
}

int main() {
    coder();
    std::cout << "Compress ratio is: " << compressRatio() << std::endl;
}