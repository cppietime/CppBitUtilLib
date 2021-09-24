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
            Flushes anything left in the buffer
            
            fill: If true, empty space is filled with 1-bits instead of 0-bits
            
            returns the number of bytes actually written to the underlying stream
            */
            size_t flush(bool fill = false);
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
            bool write(int symbol, int& code, size_t& length);
            
            /*
            Write a symbol to a BitBufferOut
            
            symbol: Symbol to write
            buffer: Output buffer to write to
            returns true if a code was found
            */
            bool write(int symbol, BitBuffer::BitBufferOut& buffer);
            
            /*
            Find the symbol that matches a code and length
            
            code: Code to match
            length: Bits in code
            output out: Found matching symbol
            returns true if a match was found
            */
            bool read(int code, size_t length, int& output);
            
            /*
            Read the next symbol from a bit input
            
            buffer: Source of bits
            output out: Matched symbol if any
            returns true if a symbol was found
            */
            bool read(BitBuffer::BitBufferIn& buffer, int& output);
            
            /*
            returns a vector of the number of symbols of each code length
            */
            std::vector<size_t> lengthCounts();
            
            /*
            returns the symbols in order of each length
            */
            std::vector<std::vector<int>> orderedSymbols();
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

#endif