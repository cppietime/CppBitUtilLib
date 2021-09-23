/*
bitbuffer.cpp
*/

#include <iostream>
#include <sstream>
#include <cstdint>
#include <map>
#include "bitutil.hpp"

BitBuffer::BitBufferOut::~BitBufferOut()
{
    flush();
}

void BitBuffer::BitBufferOut::reset()
{
    index = 0;
    building = 0;
}

void BitBuffer::BitBufferOut::push()
{
    if (order == LSB) {
        building = ((building & 0xF0) >> 4) | ((building & 0x0F) << 4);
        building = ((building & 0xCC) >> 2) | ((building & 0x33) << 2);
        building = ((building & 0xAA) >> 1) | ((building & 0x55) << 1);
    }
    stream.write(reinterpret_cast<const char*>(&building), sizeof(BITBUFFER_T));
}

size_t BitBuffer::BitBufferOut::write(std::uint32_t value, size_t bits)
{
    if (bits > 32) {
        throw BitBufferException("bit count too high");
    }
    int written = 0;
    while (bits) {
        size_t bitsToAppend = std::min(sizeof(BITBUFFER_T) * 8 - index, bits);
        size_t shift = bits - bitsToAppend;
        std::uint32_t mask = ((std::uint32_t{1} << bitsToAppend) - 1);
        building <<= bitsToAppend;
        building |= (value >> shift) & mask;
        index += bitsToAppend;
        if (index == sizeof(BITBUFFER_T) * 8) {
            written++;
            push();
            index = 0;
        }
        bits -= bitsToAppend;
    }
    return written;
}


size_t BitBuffer::BitBufferOut::writeData(const unsigned char *mem, size_t bytes)
{
    size_t written = 0;
    for (size_t byte = 0; byte < bytes; byte++) {
        written += write(*mem++, 8);
    }
    return written;
}

size_t BitBuffer::BitBufferOut::flush(bool fill)
{
    if (index == 0) {
        return 0;
    }
    size_t remaining = 8 * sizeof(BITBUFFER_T) - index;
    building <<= remaining;
    if (fill) {
        building |= (1 << remaining) - 1;
    }
    push();
    return 1;
}

void BitBuffer::BitBufferIn::fetch()
{
    stream.read(reinterpret_cast<char*>(&building), sizeof(BITBUFFER_T));
    if (order == LSB) {
        building = BitManip::reverse8(building);
    }
}

std::uint32_t BitBuffer::BitBufferIn::read(size_t bits)
{
    if (bits > 32) {
        throw BitBufferException("bit count too high");
    }
    std::uint32_t val = 0;
    while (bits) {
        if (index == sizeof(BITBUFFER_T) * 8) {
            fetch();
            index = 0;
        }
        size_t remaining = std::min(sizeof(BITBUFFER_T) * 8 - index, bits);
        size_t shift = sizeof(BITBUFFER_T) * 8 - index - remaining;
        val <<= remaining;
        std::uint32_t mask = (std::uint32_t{1} << remaining) - 1;
        mask <<= shift;
        val |= (building & mask) >> shift;
        index += remaining;
        bits -= remaining;
    }
    return val;
}

size_t BitBuffer::BitBufferIn::read(unsigned char *mem, size_t bytes)
{
    for (size_t i = 0; i < bytes; i++) {
        mem[i] = read(8);
    }
    return bytes;
}

const char* BitBuffer::BitBufferException::what()
{
    return ("BitBuffer Exception: " + message).c_str();
}

// int main()
// {
    // std::map<int, int> freqs;
    // freqs[0] = 20;
    // freqs[1] = 5;
    // freqs[2] = 12;
    // freqs[3] = 1;
    // freqs[4] = 8;
    // Huffman::HuffmanCode code(freqs);
    // std::stringstream stream;
    // BitBuffer::BitBufferOut bout(stream, BitBuffer::LSB);
    // const int message[6] = {0, 4, 2, 3, 1, 4};
    // for (size_t i = 0; i < 6; i++) {
        // code.write(message[i], bout);
    // }
    // bout.flush();
    // stream.seekg(0);
    // std::cout << "State: " << stream.rdstate() << std::endl;
    // std::string str = stream.str();
    // for (size_t i = 0; i < str.size(); i++) {
        // std::cout << i << ": " << (int)(unsigned char)str.data()[i] << std::endl;
    // }
    // BitBuffer::BitBufferIn bin(stream, BitBuffer::LSB);
    // int sym;
    // for (size_t i = 0; i < 6; i++) {
        // if (code.read(bin, sym)) {
            // std::cout << sym << std::endl;
        // } else {
            // std::cout << "No code found...\n";
        // }
    // }
    // return 0;
// }