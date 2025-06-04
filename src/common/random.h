// Generating random data
#ifndef RANDOM_H_
#define RANDOM_H_

#include <algorithm>
#include <cstdint>
#include <functional>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
using std::string;
using std::unordered_set;
using std::vector;

void GenerateRandom64(::std::size_t count, vector<string> &to_add, vector<string> &to_lookup)
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    
    to_add.resize(count);
    to_lookup.resize(count);
    
    for (size_t i = 0; i < count; i++) {
        std::string str = std::to_string(gen());
        to_add[i] = str;
        to_lookup[i] = str;
    }
}

// Using two pointer ranges for sequences x and y, create a vector clone of x but for
// y_probability y's mixed in.
template <typename T>
::std::vector<T> MixIn(const T *x_begin, const T *x_end, const T *y_begin,
                       const T *y_end, double y_probability)
{
    const size_t x_size = x_end - x_begin, y_size = y_end - y_begin;
    if (y_size > (1ull << 32))
        throw ::std::length_error("y is too long");
    ::std::vector<T> result(x_begin, x_end);
    ::std::random_device random;
    auto genrand = [&random, y_size]() {
        return (static_cast<size_t>(random()) * y_size) >> 32;
    };
    for (size_t i = 0; i < y_probability * x_size; ++i)
    {
        result[i] = *(y_begin + genrand());
    }
    ::std::shuffle(result.begin(), result.end(), random);
    return result;
}
#endif