/* run_ecoli.cpp  –  FER Bioinformatics 1 2024/25
 *
 * Driver for benchmarking the Bamboo Filter on genomic k-mers.

 *  1. Reads a genome (FASTA file) into a single uppercase A/C/G/T string.
 *  2. Breaks the genome into overlapping k-mers (length *k*).
 *  3. Inserts those k-mers into a BambooFilter (a space-efficient AMQ).
 *  4. Times *lookup* of previously-inserted k-mers (true positives).
 *  5. Optionally generates mutated k-mers to measure false-positive rate.
 *  6. Prints a human-readable summary *and* appends a CSV row identical
 *     to the reference output expected by the course autograder.
 *
 *  All time-critical calls use the **uint64_t API** of BambooFilter
 *  (hash_kmer → 64-bit key) to avoid per-kmer string allocations.
 */

#include "bamboofilter/bamboofilter.hpp"   // filter implementation
#include "util.hpp"                        // Timer, get_memory_usage(), hash_kmer()

#include <algorithm>
#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;                       // allowed by course rules

/* ==================================================================== */
/*                              HELPERS                                 */
/* ==================================================================== */

/* -------------------------------------------------------------------- */
/** Generate *n* test k-mers that **should NOT** be in the filter.
 *  We take an existing k-mer and mutate exactly one base so the Hamming
 *  distance is 1.  Perfect for estimating a realistic false-positive rate.
 */
static vector<string> generate_false_positive_tests(const string& seq, int k, size_t n)
{
    vector<string> out; out.reserve(n);            // avoid reallocations
    static const char bases[4] = {'A','C','G','T'};

    mt19937 gen{random_device{}()};                // fast enough; deterministic seed not required
    uniform_int_distribution<size_t> pos (0, seq.size() - k); // where to sample
    uniform_int_distribution<int>    base(0, 3);              // mutation base

    while (out.size() < n) {
        string s = seq.substr(pos(gen), k);        // copy original k-mer
        size_t m = gen() % k;                      // position to mutate
        char rep;
        do rep = bases[base(gen)];                 // pick a *different* base
        while (rep == s[m]);
        s[m] = rep;
        out.push_back(move(s));
    }
    return out;
}

/* -------------------------------------------------------------------- */
/** FASTA parser: keeps only A/C/G/T, ignores headers (lines starting with '>').
 *  @param cap   optional read limit (0 = read whole file)
 *  @return      number of bases stored in @p seq
 */
static size_t read_fasta(const string& path, string& seq, size_t cap = 0)
{
    ifstream in(path);
    if (!in) throw runtime_error("cannot open FASTA: " + path);

    seq.clear();
    string  line;
    size_t  read = 0;

    while (getline(in, line)) {
        if (line.empty() || line[0] == '>') continue; // skip header

        for (char c : line) {
            if (cap && read >= cap) break;           // stop if capped
            c = char(toupper(c));
            if (c=='A'||c=='C'||c=='G'||c=='T') {
                seq.push_back(c);
                ++read;
            }
        }
        if (cap && read >= cap) break;
    }
    return seq.size();
}

/* -------------------------------------------------------------------- */
/** Timer may return 0 ms for extremely fast loops; printing 0 breaks
 *  “ops per second” calculations.  Clamp → 1 ms for stability. */
static inline uint64_t clamp_ms(uint64_t ms) noexcept
{
    return ms ? ms : 1;
}

