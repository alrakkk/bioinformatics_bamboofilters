//  E. coli k-mer benchmark using Bamboo Filter
//
//  • Reads a FASTA genome (upper-cases & filters to A/C/G/T).
//  • Builds positive k-mer sets from a leading sub-array of the genome.
//  • Inserts all positive k-mers into a Bamboo Filter, measuring throughput.
//  • Looks up positives (to sanity-check) and a random set of negatives
//    (1-mutation neighbours) to measure the false-positive rate.
//  • Streams CSV results to subarray_kmer_results.csv.

#include "bamboofilter/bamboofilter.hpp"   // <— upstream Bamboo Filter
#include "util.hpp"                        // get_memory_usage(), Timer helpers
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;

// -----------------------------------------------------------------------------
//  generate_false_positive_tests
//  ---------------------------------------------------------------------------
//  Build `n` k-mers that are **guaranteed** NOT to exist in the positive set.
//  Strategy: pick a random k-mer from the real genome, flip exactly one base
//  (keeping it A/C/G/T but different).  Because the genome is not repetitive
//  enough to contain *all* Hamming-1 neighbours, these are excellent
//  near-miss negatives for a Bloom-like filter test.
// -----------------------------------------------------------------------------
static vector<string> generate_false_positive_tests(const string& seq,
                                                    int k,
                                                    size_t n)
{
    vector<string> out;
    out.reserve(n);

    static const char bases[4] = {'A', 'C', 'G', 'T'};

    mt19937 gen{random_device{}()};                 // RNG for reproducibility
    uniform_int_distribution<size_t> pos(0, seq.size() - k); // start index
    uniform_int_distribution<int> base(0, 3);       // random base chooser

    while (out.size() < n) {
        string s = seq.substr(pos(gen), k);         // random *valid* k-mer
        size_t m = gen() % k;                       // position to mutate
        char rep;
        do rep = bases[base(gen)];                  // pick a *different* base
        while (rep == s[m]);
        s[m] = rep;                                 // single mutation
        out.push_back(move(s));
    }
    return out;
}

// -----------------------------------------------------------------------------
//  read_fasta
//  ---------------------------------------------------------------------------
//  Minimal FASTA loader: concatenates all sequence lines, uppercases them,
//  and strips any non-ACGT characters.  Returns the number of bases read.
// -----------------------------------------------------------------------------
static size_t read_fasta(const string& path, string& seq)
{
    ifstream in(path);
    if (!in) throw runtime_error("cannot open FASTA: " + path);

    seq.clear();
    string line;
    while (getline(in, line)) {
        if (line.empty() || line[0] == '>') continue;  // skip headers
        for (char c : line) {
            c = char(toupper(c));
            if (c == 'A' || c == 'C' || c == 'G' || c == 'T')
                seq.push_back(c);
        }
    }
    return seq.size();
}

// -----------------------------------------------------------------------------
//  Result – one row of output metrics
// -----------------------------------------------------------------------------
struct Result {
    size_t  sub_len;     // length of the genome slice used
    int     k;           // k-mer length
    size_t  inserted;    // # positive k-mers inserted
    uint64_t ins_ms;     // insertion time (ms)
    double  ins_tp;      // insertion throughput (k-mers / s)
    uint64_t look_ms;    // positive-lookup time (ms)
    size_t  true_pos;    // true positives counted during lookup
    size_t  true_neg;    // true negatives from negative queries
    double  fp_rate;     // false-positive rate on 10 000 negative queries
    double  peak_mb;     // peak RSS (MB) during insert
    double  bpe;         // bits per element ≈ memory efficiency
};

