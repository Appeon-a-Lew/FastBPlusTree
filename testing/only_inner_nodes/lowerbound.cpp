#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../../doctest/doctest.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cstdint>
#include <cstring>

// Assuming lowerBoundEytzinger and related structures/functions are included here
int eyt_i = 0;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
int count = 0;
static inline u32 swap(u32 x) { return __builtin_bswap32(x); }
static inline u16 swap(u16 x) { return __builtin_bswap16(x); }

struct PageSlot
{
    uint16_t offset;
    uint8_t headLen;
    uint8_t remainderLen;
    union
    {
        uint32_t head;
        uint8_t headBytes[4];
    };
};
std::vector<uint8_t> uint32ToByteArray(uint32_t value)
{
    std::vector<uint8_t> arr(4);
    arr[0] = (value >> 24) & 0xFF;
    arr[1] = (value >> 16) & 0xFF;
    arr[2] = (value >> 8) & 0xFF;
    arr[3] = value & 0xFF;
    return arr;
}

static u32 extractKeyHead(u8 *&currentKey, unsigned &remainingLength)
{
    u32 extractedValue;
    if (remainingLength > 3)
    {
        extractedValue = swap(*reinterpret_cast<u32 *>(currentKey));
        currentKey += sizeof(u32);
        remainingLength -= sizeof(u32);
        return extractedValue;
    }

    // Handling cases where the key length is less than 4
    if (remainingLength == 0)
    {
        extractedValue = 0;
    }
    else if (remainingLength == 1)
    {
        extractedValue = static_cast<u32>(currentKey[0]) << 24;
    }
    else if (remainingLength == 2)
    {
        extractedValue = static_cast<u32>(swap(*reinterpret_cast<u16 *>(currentKey))) << 16;
    }
    else // remainingLength == 3
    {
        extractedValue = (static_cast<u32>(swap(*reinterpret_cast<u16 *>(currentKey))) << 16) | (static_cast<u32>(currentKey[2]) << 8);
    }

    remainingLength = 0;
    return extractedValue;
}
template <bool equalityOnly = false>
unsigned lowerBoundEytzinger(uint8_t *key, unsigned keyLength, PageSlot slot[])
{
    unsigned oldKeyLength = keyLength;
    u32 keyHead = extractKeyHead(key, keyLength);

    size_t k = 0;
    // full binary search
    while (k < count)
    {
        if (slot[k].head > keyHead)
        {
            k = 2 * k + 1;
        }

        else if ((slot[k].head < keyHead))
        {
            k = 2 * k + 2;
        }

        else
        {
            return k;
        }
    }
    // there is no exact match
    if (equalityOnly)
        return -1;
    auto j = (k + 1) >> __builtin_ffs(~(k + 1));

    return j == 0 ? count : j - 1;
}

void eytzinger(const PageSlot *sortedArray, PageSlot *eytzingerArray, int n, int k = 1)
{
    if (k <= n)
    {
        eytzinger(sortedArray, eytzingerArray, n, 2 * k);
        eytzingerArray[k] = sortedArray[eyt_i++];
        eytzinger(sortedArray, eytzingerArray, n, 2 * k + 1);
    }
}

// Wrapper function
void convertToEytzinger(__restrict_arr PageSlot *sortedArray, int n)
{
    if (n == 0)
        return;
    __restrict_arr PageSlot eytzingerArray[n + 1]; // = new PageSlot[n + 1];
    PageSlot tmp = sortedArray[count];
    eyt_i = 0;
    eytzinger(sortedArray, eytzingerArray, n);
    std::copy(eytzingerArray + 1, eytzingerArray + n + 1, sortedArray);
    // memcpy(sortedArray,eytzingerArray+1,n);
    // delete[] eytzingerArray;
    // cout << times << endl;
    return;
}

// Function to generate a vector of integers of a given length, filled with random values.
std::vector<unsigned> generateRandomVector(int length)
{
    std::vector<unsigned> vec;
    srand(static_cast<unsigned int>(time(nullptr)));
    for (int i = 0; i < length; ++i)
    {
        vec.push_back(rand());
    }
    return vec;
}

