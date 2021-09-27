/*
bitbuffer.hpp
Yaakov Schectman, 2021
A utility for writing bit strings to an ostream
*/

#ifndef _BITBUFFER_HPP
#define _BITBUFFER_HPP

#define BITBUFFER_T unsigned char

#include <iostream>
#include <cstdint>
#include <vector>
#include <utility>
#include <map>
#include <string>
#include <exception>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace BitBuffer {
    
    /*
    An enum to indicate the order of bits. For example, zlib uses LSB first
    */
    enum BitOrder {
        MSB = 0,
        LSB = 1
    };
    
    /*
    A wrapper around an ostream that can perform bitwise writes
    */
    class BitBufferOut {
        private:
            std::ostream& stream;
            BITBUFFER_T building;
            int index;
            BitOrder order;
            void push();
                        
            /* Disallow copying */
            BitBufferOut(const BitBufferOut& other);
            
        public:
            /*
            stream: The ostream this BitBufferOut wraps
            order: The bit order, defaults to MSB first
            */
            BitBufferOut(std::ostream& stream, BitOrder order = MSB) : 
                stream{stream},
                index{0},
                building{0},
                order{order} {}
            
            /*
            Flushes any remaining bits before destructing
            */
            ~BitBufferOut();
            
            /*
            Discard any buffered bits not yet written
            */
            void reset();
            
            /*
            Write an integer, in a specified number of bits, to the buffer
            
            value: The integer to be written
            bits: The number of bits. The low bits of value are written
            
            returns the number of bytes actually written to the underlying stream
            */
            size_t write(std::uint32_t value, size_t bits);
            
            /*
            Write a sequence of bytes from a point in memory
            
            mem: Memory address to start writing from
            bytes: Number of bytes to write
            
            returns the number of bytes actually written to the underlying stream
            */
            size_t writeData(const unsigned char *mem, size_t bytes);
            
            /*
            Write encoded UTF-8
            
            returns the number of bytes written
            */
            size_t writeUtf8(std::uint32_t value);
            
            /*
            Flushes anything left in the buffer
            
            fill: If true, empty space is filled with 1-bits instead of 0-bits
            
            returns the number of bytes actually written to the underlying stream
            */
            size_t flush(bool fill = false);
            
            template <class T>
            inline BitBufferOut& operator<<(std::vector<T> vec)
            {
                writeData(reinterpret_cast<const unsigned char*>(vec.data()), vec.size() * sizeof(T));
                return *this;
            }
            
            template <class T>
            inline BitBufferOut& operator<<(T value)
            {
                write(value, sizeof(T) * 8);
                return *this;
            }
    };
    
    class BitBufferIn {
        private:
            std::istream& stream;
            BITBUFFER_T building;
            int index;
            BitOrder order;
            void fetch();
            
            /* Disallow copying */
            BitBufferIn(const BitBufferIn& other);
        public:
            /*
            stream: Source of bits
            order: Bit order, MSB by default
            */
            BitBufferIn(std::istream& stream, BitOrder order = MSB) :
                stream {stream},
                order {order},
                index {8},
                building {0} {}
            
            /*
            bits: Number of bits to read
            returns up to the 32-bit representation of read bits
            */
            std::uint32_t read(size_t bits);
            
            /*
            mem: Memory to write read data to
            bytes: Number of bytes to read
            */
            size_t read(unsigned char *mem, size_t bytes);
            
            /*
            Reads and returns the following UTF-8 value or throws BitBufferException
            */
            std::uint32_t readUtf8();
    };
    
    /* Thrown when invalid arguments or state arise for bit ops */
    class BitBufferException : public std::exception {
        private:
            std::string message;
        public:
            BitBufferException(std::string message) : message{message} {};
            virtual const char* what();
    };
    
}

/*
Bitwise analyses and manipulation functions
*/
namespace BitManip {
    
    /*
    Count the number of 1-bits in a given number
    
    number: a 32-bit unsigned integer
    
    returns the number of bits set to 1 in number
    */
    inline size_t bitsSet(std::uint32_t number)
    {
        number -= (number >> 1) & 0x55555555;
        number = (number & 0x33333333) + ((number >> 2) & 0x33333333);
        number = (number & 0x0F0F0F0F) + ((number >> 4) & 0x0F0F0F0F);
        number = (number & 0x00FF00FF) + ((number >> 8) & 0x00FF00FF);
        number = (number & 0x0000FFFF) + (number >> 16);
        return number;
    }
    
