#pragma once
#include <iostream>
#include <vector>
#include <array>
#include "MurmurHash3.h"

inline std::array<uint64_t, 2> hash(void *data,
                             std::size_t len) {
  std::array<uint64_t, 2> hashValue;
  MurmurHash3_x64_128(data, len, 0, hashValue.data());

  return hashValue;
}

inline uint64_t nthHash(uint8_t n,
                        uint64_t hashA,
                        uint64_t hashB,
                        uint64_t filterSize) {
    return (hashA + n * hashB) % filterSize;
}

class BloomFilter {
public:
    BloomFilter(uint64_t size, uint8_t numHashes)
        : m_bits(size),
            m_numHashes(numHashes) {}

    void add(void *data, std::size_t len) {
        auto hashValues = hash(data, len);

        for (int n = 0; n < m_numHashes; n++) {
            m_bits[nthHash(n, hashValues[0], hashValues[1], m_bits.size())] = true;
        }
    }
    bool possiblyContains(void *data, std::size_t len) const {
        auto hashValues = hash(data, len);

        for (int n = 0; n < m_numHashes; n++) {
            if (!m_bits[nthHash(n, hashValues[0], hashValues[1], m_bits.size())]) {
                return false;
            }
        }

        return true;
    }

    void clear(){
        for(size_t i = 0; i < m_bits.size(); i++){
            m_bits[i] = false;
        }
    }

private:
    std::vector<bool> m_bits;
    uint8_t m_numHashes;
  
};