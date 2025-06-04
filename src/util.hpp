// util.hpp
#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <cstring>
#include <sys/resource.h>


// Timer utilities for benchmarking
class Timer {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    bool running;

public:
    Timer() : running(false) {}
    
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
        running = true;
    }
    
    void stop() {
        end_time = std::chrono::high_resolution_clock::now();
        running = false;
    }
    
    // Get elapsed time in milliseconds
    int64_t elapsed_ms() const {
        if (running) {
            auto current = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                current - start_time).count();
        }
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();
    }
    
    // Get elapsed time in nanoseconds
    int64_t elapsed_ns() const {
        if (running) {
            auto current = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::nanoseconds>(
                current - start_time).count();
        }
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time).count();
    }
};

// Memory usage utilities - platform specific implementation
inline size_t get_memory_usage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    return pmc.WorkingSetSize;
#else
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
    #ifdef __APPLE__
        return rusage.ru_maxrss;  // macOS: bytes
    #else
        return rusage.ru_maxrss * 1024;  // Linux: kilobytes to bytes
    #endif
#endif
}

// Current time in nanoseconds for benchmarking
inline uint64_t now_nanos() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

// Current time in milliseconds for benchmarking
inline uint64_t now_millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

// Hash a k-mer string into a 64-bit integer
inline uint64_t hash_kmer(const std::string& kmer) {
    const uint64_t seed = 0x9747b28c;
    const uint64_t m = 0xc6a4a7935bd1e995ULL;
    const int r = 47;
    
    uint64_t h = seed ^ (kmer.length() * m);
    
    // Process 8 bytes at a time
    size_t i = 0;
    for (; i + 8 <= kmer.length(); i += 8) {
        uint64_t k;
        memcpy(&k, &kmer[i], 8);
        
        k *= m;
        k ^= k >> r;
        k *= m;
        
        h ^= k;
        h *= m;
    }
    
    // Process remaining bytes
    const unsigned char* tail = reinterpret_cast<const unsigned char*>(kmer.data() + i);
    switch (kmer.length() & 7) {
    case 7: h ^= uint64_t(tail[6]) << 48;
    case 6: h ^= uint64_t(tail[5]) << 40;
    case 5: h ^= uint64_t(tail[4]) << 32;
    case 4: h ^= uint64_t(tail[3]) << 24;
    case 3: h ^= uint64_t(tail[2]) << 16;
    case 2: h ^= uint64_t(tail[1]) << 8;
    case 1: h ^= uint64_t(tail[0]);
            h *= m;
    };
    
    h ^= h >> r;
    h *= m;
    h ^= h >> r;
    
    return h;
}