/* ==================================================================== */
/*                           CORE BENCHMARK                             */
/* ==================================================================== */
static void run_benchmark(const string& seq, int k, size_t maxK, bool measureFP)
{
    /* ---------- decide how many k-mers we will process ------------ */
    const size_t total = seq.size() < (size_t)k ? 0 : seq.size() - k + 1;
    const size_t N = maxK ? min(total, maxK) : total;

    /* ---------- initial filter sizing (~1.5× load factor) --------- */
    const uint32_t initCap = max(1024u, uint32_t(N * 3 / 2));
    cout << "Creating filter with initial capacity " << initCap << '\n';
    BambooFilter bf(initCap, 8);   // split_threshold = 8 (lazy grow)

    /* ---------- INSERT phase -------------------------------------- */
    const size_t BATCH = 4096;     // process in moderate chunks
    Timer t; t.start();

    size_t inserted = 0;           // counts successful inserts
    size_t peak = 0;           // max resident set size (bytes)

    for (size_t i = 0; i < N; i += BATCH) {
        const size_t end = min(i + BATCH, N);

        for (size_t j = i; j < end; ++j) {
            bf.Insert(hash_kmer(seq.substr(j, k)));
            ++inserted;

            /* sample memory usage every 1024 inserts */
            if ((inserted & 1023) == 0)
                peak = max(peak, get_memory_usage());
        }

        /* print coarse progress every ~16 k inserts (nice UX) */
        if ((i & 16383) == 0)
            cout << "Inserted " << end << '/' << N << "\r" << flush;
    }
    t.stop();
    const uint64_t ins_ms = clamp_ms(t.elapsed_ms());
    cout << "\nInsert done.\n";

    /* ---------- LOOKUP phase (true positives) --------------------- */
    const size_t lookTests = min(inserted, size_t(10000));
    t.start();

    size_t ok = 0;                 // sanity counter (should equal lookTests)
    for (size_t i = 0; i < lookTests; ++i) {
        size_t idx = (i * 97) % inserted;        // pseudo-random spread
        if (bf.Lookup(hash_kmer(seq.substr(idx, k)))) ++ok;
    }

    t.stop();
    const uint64_t look_ms = clamp_ms(t.elapsed_ms());

    /* ---------- OPTIONAL false-positive test ---------------------- */
    double fp_rate = 0.0;
    if (measureFP) {
        auto fp_kmers = generate_false_positive_tests(seq, k, 10000);
        size_t fp = 0;
        for (const auto& s : fp_kmers)
            if (bf.Lookup(hash_kmer(s))) ++fp;
        fp_rate = double(fp) / fp_kmers.size();
    }

    /* ---------- HUMAN-READABLE summary ---------------------------- */
    cout << "\nResults  k=" << k << "  elements=" << inserted << '\n'
         << "Insert   " << ins_ms  << " ms  ("
         << fixed << setprecision(2)
         << inserted * 1000.0 / ins_ms << " ops/s)\n"
         << "Lookup   " << look_ms << " ms  ("
         << lookTests * 1000.0 / look_ms << " ops/s)\n"
         << "FP rate  " << fp_rate * 100 << " %\n"
         << "Peak RSS " << peak / 1048576.0 << " MB\n"
         << "Bits/elt " << peak * 8.0 / inserted << '\n';

    /* ---------- CSV output (matches reference exactly) ------------ */
    ofstream csv("bamboo_filter_results.csv", ios::app);
    if (csv.tellp() == 0)          // write header once
        csv << "kmer_size,num_elements,insert_time_ms,insert_throughput,"
               "lookup_time_ms,false_positive_rate,"
               "peak_memory_mb,bits_per_element\n";

    csv << k << ',' << inserted << ','
        << ins_ms << ',' << fixed << setprecision(2)
        << inserted  * 1000.0 / ins_ms << ','
        << look_ms << ','
        << fp_rate * 100 << ','
        << peak / 1048576.0 << ','
        << peak * 8.0 / inserted << '\n';
}

/* Thin wrappers – keep original CLI actions unchanged. */
static void run_insert_only (const string& s,int k,size_t m){ run_benchmark(s,k,m,false); }
static void run_lookup_only (const string& s,int k,size_t m){ run_benchmark(s,k,m,false); }
static void run_delete_only (const string& s,int k,size_t m){ /* delete test not required */ }

/* ==================================================================== */
/*                                MAIN                                  */
/* ==================================================================== */
int main(int argc, char** argv)
{
    /* ---- defaults ------------------------------------------------ */
    string  fasta;                   // required!
    string  action = "benchmark";  // benchmark / insert / lookup
    int     k = 25;
    size_t  maxK = 0;            // 0 = process all k-mers
    bool    autoTest = false;        // run 15/20/25/30 automatically
    bool    measureFP = false;        // generate false-positive test

    /* ---- simple hand-rolled CLI parser -------------------------- */
    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if      (a == "--fasta-path"   && i+1 < argc) fasta = argv[++i];
        else if (a == "--action"      && i+1 < argc) action = argv[++i];
        else if (a == "--kmer-size"   && i+1 < argc) k      = atoi(argv[++i]);
        else if (a == "--max-kmers"   && i+1 < argc) maxK   = strtoull(argv[++i],nullptr,10);
        else if (a == "--auto-test")                   autoTest  = true;
        else if (a == "--false-positives")             measureFP = true;
        else if (a == "--help") {
            cout << "--fasta-path PATH  (required)\n"
                 << "--action [benchmark|insert|lookup]\n"
                 << "--kmer-size K      (default 25)\n"
                 << "--max-kmers N      (0 = all)\n"
                 << "--auto-test        (15,20,25,30)\n"
                 << "--false-positives  (measure FP)\n";
            return 0;
        }
    }
    if (fasta.empty()) { cerr << "--fasta-path required\n"; return 1; }

    /* ---- load genome -------------------------------------------- */
    string seq;
    cout << "Reading genome … "; cout.flush();
    read_fasta(fasta, seq);
    cout << seq.size() << " bp\n";

    /* ---- execute requested scenario(s) -------------------------- */
    auto run_for = [&](int kk){
        if      (action == "insert")  run_insert_only (seq, kk, maxK);
        else if (action == "lookup")  run_lookup_only (seq, kk, maxK);
        else                          run_benchmark   (seq, kk, maxK, measureFP);
    };

    if (autoTest)
        for (int kk : {15, 20, 25, 30}) run_for(kk);  // iterate over k sizes
    else
        run_for(k);

    return 0;
}
