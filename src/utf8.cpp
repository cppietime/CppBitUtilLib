/*
utf8.cpp
Convert to UTF8
*/

// #include <iomanip>
// #include <sstream>
// #include <string>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include  "bitutil.hpp"

size_t BitManip::utf8(std::uint32_t v, std::uint8_t *dst)
{
    size_t bytes;
    std::uint8_t mask;
    if (v < (1 << 7)) {
        bytes = 1;
        mask = 0;
    }
    else if (v < (1 << 11)) {
        bytes = 2;
        mask = 0xC0;
    }
    else if (v < (1 << 16)) {
        bytes = 3;
        mask = 0xE0;
    }
    else if (v < (1 << 21)) {
        bytes = 4;
        mask = 0xF0;
    }
    else if (v < (1 << 26)) {
        bytes = 5;
        mask = 0xF8;
    }
    else {
        bytes = 6;
        mask = 0xFC;
    }
    for (size_t i = bytes - 1; i != 0; i--) {
        dst[i] = 0x80 | (v & 0x3F);
        v >>= 6;
    }
    dst[0] = mask | v;
    return bytes;
}

size_t BitManip::utf8(std::uint8_t *src, std::uint32_t &v)
{
    std::uint8_t id = *src;
    size_t bytes = utf8BytesLeft(id) + 1;
    if (bytes == 1) {
        v = id;
        return 1;
    }
    if (bytes > UTF8_MAX_LEN) {
        return 0;
    }
    v = 0;
    for (size_t i = bytes - 1; i != 0; i--) {
        if ((src[i] & 0xC0) != 0x80) {
            return 0;
        }
        v |= (0x3F & src[i]) << (6 * (bytes - 1 - i));
    }
    v |= (id & ((1 << (7 - bytes)) - 1)) << (6 * (bytes - 1));
    return bytes;
}

// int main(int argc, char **argv)
// {
    // constexpr size_t num = 10;
    // std::uint32_t symbols[num] = {
        // 0x00,
        // 0x7F,
        // 0x80,
        // 0x4000000,
        // 0x800,
        // 0x1234,
        // 0x5432,
        // 0x897867,
        // 0x100,
        // 0x64
    // };
    // std::stringstream ss;
    // BitBuffer::BitBufferOut bbo(ss);
    // for (size_t i = 0; i < num; i++) {
        // bbo.writeUtf8(symbols[i]);
    // }
    // bbo.flush();
    // ss.seekg(0);
    // std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0');
    // std::string str = ss.str();
    // BitBuffer::BitBufferIn bbi(ss);
    // for (size_t i = 0; i < num; i++) {
        // std::uint32_t sym = bbi.readUtf8();
        // std::cout << sym << std::endl;
    // }
    // return 0;
// }
