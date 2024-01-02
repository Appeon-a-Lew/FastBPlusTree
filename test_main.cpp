#include "tester2.hpp"
#include "PerfEvent.hpp"
#include <algorithm>
#include <csignal>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>

using namespace std;

void runTest(vector<vector<uint8_t>> &keys,PerfEvent& perf) {
    std::random_shuffle(keys.begin(), keys.end());
    Tester* t = new Tester();

    std::vector<uint8_t> emptyKey{};
    uint64_t count = keys.size();

    {
        printf("INSERT STARTS\n");
        PerfEventBlock peb(perf,count,{"insert"});
        for (uint64_t i = 0; i < count; ++i) {
            t->insert(keys[i], keys[i]);
        }
    }
    string str(keys[count/2].begin(), keys[count/2].end()) ;
    cout << string_to_hex(str) << endl;
    // t->btree->root->print();
    {
        printf("SCAN STARTS\n");
        PerfEventBlock peb(perf,count/5,{"scan"});
        for (uint64_t i = 0; i < count; i += 5) {
            // cout << i << endl;
            unsigned limit = 10;
            t->scan(keys[i], [&](uint16_t, uint8_t *, uint16_t) {
                limit -= 1;
                return limit > 0;
            });
            // printf("SCAN SUCKS: %d\n",i);
        }
    }
    {
        PerfEventBlock peb(perf,count,{"lookup"});
        for (uint64_t i = 0; i < count; ++i) {
            t->lookup(keys[i]);
        }
    }
    {
        PerfEventBlock peb(perf,count,{"remove"});
        for (uint64_t i = 0; i < count; ++i) {
            t->remove(keys[i]);
        }
    }
    t->~Tester();
}

std::vector<uint8_t> stringToVector(const std::string &str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}

int main() {
    srand(42);
    PerfEvent perf;
    if (getenv("INT")) {
        vector<vector<uint8_t>> data;
        vector<uint64_t> v;
        uint64_t n = atof(getenv("INT"));
        for (uint64_t i = 0; i < n; i++)
            v.push_back(i);
        for (auto x: v) {
            union {
                uint32_t x;
                uint8_t bytes[4];
            } u;
            u.x = x;
            data.emplace_back(u.bytes, u.bytes + 4);
        }
        runTest(data,perf);
    }

    if (getenv("LONG1")) {
        vector<vector<uint8_t>> data;
        uint64_t n = atof(getenv("LONG1"));
        for (unsigned i = 0; i < n; i++) {
            string s;
            for (unsigned j = 0; j < i; j++)
                s.push_back('A');
            data.push_back(stringToVector(s));;
        }
        runTest(data,perf);
    }

    if (getenv("LONG2")) {
        vector<vector<uint8_t>> data;
        uint64_t n = atof(getenv("LONG2"));
        for (unsigned i = 0; i < n; i++) {
            string s;
            for (unsigned j = 0; j < i; j++)
                s.push_back('A' + random() % 60);
            data.push_back(stringToVector(s));
        }
        runTest(data,perf);
    }

    if (getenv("FILE")) {
        vector<vector<uint8_t>> data;
        ifstream in(getenv("FILE"));
        string line;
        while (getline(in, line))
            data.push_back(stringToVector(line));;
        runTest(data,perf);
    }

    return 0;
}