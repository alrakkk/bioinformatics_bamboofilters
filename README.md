# bamboofilters for bioinformatics

A high-performance approximate membership query data structure for k-mer analysis in genomic data. Bamboo filters implement insertion, lookup, and deletion operations with constant-time cost, and support dynamic resizing through incremental expansion of the hash table.

For a closer look at how the filter is organised internally, see [`docs/algorithm.md`](docs/algorithm.md).

Example results from running this tool are provided in [`docs/subarray_kmers_results.md`](docs/subarray_kmers_results.md).

## Features

- **High throughput** for insertions, lookups, and deletions
- **Low false positive rates** (configurable based on memory usage)
- **Memory efficient** storage of k-mers
- **Dynamic resizing** capability without rebuilding the entire structure
- **Optimized for genomic data** with support for variable k-mer sizes

## Installation

```bash
# Clone the repository
git clone https://github.com/alrakkk/bioinformatics_bamboofilters.git
cd bioinformatics_bamboofilters

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make
```

### Required Packages

Building the tools and tests requires several system libraries:

- **libcurl**
- **zlib**
- **OpenSSL**

Installation hints for common platforms:

```bash
# Debian/Ubuntu
sudo apt-get install libcurl4-openssl-dev zlib1g-dev libssl-dev

# Fedora/RHEL
sudo dnf install libcurl-devel zlib-devel openssl-devel

# macOS (Homebrew)
brew install curl zlib openssl
```

## E. coli Benchmark

The Bamboo Filter includes tools to benchmark performance using the E. coli reference genome.
The reference genome file `data/GCF_000005845.2_ASM584v2_genomic.fna` is the *E. coli* K-12 MG1655 assembly obtained from the National Center for Biotechnology Information (NCBI) RefSeq database (assembly accession [GCF_000005845.2](https://www.ncbi.nlm.nih.gov/assembly/GCF_000005845.2)). NCBI places this sequence data in the public domain as a U.S. Government work.

### Download E. coli Genome

```bash
make download_ecoli
```

## Usage

The main benchmark tool is `bamboofilter_ecoli` which supports various operations:

```bash
# Basic usage with default parameters
./tools/bamboofilter_ecoli --fasta-path /path/to/ecoli.fasta --auto-test --false-positives


# Test with different k-mer sizes
./tools/bamboofilter_ecoli --fasta-path /path/to/ecoli.fasta --kmer-size 15

# Test deletion performance
./tools/bamboofilter_ecoli --fasta-path /path/to/ecoli.fasta --action delete --kmer-size 25

# Control the maximum number of k-mers to process
./tools/bamboofilter_ecoli --fasta-path /path/to/ecoli.fasta --kmer-size 25 --max-kmers 50000

```

For experiments with fixed-length subarrays of the genome and automatic false
positive measurement you can use the `subarray_kmers` tool:

```bash
./tools/subarray_kmers --fasta-path /path/to/ecoli.fasta
```

## Understanding Benchmark Results

The benchmark tool outputs detailed performance metrics to `bamboo_filter_results.csv` with the following columns:

- `kmer_size`: The size of k-mers being tested
- `num_elements`: Number of k-mers inserted
- `insert_time_ms`: Time taken to insert all k-mers (ms)
- `insert_throughput`: Insert operations per second
- `lookup_time_ms`: Time taken for lookups (ms)
- `false_positive_rate`: Percentage of false positives (lower is better)
- `peak_memory_mb`: Maximum memory usage (MB)
- `bits_per_element`: Memory efficiency metric
-                                                                                                                                                       |

## Performance Results

Benchmarks show that Bamboo Filters outperform existing alternatives:

- Highest insertion throughput 
- Superior positive lookup throughput
- Efficient deletion operations 
- Minimal performance degradation after multiple resizing operations
- Lower false positive rates for genomic data than comparable structures
- Memory efficiency comparable to state-of-the-art AMQ data structures


## References

1. National Center for Biotechnology Information (NCBI). *Escherichia coli* str. K-12 substr. MG1655, complete genome. Assembly accession GCF_000005845.2, RefSeq. Available from: https://www.ncbi.nlm.nih.gov/assembly/GCF_000005845.2. Public Domain.
2. Wang, Y., Wei, Z., Liu, G., & Sun, L. (2022). Bamboo Filters: Make Resizing Smooth. In 2022 IEEE 38th International Conference on Data Engineering (ICDE) (pp. 1421-1433). IEEE. doi: 10.1109/ICDE53745.2022.00078
3. Wang, Y., Wei, Z., Liu, G., & Sun, L. (2024). Bamboo Filters: Make Resizing Smooth and Adaptive. IEEE/ACM Transactions on Networking. doi: 10.1109/TNET.2024.3403997
4. Fan, B., Andersen, D. G., & Kaminsky, M. (2013). Cuckoo Filter: Better Than Bloom. USENIX ;login:, 38(4), 36-40. Retrieved from https://www.cs.cmu.edu/~binfan/papers/login_cuckoofilter.pdf
5. Fan, B., Andersen, D. G., Kaminsky, M., & Mitzenmacher, M. D. (2014). Cuckoo Filter: Practically Better Than Bloom. In Proceedings of the 10th ACM International on Conference on emerging Networking Experiments and Technologies (pp. 75-88). ACM. Retrieved from http://www.cs.cmu.edu/~binfan/papers/conext14_cuckoofilter.pdf



This project was developed as part of the [Bioinformatics 1](https://www.fer.unizg.hr/en/course/enbio1) course at the Faculty of Electrical Engineering and Computing (FER), University of Zagreb.
