# bamboofilters for bioinformatics

A high-performance approximate membership query data structure for k-mer analysis in genomic data. Bamboo filters implement insertion, lookup, and deletion operations with constant-time cost, and support dynamic resizing through incremental expansion of the hash table.

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

## E. coli Benchmark

The Bamboo Filter includes tools to benchmark performance using the E. coli reference genome.

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
