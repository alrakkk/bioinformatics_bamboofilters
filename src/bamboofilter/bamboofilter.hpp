#pragma once
/* 
 *  A *very* small-footprint approximate-membership filter inspired by
 *  Cuckoo-Filter design ideas (two possible locations, bounded kick-out)
 *
 *  •  Each **Segment** contains `BUCKETS_PER_SEG` *buckets*.
 *  •  Every **Bucket** holds exactly four 12-bit fingerprints (tags).
 *  •  A 64-bit *hash* is split into
 *        – a 12-bit fingerprint  (high bits) and
 *        – an index that chooses one of two segments  (low bits ⊕ fp mix).
 *  •  If both candidate buckets are full we “kick out” a random victim
 *     up to `MAX_KICKS` times before giving up.
 *
 *  Only **fingerprints** are stored – the full 64-bit keys live outside
 *  the structure, which keeps memory tight.  All public functions accept
 *  either a C-string / `std::string` (hashed internally) or a ready
 *  -made 64-bit hash for maximum speed in the hot paths.
 *   FNV-1a hash inspo from: https://gist.github.com/ruby0x1/81308642d0325fd386237cfa3b44785c
 */

#include <algorithm>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "segment.hpp"   //  Bucket & Segment definitions
#include "bitsutil.h"    //  upper_power2(), FP_MASK, constants


class BambooFilter {
public:
    /** Create an empty filter that can hold ≈ @p capacity elements before
     *  the first split.  @p split_cond is kept for API parity (unused). */
    explicit BambooFilter(uint32_t capacity, uint32_t split_cond = 2)
        : buckets_per_seg(BUCKETS_PER_SEG),
          num_items(0),
          split_threshold(split_cond),   // not used, but kept for spec
          gen(rd())
    {
        /* Decide how many *segments* we need, then round up to power-of-2
         * so that “segment ID = hash & (seg_mask)” is a cheap bit-and. */
        const size_t per_seg   = buckets_per_seg * ENTRIES_PER_BUCKET;
        num_segments           = upper_power2(
            std::max<size_t>(1, (capacity + per_seg - 1) / per_seg));
        seg_mask               = num_segments - 1;      // == num_segments-1

        /* Allocate the array of (empty) segments up-front. */
        segments.reserve(num_segments);
        for (size_t i = 0; i < num_segments; ++i)
            segments.emplace_back(std::make_unique<Segment>(buckets_per_seg));
    }

/* ─────────────────────── Public interface ────────────────────────── */
    /* --- Friendly overloads that accept raw strings ---------------- */
    bool Insert (const char*        s) { return Insert(hash_str(s)); }
    bool Insert (const std::string& s) { return Insert(s.c_str());    }

    bool Lookup (const char*        s) const { return Lookup(hash_str(s)); }
    bool Lookup (const std::string& s) const { return Lookup(s.c_str());   }

    bool Delete (const char*        s) { return Delete(hash_str(s)); }
    bool Delete (const std::string& s) { return Delete(s.c_str());   }

    /* 64-bit fast-path (used by run_ecoli.cpp hot loops) */
    /** Insert a pre-hashed key (64-bit).  Returns false only if the
     *  table is completely full and cuckoo evacuation failed.          */
    bool Insert(uint64_t h) {
        const uint16_t fp = fingerprint(h);           // 12-bit tag
        const uint32_t b  = h % buckets_per_seg;      // bucket index
        const uint32_t s1 = h & seg_mask;             // primary segment

        if (segments[s1]->get_bucket(b).insert(fp)) { // easy path
            ++num_items; grow_if_needed(); return true;
        }
        const uint32_t s2 = alt_segment(s1, fp);      // second choice
        if (segments[s2]->get_bucket(b).insert(fp)) {
            ++num_items; grow_if_needed(); return true;
        }
        /* both buckets full → try bounded cuckoo kick-out */
        return cuckoo(s1, b, fp);
    }

    /** True if the hash h is *probably* in the filter. 0 % FN, small FP. */
    bool Lookup(uint64_t h) const {
        const uint16_t fp = fingerprint(h);
        const uint32_t b  = h % buckets_per_seg;
        const uint32_t s1 = h & seg_mask;
        if (segments[s1]->get_bucket(b).lookup(fp)) return true;
        return segments[alt_segment(s1, fp)]->get_bucket(b).lookup(fp);
    }

