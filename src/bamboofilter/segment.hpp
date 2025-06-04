#pragma once
#include <array>
#include <cstdint>
#include <random>
#include <vector>

/* compile-time parameters (unchanged) */
constexpr std::size_t ENTRIES_PER_BUCKET = 4;
constexpr std::size_t BUCKETS_PER_SEG    = 10;
constexpr int         BITS_PER_TAG       = 12;
constexpr uint16_t    FP_MASK            = (1u << BITS_PER_TAG) - 1u;

/* ------------------------------------------------------------------ */
/** A bucket holds at most four 12-bit fingerprints. */
struct Bucket {
    std::array<uint16_t, ENTRIES_PER_BUCKET> fp {{0}};   // 8 bytes total

    /** Insert fingerprint; return true on success. */
    bool insert(uint16_t f) {
        f &= FP_MASK;
        for (auto& s : fp) if (s == 0) { s = f; return true; }
        return false;
    }
    /** True if the fingerprint is present. */
    bool lookup(uint16_t f) const {
        f &= FP_MASK;
        for (uint16_t s : fp) if (s == f) return true;
        return false;
    }
    /** Remove fingerprint; return true if found. */
    bool remove(uint16_t f) {
        f &= FP_MASK;
        for (auto& s : fp) if (s == f) { s = 0; return true; }
        return false;
    }
    /** Randomly evict one fingerprint and return it (0 if bucket empty). */
    uint16_t evict_random() {
        static std::mt19937 gen{std::random_device{}()};
        int filled[ENTRIES_PER_BUCKET], n = 0;
        for (int i = 0; i < int(ENTRIES_PER_BUCKET); ++i)
            if (fp[i]) filled[n++] = i;
        if (n == 0) return 0;
        uint16_t v = fp[filled[std::uniform_int_distribution<int>(0,n-1)(gen)]];
        remove(v); return v;
    }
};

/* ------------------------------------------------------------------ */
/** A segment contains a fixed array of buckets. */
class Segment {
public:
    explicit Segment(std::size_t n = BUCKETS_PER_SEG) : buckets(n) {}
    Bucket&       get_bucket(std::size_t i)       { return buckets[i]; }
    const Bucket& get_bucket(std::size_t i) const { return buckets[i]; }
private:
    std::vector<Bucket> buckets;
};