    /*
    Count the number of contiguous 0-bits starting at MSB
    
    number: a 32-bit unsigned integer
    
    returns the number of leading zeros
    */
    inline size_t leadingZeros(std::uint32_t number)
    {
#if defined(_MSC_VER)
        long mask;
        if (_BitScanReverse(&mask, number))
            return sizeof(std::uint32_t) * 8 - 1 - mask;
        return sizeof(std::uint32_t) * 8;
#elif defined(__GNUC__)
        if (number == 0)
            return sizeof(std::uint32_t) * 8;
        return __builtin_clz(number);
#else
        number |= number >> 16;
        number |= number >> 8;
        number |= number >> 4;
        number |= number >> 2;
        number |= number >> 1;
        return sizeof(std::uint32_t) * 8 - bitsSet(number);
#endif
    }
    
    /*
    Count the number of contiguous 0-bits ending with LSB
    
    number: a 32-bit unsigned integer
    
    returns the number of trailing zeros
    */
    inline size_t trailingZeros(std::uint32_t number)
    {
#if defined(_MSC_VER)
        long mask;
        if (_BitScanForward(&mask, number))
            return mask;
        return sizeof(std::uint32_t) * 8;
#elif defined(__GNUC__)
        if (number == 0)
            return sizeof(std::uint32_t) * 8;
        return __builtin_ctz(number);
#else
        number |= number << 16;
        number |= number << 8;
        number |= number << 4;
        number |= number << 2;
        number |= number << 1;
        return sizeof(std::uint32_t) * 8 - bitsSet(number);
#endif
    }
    
    /*
    Find the position of the most significant 1-bit
    
    number: a 32-bit unsigned integer
    
    returns the 0-index position of the set MSB, or 32 for 0
    */
    inline size_t msbSet(std::uint32_t number)
    {
#if defined(_MSC_VER)
        long mask;
        if (_BitScanReverse(&mask, number))
            return mask;
        return sizeof(std::uint32_t) * 8;
#elif defined(__GNUC__)
        if (number == 0)
            return sizeof(std::uint32_t) * 8;
        return sizeof(std::uint32_t) * 8 - 1 - __builtin_clz(number);
#else
        if (number == 0)
            return sizeof(std::uint32_t) * 8;
        number |= number >> 16;
        number |= number >> 8;
        number |= number >> 4;
        number |= number >> 2;
        number |= number >> 1;
        return bitsSet(number) - 1;
#endif
    }
    
    /*
    Find the position of the least significant 1-bit
    
    number: a 32-bit unsigned integer
    
    returns the 0-index position of the set LSB, or 32 for 0
    */
    inline size_t lsbSet(std::uint32_t number)
    {
#if defined(_MSC_VER)
        long mask;
        if (_BitScanForward(&mask, number))
            return mask;
        return sizeof(std::uint32_t) * 8;
#elif defined(__GNUC__)
        if (number == 0)
            return sizeof(std::uint32_t) * 8;
        return __builtin_ctz(number);
#else
        if (number == 0)
            return sizeof(std::uint32_t) * 8;
        number |= number << 16;
        number |= number << 8;
        number |= number << 4;
        number |= number << 2;
        number |= number << 1;
        return sizeof(std::uint32_t) * 8 - bitsSet(number);
#endif
    }
    
    /*
    Reverse the order of bits in an 8-bit integer
    
    number: 8-bit unsigned integer to reverse
    
    returns the bitwise reversal of number
    */
    inline std::uint8_t reverse8(std::uint8_t number)
    {
        number = ((number & 0xF0) >> 4) | ((number & 0x0F) << 4);
        number = ((number & 0xCC) >> 2) | ((number & 0x33) << 2);
        number = ((number & 0xAA) >> 1) | ((number & 0x55) << 1);
        return number;
    }
    
// #define UTF8_MAX_LEN 6
    constexpr int UTF8_MAX_LEN = 6;
    
    /*
    Convert an integer to the UTF-8 representation
    
    returns the number of bytes needed to represent v
    */
    size_t utf8(std::uint32_t v, std::uint8_t *dst);
    
    /*
    Convert UTF-8 to a single integer
    
    returns the number of bytes used, 0 for failure
    */
    size_t utf8(std::uint8_t *src, std::uint32_t &v);
    
    /*
    Given a first UTF-8 byte, how many more are there for this codepoint?
    */
    inline size_t utf8BytesLeft(std::uint8_t firstByte)
    {
        size_t firstZero = msbSet((std::uint8_t)~firstByte);
        if (firstZero == 7) {
            return 0;
        }
        return 6 - firstZero;
    }
    
}

namespace Huffman {
    
    /*
    A huffman code/tree for integer symbols
    */
    class HuffmanCode {
        private:
            std::vector<std::map<int, int>> decode;
            std::map<int, std::pair<int, size_t>> encode;
            void initFromList(std::vector<std::vector<int>>& symbolsList);
        public:
            
            /*
            Construct the code from a list of every symbol of each symbol length
            
            symbolsList: Each integer in symbolsList[x] represents a symbol with code length x+1
            */
            HuffmanCode(std::vector<std::vector<int>>& symbolsList);
            