// -----------------------------------------------------------------------------
//  benchmark
//  ---------------------------------------------------------------------------
//  • Extract the first `sub_len` bp of the full genome.
//  • Generate every overlapping k-mer (positive set).
//  • Insert into Bamboo Filter in 4 096-element batches, recording peak RSS.
//  • Lookup a sample of positives and a fixed 10 000 negatives.
//  • Return a populated Result struct.
// -----------------------------------------------------------------------------
static Result benchmark(const string& seq, size_t sub_len, int k)
{
    // ---------- Generate positive k-mers ----------
    string s = seq.substr(0, min(sub_len, seq.size()));
    const size_t total = (s.size() < static_cast<size_t>(k)) ? 0 : s.size() - k + 1;

    // ---------- Build filter ----------
    const uint32_t initCap = max(1024u, uint32_t(total * 3 / 2));  // growth headroom
    BambooFilter bf(initCap, 8);  // 8 = bits per key target

    // ---------- Batched insertion ----------
    const size_t BATCH = 4096;
    Timer t;
    t.start();

    size_t inserted = 0;
    size_t peak = get_memory_usage();          // track high-water RSS

    for (size_t i = 0; i < total; i += BATCH) {
        size_t end = min(i + BATCH, total);
        for (size_t j = i; j < end; ++j) {
            bf.Insert(hash_kmer(s.substr(j, k)));
            ++inserted;

            // update peak RSS every 1 024 inserts
            if ((inserted & 1023) == 0)
                peak = max(peak, get_memory_usage());
        }
    }
    t.stop();
    uint64_t ins_ms = t.elapsed_ms() ? t.elapsed_ms() : 1; // avoid div-by-0

    // ---------- Positive lookups ----------
    const size_t lookTests = min(inserted, size_t(10'000));
    t.start();
    size_t ok = 0;
    for (size_t i = 0; i < lookTests; ++i) {
        size_t idx = (i * 97) % inserted;          // pseudo-random deterministic index
        if (bf.Lookup(hash_kmer(s.substr(idx, k))))
            ++ok;
    }
    t.stop();
    uint64_t look_ms = t.elapsed_ms() ? t.elapsed_ms() : 1;
    size_t   true_pos = ok;

    // ---------- Negative lookups ----------
    auto fp_kmers = generate_false_positive_tests(s, k, 10'000);
    size_t fp = 0;
    for (const auto& km : fp_kmers)
        if (bf.Lookup(hash_kmer(km)))
            ++fp;
    double fp_rate = static_cast<double>(fp) / fp_kmers.size();
    size_t true_neg = fp_kmers.size() - fp;

    // ---------- Package results ----------
    Result r{ sub_len,
              k,
              inserted,
              ins_ms,
              inserted * 1000.0 / ins_ms,             // throughput (elements / s)
              look_ms,
              true_pos,
              true_neg,
              fp_rate,
              peak / 1'048'576.0,                     // bytes → MB
              peak * 8.0 / inserted };                // bits per element
    return r;
}

// -----------------------------------------------------------------------------
//  main – CLI driver
// -----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    string fasta;

    // -------- Parse very simple CLI --------
    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a == "--fasta-path" && i + 1 < argc) {
            fasta = argv[++i];
        } else if (a == "--help") {
            cout << "--fasta-path PATH\n";
            return 0;
        }
    }
    if (fasta.empty()) {
        cerr << "--fasta-path required\n";
        return 1;
    }

    // -------- Load genome FASTA --------
    string genome;
    read_fasta(fasta, genome);

    // -------- Parameter grid --------
    vector<size_t> lens = {10'000, 50'000, 100'000, 500'000, 1'000'000, 5'000'000};
    vector<int>    ks   = {15, 20, 25, 30};

    // -------- CSV header --------
    ofstream csv("subarray_kmer_results.csv");
    csv << "subarray_len,kmer_size,num_elements,insert_time_ms,insert_throughput,"
           "true_positives,true_negatives,false_positive_rate,peak_memory_mb,bits_per_element\n";

    // -------- Run all experiments --------
    for (size_t len : lens) {
        for (int k : ks) {
            auto res = benchmark(genome, len, k);
            csv << res.sub_len     << ','
                << res.k           << ','
                << res.inserted    << ','
                << res.ins_ms      << ','
                << fixed << setprecision(2)
                << res.ins_tp      << ','
                << res.true_pos    << ','
                << res.true_neg    << ','
                << res.fp_rate * 100 << ','           // express as %
                << res.peak_mb     << ','
                << res.bpe         << '\n';
        }
    }
    return 0;
}
