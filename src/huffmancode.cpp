/*
huffmancode.cpp
*/

#include <vector>
#include <utility>
#include <algorithm>
#include <iostream>
#include <queue>
#include "bitutil.hpp"

struct HuffmanNode {
    int symbol;
    int frequency;
    int length;
    std::pair<HuffmanNode*, HuffmanNode*> children;
    HuffmanNode(int symbol, int frequency) :
        symbol{symbol},
        frequency{frequency},
        length{0},
        children{std::pair<HuffmanNode*, HuffmanNode*>(nullptr, nullptr)} {}
    HuffmanNode(HuffmanNode *left, HuffmanNode *right) :
        symbol{-1},
        frequency{left->frequency + right->frequency},
        length{0},
        children{std::pair<HuffmanNode*, HuffmanNode*>(left, right)} {}
    void clean()
    {
        if (children.first != nullptr) {
            children.first->clean();
            delete children.first;
        }
        if (children.second != nullptr) {
            children.second->clean();
            delete children.second;
        }
    }
    bool operator<(HuffmanNode other)
    {
        if (frequency != other.frequency) {
            return frequency > other.frequency;
        }
        return symbol > other.symbol;
    }
};

Huffman::HuffmanCode::HuffmanCode(std::vector<std::vector<int>>& symbolList)
{
    initFromList(symbolList);
}

Huffman::HuffmanCode::HuffmanCode(std::map<int, int>& frequencies, size_t limit)
{
    std::vector<HuffmanNode> heap;
    for (auto it = frequencies.begin(); it != frequencies.end(); it++) {
        heap.push_back(HuffmanNode(it->first, it->second));
    }
    std::make_heap(heap.begin(), heap.end());
    while (heap.size() > 1) {
        std::pop_heap(heap.begin(), heap.end());
        HuffmanNode *right = new HuffmanNode(heap.back());
        heap.pop_back();
        std::pop_heap(heap.begin(), heap.end());
        HuffmanNode *left = new HuffmanNode(heap.back());
        heap.pop_back();
        heap.push_back(HuffmanNode(left, right));
        std::push_heap(heap.begin(), heap.end());
    }
    HuffmanNode root = heap[0];
    std::queue<HuffmanNode> queue;
    queue.push(root);
    std::vector<std::pair<int, int>> sortedSyms;
    std::vector<int> population;
    while(!queue.empty()) {
        HuffmanNode node = queue.front();
        queue.pop();
        if (node.children.first == nullptr) { // Leaf
            sortedSyms.push_back(std::pair<int, int>(node.length, node.symbol));
            while (population.size() < node.length) {
                population.push_back(0);
            }
            population[node.length - 1]++;
        }
        else { // Internal
            node.children.first->length = node.length + 1;
            node.children.second->length = node.length + 1;
            queue.push(*node.children.first);
            queue.push(*node.children.second);
        }
    }
    root.clean();
    std::sort(sortedSyms.begin(), sortedSyms.end());
    if (limit > 0) {
        while (population.size() > limit) {
            if (population.back() == 0) {
                population.pop_back();
                continue;
            }
            if (population.size() < 3) {
                throw Huffman::HuffmanException("Limit too small");
            }
            size_t key = population.size() - 3;
            while (population[key] == 0) {
                if (key == 0) {
                    throw Huffman::HuffmanException("Limit too small");
                }
                key--;
            }
            population[population.size() - 1] -= 2;
            population[population.size() - 2]++;
            population[key + 1] += 2;
            population[key]--;
        }
    }
    std::vector<std::vector<int>> symbolList;
    size_t length = 1;
    for (auto it = sortedSyms.begin(); it != sortedSyms.end(); it++) {
        while (population[length - 1] == 0) {
            length++;
        }
        while (symbolList.size() < length) {
            symbolList.push_back(std::vector<int>());
        }
        symbolList[length - 1].push_back(it->second);
        population[length - 1]--;
    }
    initFromList(symbolList);
}

void Huffman::HuffmanCode::initFromList(std::vector<std::vector<int>>& symbolList)
{
    int code = 0;
    for (size_t i = 0; i < symbolList.size(); i++) {
        std::vector<int>& symbols = symbolList[i];
        size_t length = i + 1;
        if (symbols.size() > 0) {
            while (decode.size() < length) {
                decode.push_back(std::map<int, int>());
            }
            for (size_t j = 0; j < symbols.size(); j++) {
                int symbol = symbols[j];
                encode[symbol] = std::pair<int, size_t>(code, length);
                decode[i][code++] = symbol;
            }
        }
        code <<= 1;
    }
    // for (size_t i = 0; i < decode.size(); i++) {
        // std::map<int, int>& symbols = decode[i];
        // for (auto it = symbols.begin(); it != symbols.end(); it++) {
            // std::cout << it->second << ":(" << it->first << ',' << i + 1 << ")\n";
        // }
    // }
}

bool Huffman::HuffmanCode::write(int symbol, int& code, size_t& length)
{
    auto it = encode.find(symbol);
    if (it == encode.end()) {
        return false;
    }
    code = it->second.first;
    length = it->second.second;
    return true;
}

bool Huffman::HuffmanCode::write(int symbol, BitBuffer::BitBufferOut& buffer)
{
    int code;
    size_t length;
    if (!write(symbol, code, length)) {
        // std::cout << "Symbol " << symbol << " not found\n";
        return false;
    }
    // std::cout << "Writing " << code << " in " << length << std::endl;
    buffer.write(code, length);
    return true;
}

bool Huffman::HuffmanCode::read(int code, size_t length, int& symbol)
{
    if (length > decode.size() || length == 0) {
        return false;
    }
    auto codes = decode[length - 1];
    auto it = codes.find(code);
    if (it == codes.end()) {
        return false;
    }
    symbol = it->second;
    return true;
}

bool Huffman::HuffmanCode::read(BitBuffer::BitBufferIn& buffer, int& output)
{
    int code = 0;
    for (size_t length = 1; length <= decode.size(); length++) {
        code = (code << 1) | buffer.read(1);
        if (read(code, length, output)) {
            return true;
        }
        // std::cout << "Read " << code << " of length " << length << " failed\n";
    }
    return false;
}

const char* Huffman::HuffmanException::what()
{
    return ("Huffman Exception: " + message).c_str();
}
