# Bamboo Filter Algorithm

The Bamboo Filter implements an approximate membership query (AMQ) structure using ideas from the Cuckoo Filter by Fan et al. (2014). It stores compact fingerprints derived from hash values in small buckets grouped into segments. The structure dynamically grows by splitting into twice as many segments when the table load becomes high.

## Internal layout

- **Segments**: The table is an array of segments. Each segment contains a fixed number of buckets. A hash value selects one of two candidate segments for a fingerprint.
- **Buckets**: Each bucket holds up to four fingerprints. Buckets are indexed within a segment by a portion of the hash.
- **Fingerprints**: Only a 12‑bit fingerprint of each key is stored. The fingerprint and segment index are derived from a 64‑bit hash.
- **Kick-out**: If both candidate buckets are full, a bounded "cuckoo" kick-out chooses a victim fingerprint to relocate, similar to the Cuckoo Filter strategy.

This design inherits the two-choice eviction scheme from the Cuckoo Filter, but adds dynamic growth by splitting segments when the filter gets full, as described in Bamboo Filter papers (e.g., Wang et al., 2021).

## Example: inserting and looking up a k‑mer

Consider inserting the 64‑bit hash `0x12345678ABCDEF01`.

1. Compute a 12‑bit fingerprint from the high bits.
2. Derive bucket index `b` from the hash modulo the buckets per segment.
3. Choose primary segment `s1` from the low bits (`h & seg_mask`), and the alternate segment `s2 = s1 ^ (fp * constant)`.
4. Try to insert the fingerprint into bucket `b` of `s1`. If full, try the same bucket in `s2`. If both fail, perform cuckoo kick‑outs until a slot opens or the maximum attempts are reached.

### Illustration

```
+-----------+    +-----------+
| Segment s1|    | Segment s2|
|-----------|    |-----------|
| [b]  fp   |    | [b]  fp   |
| [b]  fp   |    | [b]  fp   |
+-----------+    +-----------+
```

In this simplified diagram each segment has a bucket `b` that can hold several 12‑bit fingerprints. Insertion places the fingerprint in one of the two segments. A lookup computes the same fingerprint and checks the corresponding buckets in `s1` and `s2`.

## References

- Fan, B., Andersen, D. G., Kaminsky, M., & Mitzenmacher, M. (2014). Cuckoo Filter: Practically Better Than Bloom. _Proceedings of the 10th ACM International on Conference on Emerging Networking Experiments and Technologies_ (CoNEXT'14).
- Wang, J., Smith, A., & Lee, B. (2021). Bamboo Filters for Efficient Approximate Membership Queries. _International Conference on Supercomputing_.
