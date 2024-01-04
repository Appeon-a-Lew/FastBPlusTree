#pragma once 
#include <vector>
#include <algorithm>
#include <iostream>
#include <chrono>

// Helper function to build Eytzinger array recursively
template <typename T>
void buildEytzinger(const std::vector<T> &sorted, std::vector<T> &eytzinger, int index, int &sortedIndex)
{
    if (index < eytzinger.size())
    {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        buildEytzinger(sorted, eytzinger, left, sortedIndex);
        eytzinger[index] = sorted[sortedIndex++];
        buildEytzinger(sorted, eytzinger, right, sortedIndex);
    }
}

// Convert sorted array to Eytzinger array
template <typename T>
std::vector<T> toEytzinger(const std::vector<T> &sorted)
{
    std::vector<T> eytzinger(sorted.size());
    int sortedIndex = 0;
    buildEytzinger(sorted, eytzinger, 0, sortedIndex);
    return eytzinger;
}

// Convert Eytzinger array back to sorted array (trivial, just sort it)
template <typename T>
std::vector<T> fromEytzinger(const std::vector<T> &eytzinger)
{
    std::vector<T> sorted(eytzinger);
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

// Regular binary search
template <typename T>
int binarySearch(const std::vector<T> &arr, T target)
{
    int left = 0, right = arr.size() - 1;
    while (left <= right)
    {
        int mid = left + (right - left) / 2;
        if (arr[mid] == target)
            return mid;
        else if (arr[mid] < target)
            left = mid + 1;
        else
            right = mid - 1;
    }
    return -1; // not found
}

// Eytzinger layout binary search
template <typename T>
int eytzingerSearch(const std::vector<T> &arr, T target)
{
    int index = 0;
    while (index < arr.size() && arr[index] != target)
    {
        index = 2 * index + 1 + (arr[index] < target);
    }
    return (index < arr.size() && arr[index] == target) ? index : -1;
}

// Template function to find the lower bound using either binary search
template <template <typename> class SearchFunc, typename T>
int lowerBound(const std::vector<T> &arr, T value)
{
    int index = SearchFunc(arr, value);
    // Handle the logic for finding the lower bound if not directly found
    // ...
    return index;
}

#include <map>
#include <random>
#include <cassert>

// Function to test correctness of Eytzinger layout against std::map
template <typename T>
void testEytzingerCorrectness(const std::vector<T> &sorted)
{
    // Convert to Eytzinger layout
    std::vector<T> eytzinger = toEytzinger(sorted);

    // Convert back from Eytzinger layout
    std::vector<T> backToSorted = fromEytzinger(eytzinger);

    // Compare the result with the original sorted array
    assert(sorted == backToSorted && "Eytzinger conversion failed!");
}

// Function to test correctness of binary search functions against std::map
template <typename T>
void testSearchCorrectness(int (*searchFunc)(const std::vector<T> &, T), const std::vector<T> &arr, const std::map<T, int> &referenceMap)
{
    for (const auto &pair : referenceMap)
    {
        T value = pair.first;
        int expectedIndex = pair.second;
        int foundIndex = searchFunc(arr, value);

        // Compare the results
        assert((foundIndex == expectedIndex || (foundIndex == -1 && referenceMap.find(value) == referenceMap.end())) && "Search function failed!");
    }
}

std::map<int, int> createEytzingerReferenceMap(const std::vector<int> &eytzinger)
{
    std::map<int, int> refMap;
    for (int i = 0; i < eytzinger.size(); ++i)
    {
        refMap[eytzinger[i]] = i;
    }
    return refMap;
}

// Function to generate a linear sequence
std::vector<int> generateLinearSequence(int size, int start = 0, int step = 1)
{
    std::vector<int> seq(size);
    for (int i = 0; i < size; ++i)
    {
        seq[i] = start + i * step;
    }
    return seq;
}

// Function to generate a random sequence
std::vector<int> generateRandomSequence(int size, int minVal = 0, int maxVal = 1000)
{
    std::vector<int> seq(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(minVal, maxVal);

    for (int i = 0; i < size; ++i)
    {
        seq[i] = distrib(gen);
    }
    return seq;
}

// Function to create a reference map from a sequence
std::map<int, int> createReferenceMap(const std::vector<int> &seq)
{
    std::map<int, int> refMap;
    for (int i = 0; i < seq.size(); ++i)
    {
        refMap[seq[i]] = i;
    }
    return refMap;
}

int main2()
{
    // Create a large sorted array
    std::vector<int> sortedArray(1000000);
    for (int i = 0; i < sortedArray.size(); ++i)
    {
        sortedArray[i] = i * 2; // Even numbers
    }

    // Convert to Eytzinger array
    std::vector<int> eytzingerArray = toEytzinger(sortedArray);

    // Measure time taken by regular binary search
    auto start = std::chrono::high_resolution_clock::now();
    binarySearch(sortedArray, 123456);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Regular binary search time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " microseconds\n";

    // Measure time taken by Eytzinger binary search
    start = std::chrono::high_resolution_clock::now();
    eytzingerSearch(eytzingerArray, 123456);
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Eytzinger binary search time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " microseconds\n";

    // Generate test data
    std::vector<int> linearSeq = generateLinearSequence(1000);
    std::vector<int> randomSeq = generateRandomSequence(1000);
    std::sort(randomSeq.begin(), randomSeq.end());

    // Create reference maps
    std::map<int, int> linearMap = createReferenceMap(linearSeq);
    std::map<int, int> randomMap = createReferenceMap(randomSeq);

    // Test Eytzinger correctness
    testEytzingerCorrectness(linearSeq);
    testEytzingerCorrectness(randomSeq);

    std::vector<int> eytzingerLinearSeq = toEytzinger(linearSeq);
    std::map<int, int> eytzingerLinearMap = createEytzingerReferenceMap(eytzingerLinearSeq);

    // Convert randomSeq to Eytzinger and create a reference map for it
    std::vector<int> eytzingerRandomSeq = toEytzinger(randomSeq);
    std::map<int, int> eytzingerRandomMap = createEytzingerReferenceMap(eytzingerRandomSeq);

    // Test search correctness
    testSearchCorrectness(binarySearch, linearSeq, linearMap);
    std::cout << "first" << std::endl;
    testSearchCorrectness(eytzingerSearch, eytzingerLinearSeq, eytzingerLinearMap);
    std::cout << "second" << std::endl;
    testSearchCorrectness(binarySearch, randomSeq, randomMap);
    std::cout << "third" << std::endl;

    testSearchCorrectness(eytzingerSearch, eytzingerRandomSeq, eytzingerRandomMap);

    std::cout << "All tests passed successfully!" << std::endl;

    return 0;
}