            /*
            Construct the code from a frequency table, optionally limiting code length
            
            frequencies: A map of symbol to relative frequency
            limit: The maximum code length, or 0 for no limit
            */
            HuffmanCode(std::map<int, int>& frequencies, size_t limit = 0);
            
            /*
            Get the code and code lengths for a given symbol
            
            symbol: Numeric symbol to write
            code out: Code of symbol
            length out: Number of bits in code to write
            returns true if a code was found to match symbol
            */
            bool write(int symbol, int& code, size_t& length) const;
            
            /*
            Write a symbol to a BitBufferOut
            
            symbol: Symbol to write
            buffer: Output buffer to write to
            returns true if a code was found
            */
            bool write(int symbol, BitBuffer::BitBufferOut& buffer) const;
            
            /*
            Find the symbol that matches a code and length
            
            code: Code to match
            length: Bits in code
            output out: Found matching symbol
            returns true if a match was found
            */
            bool read(int code, size_t length, int& output) const;
            
            /*
            Read the next symbol from a bit input
            
            buffer: Source of bits
            output out: Matched symbol if any
            returns true if a symbol was found
            */
            bool read(BitBuffer::BitBufferIn& buffer, int& output) const;
            
            /*
            returns a vector of the number of symbols of each code length
            */
            std::vector<size_t> lengthCounts() const;
            
            /*
            returns the symbols in order of each length
            */
            std::vector<std::vector<int>> orderedSymbols() const;
    };
    
    /*
    Exception thrown when a Huffman code could not be generated
    */
    class HuffmanException : public std::exception {
        private:
            std::string message;
        public:
            HuffmanException(std::string message) : message{message} {}
            virtual const char* what();
    };
    
}

namespace Digest {
    
    std::uint8_t crc8_base(const std::uint8_t *data, size_t n, std::uint8_t start = 0);
    
    /*
    Calculate and accumulate the CRC8 of some data
    
    data: Pointer to data
    n: Number of T elements to checksum (number of T, not bytes)
    start: Starting CRC value, defaults to 0
    returns the 8-bit CRC8 with polynomial 7
    */
    template <class T>
    inline std::uint8_t crc8(const T *data, size_t n, std::uint8_t start = 0)
    {
        return crc8_base(reinterpret_cast<const std::uint8_t*>(data), n * sizeof(T), start);
    }
    
    std::uint16_t crc16_base(const std::uint8_t *data, size_t n, std::uint16_t start = 0);
    
    /*
    Calculate and accumulate the CRC16 of some data
    
    data: Pointer to data
    n: Number of T elements to checksum (number of T, not bytes)
    start: Starting CRC value, defaults to 0
    returns the 16-bit CRC16 with polynomial 0x8005
    */
    template <class T>
    std::uint16_t crc16(const T *data, size_t n, std::uint16_t start = 0)
    {
        return crc16_base(reinterpret_cast<const std::uint8_t*>(data), n * sizeof(T), start);
    }
    
    template <class T>
    inline std::uint8_t crc8(const std::vector<T>& vec, std::uint8_t start = 0)
    {
        return crc8(vec.data(), vec.size(), start);
    }
    
    template <class T>
    inline std::uint16_t crc16(const std::vector<T>& vec, std::uint16_t start = 0)
    {
        return crc16(vec.data(), vec.size(), start);
    }
    
    constexpr size_t MD5_BUFFER_SIZE = 16;
    constexpr std::uint32_t MD5_A = 0x67452301;
    constexpr std::uint32_t MD5_B = 0xefcdab89;
    constexpr std::uint32_t MD5_C = 0x98badcfe;
    constexpr std::uint32_t MD5_D = 0x10325476;
    
    /*
    An object to accumulate data to produce an MD5 digest
    */
    class MD5Context {
        private:
            size_t bytesProcessed;
            size_t bufferIndex;
            std::uint32_t a, b, c, d;
            std::uint32_t buffer[MD5_BUFFER_SIZE];
            void processBuffer();
        public:
            MD5Context() :
                bytesProcessed{0},
                bufferIndex{0},
                buffer{0},
                a {MD5_A},
                b {MD5_B},
                c {MD5_C},
                d {MD5_D} {}
            
            /*
            Take in arbitrary data and process it
            */
            template <class T>
            inline void consume(const T *data, size_t n)
            {
                consume(reinterpret_cast<const std::uint8_t*>(data), n * sizeof(T));
            }
            
            void consume(const std::uint8_t *data, size_t n);
            
            /*
            Consume a single byte
            */
            MD5Context& operator<<(std::uint8_t byte);
            
            /*
            Consume a vector of arbitrary type
            */
            template <class T>
            inline MD5Context& operator<<(const std::vector<T>& vec)
            {
                consume(vec.data(), vec.size());
                return *this;
            }
            
            /*
            Finalize remaining data and metadata and return the MD5 digest
            */
            std::vector<std::uint8_t> finalize();
    };
    
}

#endif