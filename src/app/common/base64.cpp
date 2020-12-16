#include "base64.h"

const char base64::encode_character_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const char base64::decode_character_table[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,
    -1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,-1,0,
    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
    -1,-1,-1,-1,-1,-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,
    42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

base64::base64()
{
}

base64::~base64()
{
}

void base64::encode(std::istream &in, std::ostringstream &out)
{
    char buff1[3];
    char buff2[4];
    int i = 0, j;
    while (in.readsome(&buff1[i++], 1)) {
        if (i == 3) {
            out << encode_character_table[(buff1[0] & 0xfc) >> 2];
            out	<< encode_character_table[((buff1[0] & 0x03) << 4) + ((buff1[1] & 0xf0) >> 4)];
            out	<< encode_character_table[((buff1[1] & 0x0f) << 2) + ((buff1[2] & 0xc0) >> 6)];
            out	<< encode_character_table[buff1[2] & 0x3f];
            i = 0;
        }
    }

    if (--i) {
        for (j = i; j < 3; j++)
            buff1[j] = '\0';

        buff2[0] = (buff1[0] & 0xfc) >> 2;
        buff2[1] = ((buff1[0] & 0x03) << 4) + ((buff1[1] & 0xf0) >> 4);
        buff2[2] = ((buff1[1] & 0x0f) << 2) + ((buff1[2] & 0xc0) >> 6);
        buff2[3] = buff1[2] & 0x3f;

        for (j = 0; j < (i + 1); j++)
            out << encode_character_table[(int)buff2[j]];

        while (i++ < 3)
            out << '=';
    }

}

void base64::decode(std::istringstream &in, std::ostream &out)
{
    char buff1[4];
    char buff2[4];
    int i = 0, j;

    while (in.readsome(&buff2[i], 1) && buff2[i] != '=') {
        if (++i == 4) {
            for (i = 0; i != 4; i++)
                buff2[i] = decode_character_table[(int)buff2[i]];

            out << (char)((buff2[0] << 2) + ((buff2[1] & 0x30) >> 4));
            out << (char)(((buff2[1] & 0xf) << 4) + ((buff2[2] & 0x3c) >> 2));
            out << (char)(((buff2[2] & 0x3) << 6) + buff2[3]);

            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            buff2[j] = '\0';
        for (j = 0; j < 4; j++)
            buff2[j] = decode_character_table[(int)buff2[j]];

        buff1[0] = (buff2[0] << 2) + ((buff2[1] & 0x30) >> 4);
        buff1[1] = ((buff2[1] & 0xf) << 4) + ((buff2[2] & 0x3c) >> 2);
        buff1[2] = ((buff2[2] & 0x3) << 6) + buff2[3];

        for (j = 0; j < (i - 1); j++)
            out << (char)buff1[j];
    }
}

