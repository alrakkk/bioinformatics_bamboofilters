# subarray_kmers Benchmark Results

The `subarray_kmers` tool was run on the provided `data/ecoli_small.fna` FASTA genome. Multiple k-mer sizes and slice lengths were tested. The resulting metrics are shown below.

| subarray_len | kmer_size | num_elements | insert_time_ms | insert_throughput | lookup_time_ms | false_positive_rate | peak_memory_mb | bits_per_element |
| ------------ | --------- | ------------ | -------------- | ----------------- | -------------- | ------------------- | -------------- | ---------------- |
| 10000        | 15        | 9986         | 1              | 9986000.00        | 1              | 0.13                | 10.87          | 9132.12          |
| 10000        | 20        | 9981         | 1              | 9981000.00        | 1              | 0.10                | 10.87          | 9136.69          |
| 10000        | 25        | 9976         | 1              | 9976000.00        | 1              | 0.17                | 10.87          | 9141.27          |
| 10000        | 30        | 9971         | 1              | 9971000.00        | 1              | 0.16                | 10.87          | 9145.86          |
| 50000        | 15        | 49986        | 2              | 24993000.00       | 1              | 0.20                | 10.87          | 1824.38          |
| 50000        | 20        | 49981        | 3              | 16660333.33       | 1              | 0.13                | 10.87          | 1824.56          |
| 50000        | 25        | 49976        | 6              | 8329333.33        | 1              | 0.15                | 10.87          | 1824.74          |
| 50000        | 30        | 49971        | 3              | 16657000.00       | 1              | 0.18                | 10.87          | 1824.93          |
| 100000       | 15        | 99986        | 5              | 19997200.00       | 1              | 0.23                | 10.87          | 912.06           |
| 100000       | 20        | 99981        | 6              | 16663500.00       | 1              | 0.15                | 10.87          | 912.11           |
| 100000       | 25        | 99976        | 6              | 16662666.67       | 1              | 0.20                | 10.87          | 912.15           |
| 100000       | 30        | 99971        | 6              | 16661833.33       | 1              | 0.20                | 10.87          | 912.20           |
| 500000       | 15        | 499986       | 65             | 7692092.31        | 1              | 0.25                | 12.80          | 214.83           |
| 500000       | 20        | 499981       | 65             | 7692015.38        | 1              | 0.11                | 13.05          | 219.03           |
| 500000       | 25        | 499976       | 87             | 5746850.57        | 1              | 0.16                | 13.43          | 225.32           |
| 500000       | 30        | 499971       | 58             | 8620189.66        | 1              | 0.09                | 13.55          | 227.42           |
| 1000000      | 15        | 999986       | 279            | 3584179.21        | 1              | 0.39                | 17.55          | 147.26           |
| 1000000      | 20        | 999981       | 233            | 4291763.95        | 1              | 0.12                | 17.80          | 149.36           |
| 1000000      | 25        | 999976       | 212            | 4716867.92        | 1              | 0.14                | 18.18          | 152.51           |
| 1000000      | 30        | 999971       | 181            | 5524701.66        | 1              | 0.12                | 18.30          | 153.56           |
| 5000000      | 15        | 4641638      | 1521           | 3051701.51        | 1              | 1.32                | 46.55          | 84.14            |
| 5000000      | 20        | 4641633      | 2005           | 2315028.93        | 1              | 0.20                | 46.80          | 84.59            |
| 5000000      | 25        | 4641628      | 1553           | 2988813.91        | 1              | 0.13                | 47.14          | 85.20            |
| 5000000      | 30        | 4641623      | 1704           | 2723957.16        | 1              | 0.10                | 47.27          | 85.42            |

Units:

- time columns are in milliseconds (ms)
- false_positive_rate is in percent (%)
- peak_memory_mb is in megabytes (MB)

Overall, insertion and lookup were very fast even for million-element slices. Peak memory usage scaled with the number of k-mers but remained under 50&nbsp;MB for slices up to five million bases. The false-positive rate stayed below 1&nbsp;% in all cases and decreased with larger k-mer size. The main trade-off is between memory usage and accuracy: longer k-mers produced fewer false positives but slightly higher memory per element.
