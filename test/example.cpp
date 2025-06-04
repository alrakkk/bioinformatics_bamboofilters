#include <string>
#include <cmath>
#include <iostream>

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <random>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <chrono>

#include "bamboofilter/bamboofilter.hpp"
#include "bamboofilter/bitsutil.h"



using namespace std;

uint64_t NowNanos() {
    return chrono::duration_cast<chrono::nanoseconds>(
        chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

// Replacement for common/random.h
void GenerateRandom64(size_t count, vector<string>& to_add, vector<string>& to_lookup) {
    to_add.resize(count);
    to_lookup.resize(count);
    
    random_device rd;
    mt19937_64 gen(rd());
    
    for (size_t i = 0; i < count; i++) {
        string str = to_string(gen());
        to_add[i] = str;
        to_lookup[i] = str;
    }
}

int main(int argc, char *argv[])
{
    size_t add_count = 65536;

    cout << "Prepare..." << endl;

    vector<string> to_add, to_lookup;
    GenerateRandom64(add_count, to_add, to_lookup);

    cout << "Begin test" << endl;

    BambooFilter *bbf = new BambooFilter(upper_power2(65536), 2);
    auto start_time = NowNanos();

    for (uint64_t added = 0; added < add_count; added++)
    {
        bbf->Insert(to_add[added].c_str());
    }

    cout << ((add_count * 1000.0) / static_cast<double>(NowNanos() - start_time)) << endl;

    start_time = NowNanos();
    for (uint64_t added = 0; added < add_count; added++)
    {
        if (!bbf->Lookup(to_add[added].c_str()))
        {
            throw logic_error("False Negative");
        }
    }
    cout << ((add_count * 1000.0) / static_cast<double>(NowNanos() - start_time)) << endl;

    return 0;
}