// Test cases converted to doctest

TEST_CASE("EmptyArray")
{
    PageSlot slots[1];
    count = 0;
    uint8_t key[4] = {0};
    unsigned keyLength = sizeof(key);
    unsigned result = lowerBoundEytzinger<true>(reinterpret_cast<uint8_t *>(key), keyLength, slots);
    CHECK(result == -1);
}

TEST_CASE("SingleElement")
{
    PageSlot slots[1];
    slots[0] = {0, 4, 0, {1234}};
    count = 1;
    auto bytes = uint32ToByteArray(1234);
    auto result = lowerBoundEytzinger<true>(bytes.data(), bytes.size(), slots);
    CHECK(result == 0);
}

TEST_CASE("MultipleElements - Key Found")
{
    PageSlot slots[3];
    slots[0] = {0, 4, 0, {1000}};
    slots[1] = {0, 4, 0, {2000}};
    slots[2] = {0, 4, 0, {3000}};
    count = 3;
    convertToEytzinger(slots, 3);
    auto bytes = uint32ToByteArray(2000);
    auto result = lowerBoundEytzinger<true>(bytes.data(), bytes.size(), slots);
    CHECK(result == 0);
}

TEST_CASE("MultipleElements - Key Not Found")
{
    PageSlot slots[3];
    slots[0] = {0, 4, 0, {1000}};
    slots[1] = {0, 4, 0, {2000}};
    slots[2] = {0, 4, 0, {3000}};
    count = 3;
    convertToEytzinger(slots, 3);

    auto bytes = uint32ToByteArray(1234);
    auto result = lowerBoundEytzinger<true>(bytes.data(), bytes.size(), slots);
    CHECK(result == -1);
}


TEST_CASE("MultipleElements - Key Not Found (false)")
{
    PageSlot slots[3];
    slots[0] = {0, 4, 0, {1000}};
    slots[1] = {0, 4, 0, {2000}};
    slots[2] = {0, 4, 0, {3000}};
    count = 3;
    convertToEytzinger(slots, 3);

    auto bytes = uint32ToByteArray(900);
    auto result = lowerBoundEytzinger<false>(bytes.data(), bytes.size(), slots);
    CHECK(result == 1);
}
int standard_lower_bound(const std::vector<unsigned> &a, int x)
{
    auto it = std::lower_bound(a.begin(), a.end(), x);
    return (it != a.end()) ? distance(a.begin(), it) : -1; // Return -1 if x is greater than any element in a
}
TEST_CASE("VectorRandomness")
{
    std::vector<unsigned> vec = generateRandomVector(15);
    std::sort(vec.begin(), vec.end());
    PageSlot arr[15];
    for (size_t i = 0; i < 15; i++)
    {
        arr[i].head = vec[i];
    }
    convertToEytzinger(arr, 15);
    count = 15;
    auto bytes = uint32ToByteArray(vec.at(7));
    
    auto result_eyt = lowerBoundEytzinger<true>(bytes.data(), bytes.size(), arr);

    auto result_bin = standard_lower_bound(vec, vec.at(7));
    CHECK(arr[result_eyt].head == vec[result_bin]);
}


TEST_CASE("VectorRandomness large")
{
    std::vector<unsigned> vec = generateRandomVector(150);
    std::sort(vec.begin(), vec.end());
    PageSlot arr[150];
    for (size_t i = 0; i < 150; i++)
    {
        arr[i].head = vec[i];
    }
    convertToEytzinger(arr, 150);
    count = 150;
    auto bytes = uint32ToByteArray(vec.at(9));
    
    auto result_eyt = lowerBoundEytzinger<true>(bytes.data(), bytes.size(), arr);

    auto result_bin = standard_lower_bound(vec, vec.at(9));
    CHECK(arr[result_eyt].head == vec[result_bin]);
}
