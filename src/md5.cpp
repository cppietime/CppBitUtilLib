/*
md5.cpp
*/

#include <iostream>
#include <iomanip>
#include <sstream>

#include <cstdint>
#include <algorithm>
// #include <endian.h>
#include "bitutil.hpp"

#define MD5_SIN_SIZE 64
static const std::uint32_t SIN[MD5_SIN_SIZE] = {
     3614090360, 3905402710,  606105819, 3250441966, 4118548399, 1200080426,
     2821735955, 4249261313, 1770035416, 2336552879, 4294925233, 2304563134,
     1804603682, 4254626195, 2792965006, 1236535329, 4129170786, 3225465664,
      643717713, 3921069994, 3593408605,   38016083, 3634488961, 3889429448,
      568446438, 3275163606, 4107603335, 1163531501, 2850285829, 4243563512,
     1735328473, 2368359562, 4294588738, 2272392833, 1839030562, 4259657740,
     2763975236, 1272893353, 4139469664, 3200236656,  681279174, 3936430074,
     3572445317,   76029189, 3654602809, 3873151461,  530742520, 3299628645,
     4096336452, 1126891415, 2878612391, 4237533241, 1700485571, 2399980690,
     4293915773, 2240044497, 1873313359, 4264355552, 2734768916, 1309151649,
     4149444226, 3174756917,  718787259, 3951481745
};

static const int SHIFTS[MD5_SIN_SIZE] = {
    7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
    5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
    4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
    6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};

void Digest::MD5Context::processBuffer()
{
    // std::cout << "Processing buffer ";
    // for (size_t i = 0; i < 16; i++) {
        // std::cout << buffer[i] << ", ";
    // }
    // std::cout << std::endl;
    std::uint32_t a1 = a, b1 = b, c1 = c, d1 = d;
    for (int i = 0; i < 64; i++) {
        int quarter = i >> 4;
        std::uint32_t f;
        size_t g;
        switch (quarter) {
            case 0:
                f = (b1 & c1) | ((~b1) & d1);
                g = i;
                break;
            case 1:
                f = (d1 & b1) | ((~d1) & c1);
                g = (5 * i + 1) & 15;
                break;
            case 2:
                f = b1 ^ c1 ^ d1;
                g = (3 * i + 5) & 15;
                break;
            case 3:
                f = c1 ^ (b1 | ~d1);
                g = (7 * i) & 15;
                break;
        }
        f += a1 + SIN[i] + buffer[g];
        a1 = d1;
        d1 = c1;
        c1 = b1;
        b1 += (f << SHIFTS[i]) | (f >> (32 - SHIFTS[i]));
        // std::cout << a1 << ' ' << b1 << ' ' << c1 << ' ' << d1 << ' ' << f << std::endl;
    }
    a += a1;
    b += b1;
    c += c1;
    d += d1;
    std::fill(buffer, buffer + MD5_BUFFER_SIZE, 0);
}

void Digest::MD5Context::consume(const std::uint8_t *data, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        operator<<(data[i]);
    }
}

Digest::MD5Context& Digest::MD5Context::operator<<(std::uint8_t byte)
{
    bytesProcessed++;
    buffer[bufferIndex >> 2] |= byte << (8 * (bufferIndex & 3));
    bufferIndex++;
    if (bufferIndex == MD5_BUFFER_SIZE * 4) {
        bufferIndex = 0;
        processBuffer();
    }
    return *this;
}

std::vector<std::uint8_t> Digest::MD5Context::finalize()
{
    // std::uint64_t bits = htole64(bytesProcessed << 3);
    std::uint8_t bits[8] = {
        (std::uint8_t)(bytesProcessed << 3),
        (std::uint8_t)(bytesProcessed >> 5),
        (std::uint8_t)(bytesProcessed >> 13),
        (std::uint8_t)(bytesProcessed >> 21),
        (std::uint8_t)(bytesProcessed >> 29),
        (std::uint8_t)(bytesProcessed >> 37),
        (std::uint8_t)(bytesProcessed >> 45),
        (std::uint8_t)(bytesProcessed >> 53)
    };
    operator<<(0x80);
    while (bufferIndex != 56) {
        operator<<(0x00);
    }
    consume(bits, sizeof(std::uint64_t));
    // std::uint32_t words[4] = {
        // htole32(a),
        // htole32(b),
        // htole32(c),
        // htole32(d)
    // };
    // const std::uint8_t *asBytes = reinterpret_cast<const std::uint8_t*>(words);
    // return std::vector<std::uint8_t>(asBytes, asBytes + 16);
    return {
        (std::uint8_t)(a >> 0), (std::uint8_t)(a >> 8), (std::uint8_t)(a >> 16), (std::uint8_t)(a >> 24),
        (std::uint8_t)(b >> 0), (std::uint8_t)(b >> 8), (std::uint8_t)(b >> 16), (std::uint8_t)(b >> 24),
        (std::uint8_t)(c >> 0), (std::uint8_t)(c >> 8), (std::uint8_t)(c >> 16), (std::uint8_t)(c >> 24),
        (std::uint8_t)(d >> 0), (std::uint8_t)(d >> 8), (std::uint8_t)(d >> 16), (std::uint8_t)(d >> 24),
    };
}

// int main()
// {
    // Digest::MD5Context md5;
    // const char *str = "Hello";
    // for (size_t i = 0; i < 20; i++)
        // md5.consume(str, 5);
    // auto digest = md5.finalize();
    // std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0');
    // for (auto it = digest.begin(); it != digest.end(); it++) {
        // std::cout << (int)*it << ' ';
    // }
    // std::stringstream strs;
    // BitBuffer::BitBufferOut bbo(strs);
    // std::cout << std::endl;
    // bbo << digest;
    // bbo << (std::uint32_t)digest.size();
    // bbo.flush();
    // auto string = strs.str();
    // for (int i = 0; i < string.size(); i++) {
        // std::cout << (int)string.data()[i] << ", ";
    // }
    // std::cout << std::endl;
// }