    /** Remove element.  Returns *true* only if a fingerprint was found.  */
    bool Delete(uint64_t h) {
        const uint16_t fp = fingerprint(h);
        const uint32_t b  = h % buckets_per_seg;
        const uint32_t s1 = h & seg_mask;
        if (segments[s1]->get_bucket(b).remove(fp)) { --num_items; return true; }
        if (segments[alt_segment(s1, fp)]->get_bucket(b).remove(fp)) {
            --num_items; return true;
        }
        return false;
    }

    /** Current logical element count (approx.). */
    uint32_t size() const { return num_items; }

    /* Public diagnostic fields filled by grow_if_needed() – optional. */
    size_t last_expand_before_memory = 0;  ///< bytes before last grow()
    size_t last_expand_after_memory  = 0;  ///< bytes after  last grow()

/* ────────────────────── Implementation details ───────────────────── */
private:
    /* -------- fixed layout parameters (runtime copies) ------------- */
    uint32_t buckets_per_seg;   ///< number of buckets per segment (10)
    uint32_t num_segments;      ///< power-of-2 segment count
    uint32_t num_items;         ///< fingerprints currently stored
    uint32_t split_threshold;   ///< unused in this pared-down variant
    uint32_t seg_mask;          ///< num_segments-1 → bitmask for fast %
    std::vector<std::unique_ptr<Segment>> segments; ///< the table

    /* -------- RNG used only for kick-out victim selection ---------- */
    std::random_device rd;
    std::mt19937       gen;

/* helper: hashing & fingerprinting  */

    /** Tiny, fast 64-bit FNV-1a hash. inspo from*/
    static uint64_t hash_str(const char* s)
    {
        constexpr uint64_t off = 0xcbf29ce484222325ULL;
        constexpr uint64_t prm = 0x100000001b3ULL;
        uint64_t h = off;
        while (*s) { h ^= static_cast<uint8_t>(*s++); h *= prm; }
        return h;
    }

    /** Extract 12-bit fingerprint from a 64-bit hash.
     *  0 is reserved, so map it to 1. */
    static uint16_t fingerprint(uint64_t h)
    {
        uint16_t fp = static_cast<uint16_t>((h >> 32) & FP_MASK);
        return fp ? fp : 1;
    }

    /** Derive alternate segment ID from primary @p s and @p fp. 
     *  Multiplication by an odd 32-bit constant → good mixing. */
    uint32_t alt_segment(uint32_t s, uint16_t fp) const
    {
        return (s ^ (fp * 0x5bd1e995u)) & seg_mask;
    }

/* ----- dynamic growth (split into twice as many segments) ---------- */

    /** Grow table when global load > 90 %.   Simpler than full Bamboo
     *  algorithm but good enough for the assignment. */
    void grow_if_needed()
    {
        const double load = double(num_items) /
                            (num_segments *
                             buckets_per_seg *
                             ENTRIES_PER_BUCKET);   // denominator = slots
        if (load < 0.90) return;                   // still plenty of room

        last_expand_before_memory = mem_bytes();

        const size_t old = num_segments;
        num_segments <<= 1;                        // ×2 segments
        seg_mask = num_segments - 1;
        segments.reserve(num_segments);
        for (size_t i = 0; i < old; ++i)
            segments.emplace_back(
                std::make_unique<Segment>(buckets_per_seg));

        last_expand_after_memory  = mem_bytes();
    }

    /** Crude self-report (bytes) used only for driver diagnostics. */
    size_t mem_bytes() const
    {
        return sizeof(*this) +
               segments.size() *
               (sizeof(Segment) + buckets_per_seg * sizeof(Bucket));
    }

/* ----- Cuckoo kick-out (bounded) ----------------------------------- */

    /** Try to place *fp* starting at (seg,buc).  Randomly evicts one slot
     *  up to `MAX_KICKS` times.  Returns *true* on success.            */
    bool cuckoo(uint32_t seg, uint32_t buc, uint16_t fp)
    {
        constexpr int MAX_KICKS = 8;               // small to bound latency
        for (int i = 0; i < MAX_KICKS; ++i) {
            Bucket& b = segments[seg]->get_bucket(buc);

            /* 1. Kick out a random victim fingerprint from this bucket. */
            uint16_t victim = b.evict_random();    // returns 0 if empty (rare)
            if (!victim) return false;             // should not happen

            b.insert(fp);                          // place our fp here

            /* 2. Re-insert evicted fingerprint into its *alternate*
             *    segment; continue the chain if that bucket is full.  */
            fp  = victim;
            seg = alt_segment(seg, fp);
            if (segments[seg]->get_bucket(buc).insert(fp)) {
                ++num_items;                       // finally placed
                grow_if_needed();
                return true;
            }
        }
        return false;                              // table considered full
    }
};
