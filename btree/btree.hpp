/**
 * @file btree.hpp
 * @brief B+ tree implementation
 * @date 2023-11-19
 * @author Ismail Safa Toy
 * inspiration from leanstore
 */

#pragma once

#include <iostream>
#include <fstream>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <cassert>
#include <string>
#include <x86intrin.h>
#include <functional>

#include <chrono>
#include <stack>
#include <thread>

#include "../common.h"
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using namespace std;
struct BTreeNode;
using SwipType = BTreeNode *;
static inline u64 swap(u64 x) { return __builtin_bswap64(x); }
static inline u32 swap(u32 x) { return __builtin_bswap32(x); }
static inline u16 swap(u16 x) { return __builtin_bswap16(x); }
static int counter = 0;
static int times = 0;
struct BTreeNodeHeader
{
    static const unsigned PAGE_SIZE = 1024 *4;
    static const unsigned under_full = PAGE_SIZE * 0.6;
    static constexpr u8 limit = 254;
    static constexpr u8 marker = 255;



    struct FenceKey
    {
        u16 offset;
        u16 length;
    };

    BTreeNode *upper = nullptr;
    FenceKey lower_fence = {0, 0};
    FenceKey upper_fence = {0, 0};

    u16 count = 0;
    bool is_leaf;
    u16 space_used = 0;
    u16 free_offset = static_cast<u16>(PAGE_SIZE);
    u16 prefix_len = 0;

    static const unsigned hintCount = 16;
    u32 hint[hintCount];

    BTreeNodeHeader(bool isLeaf)
        : is_leaf(isLeaf) {}
    ~BTreeNodeHeader() {}

    bool is_eyt = false;
    int eyt_i = 0;
    // int furthest_point_eyt; // in the sorted we can check the max space with: free_offset - (reinterpret_cast<u8 *>(slot + count) - ptr()
                            //  but in eyt the count is not indicative of the furthest_slot -> this sneaky index

    inline u8 *ptr() { return reinterpret_cast<u8 *>(this); }
    inline bool isInner() { return !is_leaf; }
    inline u8 *getLowerFenceKey() { return lower_fence.offset ? ptr() + lower_fence.offset : nullptr; }
    inline u8 *getUpperFenceKey() { return upper_fence.offset ? ptr() + upper_fence.offset : nullptr; }
};
struct BTreeNode : public BTreeNodeHeader
{

    struct PageSlot
    {
        u16 offset;
        u8 headLen;
        u8 remainderLen;
        union
        {
            u32 head;
            u8 headBytes[4];
        };
    } __attribute__((packed));
    const static size_t slotnum = (PAGE_SIZE - sizeof(BTreeNodeHeader)) / (sizeof(PageSlot));

    __restrict_arr alignas(8) PageSlot slot[slotnum];

    int get_times()
    {
        return times;
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
        eyt_i = 0;
        // __builtin_prefetch(sortedArray);
        eytzinger(sortedArray, eytzingerArray, n);
        std::copy(eytzingerArray + 1, eytzingerArray + n + 1, sortedArray);
        // memcpy(sortedArray,eytzingerArray+1,n* sizeof(PageSlot));
        // delete[] eytzingerArray;
        is_eyt = true;
        assert(checkEytzingerLayout(slot, count));
        times++;
        return;
    }

    void fromEytzingerLayout(const PageSlot *eytzingerArray, PageSlot *sortedArray, int index, int n)
    {
        if (index >= n)
        {
            return;
        }
        fromEytzingerLayout(eytzingerArray, sortedArray, 2 * index + 1, n);
        sortedArray[eyt_i++] = eytzingerArray[index];
        fromEytzingerLayout(eytzingerArray, sortedArray, 2 * index + 2, n);
    }

    // Wrapper function
    void convertFromEytzinger(__restrict_arr PageSlot *eytzingerArray, int n, PageSlot tmp)
    {
        if (n == 0)
            return;

        __restrict_arr PageSlot sortedArray[n]; // = new PageSlot[n];
        eyt_i = 0;
        // __builtin_prefetch(eytzingerArray);
        fromEytzingerLayout(eytzingerArray, sortedArray, 0, n);
        std::copy(sortedArray, sortedArray + n, eytzingerArray);
        // memcpy(eytzingerArray,sortedArray,n * sizeof(PageSlot));
        slot[count] = tmp;
        // delete[] sortedArray;
        is_eyt = false;
        times++;
        assert(isSorted(eytzingerArray,count));
    }

    bool isEytzingerLayout(const PageSlot *array, int index, int n)
    {
        int leftChildIndex = 2 * index + 1;
        int rightChildIndex = 2 * index + 2;
        if (leftChildIndex >= n)
            return true;
        if (rightChildIndex >= n && array[leftChildIndex].head < array[index].head)
            return true;

        // Check if left child exists and if its key is less than or equal to the parent's key
        if (leftChildIndex < n && array[leftChildIndex].head > array[index].head)
        {
            std::cout << "The array is wrong at " << index << "and  the left is " << leftChildIndex << std::endl;
            return false;
        }

        // Check if right child exists and if its key is greater than or equal to the parent's key
        if (rightChildIndex < n && array[rightChildIndex].head < array[index].head)
        {
            std::cout << "The array is wrong at " << index << "and  the right is " << rightChildIndex << std::endl;

            return false;
        }

        // Recursively check for children nodes if they exist
        return isEytzingerLayout(array, leftChildIndex, n) &&
               isEytzingerLayout(array, rightChildIndex, n);
    }

    // Wrapper function to start from the root
    bool checkEytzingerLayout(const PageSlot *array, int n)
    {
        if (n == 0)
            return true; // Empty array is trivially in Eytzinger layout
        return isEytzingerLayout(array, 0, n);
    }

    bool isSorted(const PageSlot *array, int n)
    {
        for (int i = 1; i < n; ++i)
        {
            if (array[i].head < array[i - 1].head)
            {
                cout << "At the index: " << i << " and the cpunt " << count << endl;
                cout << "arr[" << i << "]: " << array[i].head << " arr[" << i - 1 << "]: " << array[i - 1].head << endl;

                cout << "arr[" << i << "].headLen: " << +array[i].headLen << " arr[" << i - 1 << "]headLen: " << +array[i - 1].headLen << endl;
                cout << "arr[" << i << "].remainderLen: " << +array[i].remainderLen << " arr[" << i - 1 << "]remainderLen: " << +array[i - 1].remainderLen << endl;

                return false;
            }
            if (array[i].head == array[i - 1].head && (cmpKeys(isLarge(i - 1) ? getRemainderLarge(i - 1) : getRest(i - 1),
                                                               isLarge(i) ? getRemainderLarge(i) : getRest(i),
                                                               isLarge(i - 1) ? getRestLenLarge(i - 1) : getRemainderLength(i - 1),
                                                               isLarge(i) ? getRestLenLarge(i) : getRemainderLength(i)) > 0))
            {

                cout << "At the index: " << i << " and the cpunt " << count << " is_eyt " << is_eyt << endl;
                cout << "arr[" << i << "]: " << +array[i].head << " arr[" << i - 1 << "]: " << +array[i - 1].head << endl;
                cout << "arr[" << i << "].remainderLen: " << +array[i].remainderLen << " arr[" << i - 1 << "]remainderLen: " << +array[i - 1].remainderLen << endl;

                for (size_t i = 0; i < count; i++)
                {
                    cout << "[head: " << slot[i].head << ", remainder: " << +slot[i].remainderLen << " ]";
                }
                cout << endl;
                return false;
            }
        }
        return true;
    }
    BTreeNode(bool is_leaf)
        : BTreeNodeHeader(is_leaf)
    {
        for (size_t i = 0; i < slotnum; i++)
        {
            slot[i] = {0, 0, 0, {0}};
        }
    }

    unsigned freeSpace() { return free_offset - (reinterpret_cast<u8 *>(slot + count) - ptr()); }
    unsigned spacePostCompact() { return PAGE_SIZE - (reinterpret_cast<u8 *>(slot + count) - ptr()) - space_used; }

    bool allocateSpace(unsigned spaceNeeded)
    {
        unsigned space = freeSpace();
        if (spaceNeeded <= space)
            return true;
        if (spaceNeeded <= spacePostCompact())
        {
            compact();
            return true;
        }
        return false;
    }

    static BTreeNode *makeLeaf() { return new BTreeNode(true); }
    static BTreeNode *makeInner() { return new BTreeNode(false); }
    inline u8 *getRest(unsigned slot_id)
    {
        assert(!isLarge(slot_id));
        return ptr() + slot[slot_id].offset + sizeof(SwipType);
    }
    inline unsigned getRemainderLength(unsigned slot_id)
    {
        assert(!isLarge(slot_id));
        return slot[slot_id].remainderLen;
    }
    inline u8 *getPayload(unsigned slot_id)
    {
        assert(!isLarge(slot_id));
        return ptr() + slot[slot_id].offset + sizeof(SwipType) + getRemainderLength(slot_id);
    }

    inline u8 *getRemainderLarge(unsigned slot_id)
    {
        assert(isLarge(slot_id));
        return ptr() + slot[slot_id].offset + sizeof(SwipType) + sizeof(u16);
    }
    inline u16 &getRestLenLarge(unsigned slot_id)
    {
        assert(isLarge(slot_id));
        return *reinterpret_cast<u16 *>(ptr() + slot[slot_id].offset + sizeof(SwipType));
    }
    inline bool isLarge(unsigned slot_id) { return slot[slot_id].remainderLen == marker; }
    inline void setLarge(unsigned slot_id) { slot[slot_id].remainderLen = marker; }

    inline u8 *getPayloadLarge(unsigned slot_id)
    {
        assert(isLarge(slot_id));
        return ptr() + slot[slot_id].offset + sizeof(SwipType) + sizeof(u16) + getRestLenLarge(slot_id);
    }

    inline u64 getPayloadLength(unsigned slot_id) { return *reinterpret_cast<u64 *>(ptr() + slot[slot_id].offset); }
    inline SwipType &getChild(unsigned slot_id) { return *reinterpret_cast<SwipType *>(ptr() + slot[slot_id].offset); }
    inline unsigned getFullKeyLength(unsigned slot_id) { return prefix_len + slot[slot_id].headLen + (isLarge(slot_id) ? getRestLenLarge(slot_id) : getRemainderLength(slot_id)); }

    inline void copyKeyOut(unsigned slot_id, u8 *out, unsigned key_len)
    {
        std::copy_n(getLowerFenceKey(), prefix_len, out);
        out += prefix_len;
        key_len -= prefix_len;

        auto &current_slot = slot[slot_id];
        auto headLen = current_slot.headLen;

        if (key_len >= headLen)
        {
            *reinterpret_cast<u32 *>(out) = swap(current_slot.head);
            memcpy(out + headLen, (isLarge(slot_id) ? getRemainderLarge(slot_id) : getRest(slot_id)), key_len - headLen);
        }
        else
        {
            switch (headLen)
            {
            case 4:
                *reinterpret_cast<u32 *>(out) = swap(current_slot.head);
                break;
            case 3:
                out[2] = current_slot.headBytes[1]; // fallthrough
            case 2:
                out[1] = current_slot.headBytes[2]; // fallthrough
            case 1:
                out[0] = current_slot.headBytes[3]; // fallthrough
            case 0:
                break;
            default:
                __builtin_unreachable();
            };
        }
    }

    static unsigned spaceNeeded(unsigned key_len, unsigned prefix_len)
    {
        assert(key_len >= prefix_len);
        auto restLen = key_len - prefix_len;
        if (restLen <= sizeof(u32))
            return sizeof(PageSlot) + sizeof(SwipType);
        restLen -= sizeof(u32);
        auto additional = (restLen > limit) ? sizeof(u16) : 0;
        return sizeof(PageSlot) + restLen + sizeof(SwipType) + additional;
    }

    static int cmpKeys(u8 *keyA, u8 *keyB, unsigned lengthA, unsigned lengthB)
    {
        auto res = std::memcmp(keyA, keyB, min(lengthA, lengthB));
        if (res)
            return res;
        return (lengthA - lengthB);
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

    void makeHint()
    {
        unsigned dist = count / (hintCount + 1);
        for (unsigned i = 0; i < hintCount; i++)
            hint[i] = slot[dist * (i + 1)].head;
    }

    template <bool equalityOnly = false>
    unsigned lowerBoundEytzinger_1_indexed(u8 *key, unsigned keyLength)
    {
        PageSlot arr_tmp[count + 1];
        std::copy(slot, slot + count, arr_tmp + 1);

        if (equalityOnly)
        {
            if ((keyLength < prefix_len) || (bcmp(key, getLowerFenceKey(), prefix_len) != 0))
                return -1;
        }
        else
        {
            int prefixCmp = cmpKeys(key, getLowerFenceKey(), min<unsigned>(keyLength, prefix_len), prefix_len);
            if (prefixCmp < 0)
            {
                return findSmallestChildEyt(count);
            }

            else if (prefixCmp > 0)
            {
                return findSmallestChildEyt(count)-1;
            }
        }
        key += prefix_len;
        keyLength -= prefix_len;

        unsigned oldKeyLength = keyLength;
        u32 keyHead = extractKeyHead(key, keyLength);

        size_t k = 1;
        // full binary search
        while (k <= count)
        {
            __builtin_prefetch(arr_tmp + k * 8);
            if (arr_tmp[k].head > keyHead)
            {
                k = 2 * k;
            }

            else if ((arr_tmp[k].head < keyHead))
            {
                k = 2 * k + 1;
            }

            else if (arr_tmp[k].remainderLen == 0)
            {
                if (oldKeyLength < arr_tmp[k].headLen)
                {
                    k = 2 * k;
                }
                else if (oldKeyLength > arr_tmp[k].headLen)
                {
                    k = 2 * k + 1;
                }
                else
                {
                    return k - 1;
                }
            }
            else
            {
                int cmp;
                if (isLarge(k - 1))
                {
                    cmp = cmpKeys(key, getRemainderLarge(k - 1), keyLength, getRestLenLarge(k - 1));
                }
                else
                {
                    cmp = cmpKeys(key, getRest(k - 1), keyLength, getRemainderLength(k - 1));
                }

                if (cmp < 0)
                {
                    k = 2 * k;
                }
                else if (cmp > 0)
                {
                    k = 2 * k + 1;
                }
                else
                {

                    return k - 1;
                }
            }
        }
        // there is no exact match
        if (equalityOnly)
            return -1;
        k >>= __builtin_ffs(~k);

        return k - 1;
    }

    bool lowerBoundBranchless(u32 keyHead, u8 *key, unsigned keyLength, unsigned k)
    {
        if (slot[k].head > keyHead)
        {
            return true;
        }

        else if ((slot[k].head < keyHead))
        {
            return false;
        }

        else if (slot[k].remainderLen == 0)
        {
            if (keyLength < slot[k].headLen)
            {
                return false;
            }
            else if (keyLength > slot[k].headLen)
            {
                return true;
            }
        }
        else
        {
            int cmp;
            if (isLarge(k))
            {
                cmp = cmpKeys(key, getRemainderLarge(k), keyLength, getRestLenLarge(k));
            }
            else
            {
                cmp = cmpKeys(key, getRest(k), keyLength, getRemainderLength(k));
            }

            if (cmp < 0)
            {
                return true;
            }
            else if (cmp > 0)
            {
                return false;
            }
            
        }
        return false;
    }
    template <bool equalityOnly = false>
    unsigned lowerBoundEytzinger(u8 *key, unsigned keyLength)
    {
        if (equalityOnly)
        {
            if ((keyLength < prefix_len) || (bcmp(key, getLowerFenceKey(), prefix_len) != 0))
                return -1;
        }
        else
        {
            int prefixCmp = cmpKeys(key, getLowerFenceKey(), min<unsigned>(keyLength, prefix_len), prefix_len);
            if (prefixCmp < 0)
            {
                return findSmallestChildEyt(count);
            }

            else if (prefixCmp > 0)
            {
                return findSmallestChildEyt(count) - 1; //defacto biggest
            }
        }
        key += prefix_len;
        keyLength -= prefix_len;

        unsigned oldKeyLength = keyLength;
        u32 keyHead = extractKeyHead(key, keyLength);

        size_t k = 0;
        // full binary search
        while (k < count)
        {
            // __builtin_prefetch(slot + k * (64/sizeof(PageSlot)));
            // __builtin_prefetch(slot + k * (64/sizeof(PageSlot)) + 64);

            if (slot[k].head > keyHead)
            {
                k = 2 * k + 1;
            }

            else if ((slot[k].head < keyHead))
            {
                k = 2 * k + 2;
            }

            else if (slot[k].remainderLen == 0)
            {
                if (oldKeyLength < slot[k].headLen)
                {
                    k = 2 * k + 1;
                }
                else if (oldKeyLength > slot[k].headLen)
                {
                    k = 2 * k + 2;
                }
                else
                {
                    return k;
                }
            }
            else
            {
                int cmp;
                if (isLarge(k))
                {
                    // __builtin_prefetch(ptr() + slot[k].offset + sizeof(SwipType) + sizeof(u16));

                    cmp = cmpKeys(key, getRemainderLarge(k), keyLength, getRestLenLarge(k));
                }
                else
                {
                    // __builtin_prefetch(ptr() + slot[k].offset + sizeof(SwipType));

                    cmp = cmpKeys(key, getRest(k), keyLength, getRemainderLength(k));
                }
                // cout << "[DEBUG]: large branch " << cmp << " and " << k << " and " << arr_tmp[k].offset << endl;

                if (cmp < 0)
                {
                    k = 2 * k + 1;
                }
                else if (cmp > 0)
                {
                    k = 2 * k + 2;
                }
                else
                {

                    return k;
                }
            }
        }
        // there is no exact match
        if (equalityOnly)
            return -1;
        auto j = (k + 1) >> __builtin_ffs(~(k + 1));

        return j == 0 ? count : j - 1;
    }

    template <bool equalityOnly = false>
    unsigned lowerBoundEytzinger_branchless(u8 *key, unsigned keyLength)
    {

        if (equalityOnly)
        {
            if ((keyLength < prefix_len) || (bcmp(key, getLowerFenceKey(), prefix_len) != 0))
                return -1;
        }
        else
        {
            int prefixCmp = cmpKeys(key, getLowerFenceKey(), min<unsigned>(keyLength, prefix_len), prefix_len);
            if (prefixCmp < 0)
            {
                return 0;
            }

            else if (prefixCmp > 0)
            {
                return count;
            }
        }
        key += prefix_len;
        keyLength -= prefix_len;
        u32 keyHead = extractKeyHead(key, keyLength);

        size_t k = 0;
        // full binary search
        while (k < count)
        {
            __builtin_prefetch(slot + k * 16);
            k = lowerBoundBranchless(keyHead, key, keyLength, k) ? 2 * k + 1 : 2 * k + 2;
        }
        // there is no exact match
        if (equalityOnly)
            return -1;
        auto j = (k + 1) >> __builtin_ffs(~(k + 1));

        return j == 0 ? count : j - 1;
    }

    template <bool equalityOnly = false>
    unsigned lowerBound(u8 *key, unsigned keyLength)
    {
        if (equalityOnly)
        {
            if ((keyLength < prefix_len) || (bcmp(key, getLowerFenceKey(), prefix_len) != 0))
            {
                return -1;
            }
        }
        else
        {
            int prefixCmp = cmpKeys(key, getLowerFenceKey(), min<unsigned>(keyLength, prefix_len), prefix_len);
            if (prefixCmp < 0)
            {

                return 0;
            }
            else if (prefixCmp > 0)
                return count;
        }
        key += prefix_len;
        keyLength -= prefix_len;

        unsigned lower = 0;
        unsigned upper = count;

        unsigned oldKeyLength = keyLength;
        u32 keyHead = extractKeyHead(key, keyLength);

        // searching with hints
        if (count > hintCount * 2)
        {
            unsigned dist = count / (hintCount + 1);
            unsigned pos;
            for (pos = 0; pos < hintCount; pos++)
                if (hint[pos] >= keyHead)
                    break;
            lower = pos * dist;
            unsigned pos2;
            for (pos2 = pos; pos2 < hintCount; pos2++)
                if (hint[pos2] != keyHead)
                    break;
            if (pos2 < hintCount)
                upper = (pos2 + 1) * dist;
        }

        // full binary search
        while (lower < upper)
        {
            unsigned mid = ((upper - lower) / 2) + lower;
            if (keyHead < slot[mid].head)
            {
                upper = mid;
            }
            else if (keyHead > slot[mid].head)
            {
                lower = mid + 1;
            }
            else if (slot[mid].remainderLen == 0)
            {
                if (oldKeyLength < slot[mid].headLen)
                {
                    upper = mid;
                }
                else if (oldKeyLength > slot[mid].headLen)
                {
                    lower = mid + 1;
                }
                else
                {
                    return mid;
                }
            }
            else
            {
                int cmp;
                if (isLarge(mid))
                {
                    cmp = cmpKeys(key, getRemainderLarge(mid), keyLength, getRestLenLarge(mid));
                }
                else
                {
                    cmp = cmpKeys(key, getRest(mid), keyLength, getRemainderLength(mid));
                }
                if (cmp < 0)
                {
                    upper = mid;
                }
                else if (cmp > 0)
                {
                    lower = mid + 1;
                }
                else
                {
                    return mid;
                }
            }
        }
        // there is no exact match
        if (equalityOnly)
            return -1;
        return lower;
    }

    void updateHints(unsigned slot_id)
    {
        unsigned dist = count / (hintCount + 1);
        unsigned begin = 0;
        if ((count > hintCount * 2 + 1) && (((count - 1) / (hintCount + 1)) == dist) && ((slot_id / dist) > 1))
            begin = (slot_id / dist) - 1;
        for (unsigned i = begin; i < hintCount; i++)
            hint[i] = slot[dist * (i + 1)].head;
        for (unsigned i = 0; i < hintCount; i++)
            assert(hint[i] == slot[dist * (i + 1)].head);
    }
    bool insert(u8 *key, unsigned keyLength, SwipType value, u8 *payload = nullptr)
    { 
        if (!is_leaf && is_eyt)
        {
            convertFromEytzinger(slot, count, slot[count]);
            assert(isSorted(slot, count));
        }
        const u16 space_needed = (is_leaf) ? u64(value) + spaceNeeded(keyLength, prefix_len) : spaceNeeded(keyLength, prefix_len);
        if (!allocateSpace(space_needed))
        {
            if (!is_leaf && !is_eyt)
                convertToEytzinger(slot, count);
            return false; // not enough space insert fails
        }
        unsigned slot_id = lowerBound<false>(key, keyLength);
        memmove(slot + slot_id + 1, slot + slot_id, sizeof(PageSlot) * (count - slot_id)); // move the bigger fences one slot higher
        storePayload(slot_id, key, keyLength, value, payload); // store the key value pair
        count++;
        updateHints(slot_id);
        assert(lowerBound<true>(key, keyLength) == slot_id); // duplicate check
        if (!is_leaf && !is_eyt)
        {
            convertToEytzinger(slot, count);
        }
        return true;
    }

    bool removeSlot(unsigned slot_id)
    {
        if (slot[slot_id].remainderLen)
            space_used -= sizeof(SwipType) + (isLarge(slot_id) ? (getRestLenLarge(slot_id) + sizeof(u16)) : slot[slot_id].remainderLen);
        space_used -= getPayloadLength(slot_id);
        memmove(slot + slot_id, slot + slot_id + 1, sizeof(PageSlot) * (count - slot_id - 1));
        count--;
        makeHint();
        return true;
    }

    bool remove(u8 *key, unsigned keyLength)
    {
        if (!is_leaf && is_eyt)
        {
            convertFromEytzinger(slot, count, slot[count]);
        }
        int slot_id = lowerBound<true>(key, keyLength);
        bool ret;
        if (slot_id == -1)
            ret = false;
        else
            ret = removeSlot(slot_id);
        return ret;
    }

    bool remove(unsigned slot_id)
    {
        if (!is_leaf && is_eyt)
        {
            convertFromEytzinger(slot, count, slot[count]);
        }
        bool ret;
        if (slot_id == static_cast<unsigned>(-1))
            ret = false;
        else
            ret = removeSlot(slot_id);

        return ret;
    }

    void compact()
    {
        unsigned should = spacePostCompact();
        static_cast<void>(should);
        BTreeNode tmp(is_leaf);
        tmp.setFences(getLowerFenceKey(), lower_fence.length, getUpperFenceKey(), upper_fence.length);
        copyKeyValueRange(&tmp, 0, 0, count);
        tmp.upper = upper;
        memcpy(reinterpret_cast<char *>(this), &tmp, sizeof(BTreeNode));
        makeHint();
    }

    unsigned calculateMergeSpace(const BTreeNode &tempNode, const BTreeNode *right, unsigned slot_id = 0, BTreeNode *parent = nullptr)
    {
        unsigned leftGrow = (prefix_len - tempNode.prefix_len) * count;
        unsigned rightGrow = (right->prefix_len - tempNode.prefix_len) * right->count;
        unsigned innerGrow = 0;
        if (parent)
        {
            unsigned extraKeyLength = parent->getFullKeyLength(slot_id);
            innerGrow = spaceNeeded(extraKeyLength, tempNode.prefix_len);
        }

        return space_used + right->space_used + (reinterpret_cast<u8 *>(slot + count + right->count) - ptr()) + leftGrow + rightGrow + innerGrow;
    }

    void performLeafNodeMerge(BTreeNode *tempNode, BTreeNode *right, BTreeNode *parent, unsigned slot_id)
    {
        copyKeyValueRange(tempNode, 0, 0, count);
        right->copyKeyValueRange(tempNode, count, 0, right->count);
        parent->remove(slot_id);
        memcpy(reinterpret_cast<u8 *>(right), tempNode, sizeof(BTreeNode));
        right->makeHint();
    }

    bool mergeLeafNodes(unsigned slot_id, BTreeNode *parent, BTreeNode *right)
    {
        assert(right->is_leaf);
        BTreeNode tempNode(is_leaf);
        tempNode.setFences(getLowerFenceKey(), lower_fence.length, right->getUpperFenceKey(), right->upper_fence.length);

        // Calculate space requirement
        unsigned spaceNeeded = calculateMergeSpace(tempNode, right);
        if (spaceNeeded > PAGE_SIZE)
            return false;

        // Perform merging
        performLeafNodeMerge(&tempNode, right, parent, slot_id);
        return true;
    }

    void performInnerNodeMerge(BTreeNode *tempNode, BTreeNode *right, BTreeNode *parent, unsigned slot_id)
    {
        copyKeyValueRange(tempNode, 0, 0, count);
        unsigned extraKeyLength = parent->getFullKeyLength(slot_id);
        u8 extraKey[extraKeyLength];
        parent->copyKeyOut(slot_id, extraKey, extraKeyLength);
        storePayload(count, extraKey, extraKeyLength, parent->getChild(slot_id));
        count++;
        right->copyKeyValueRange(tempNode, count, 0, right->count);
        parent->removeSlot(slot_id);
        memcpy(reinterpret_cast<u8 *>(right), tempNode, sizeof(BTreeNode));
    }

    bool mergeInnerNodes(unsigned slot_id, BTreeNode *parent, BTreeNode *right)
    {
        assert(!right->is_leaf);
        BTreeNode tempNode(is_leaf);
        tempNode.setFences(getLowerFenceKey(), lower_fence.length, right->getUpperFenceKey(), right->upper_fence.length);

        // Calculate space requirement
        unsigned spaceNeeded = calculateMergeSpace(tempNode, right, slot_id, parent);

        if (spaceNeeded > PAGE_SIZE)
            return false;

        // Perform merging
        performInnerNodeMerge(&tempNode, right, parent, slot_id);
        return true;
    }

    // merge is not really necesarry and this does not work 100% of the time
    bool merge(unsigned slot_id, BTreeNode *parent, BTreeNode *right)
    {
        
        bool ret;
        if (is_leaf)
        {
            ret = mergeLeafNodes(slot_id, parent, right);
        }
        else
        {
            ret = mergeInnerNodes(slot_id, parent, right);
        }
        return ret;
    }

    bool allocateSpaceForKeyValue(unsigned slot_id, unsigned keyLength, SwipType value, bool isLeaf)
    {
        unsigned spaceNeeded = keyLength + sizeof(SwipType) + ((keyLength > limit) ? sizeof(u16) : 0) + (isLeaf ? u64(value) : 0);
        free_offset -= spaceNeeded;
        space_used += spaceNeeded;
        slot[slot_id].offset = free_offset;
        getChild(slot_id) = value;
        return spaceNeeded <= PAGE_SIZE;
    }

    void storeLargeKeyValue(unsigned slot_id, u8 *key, unsigned keyLength, u8 *payload)
    {
        setLarge(slot_id);
        getRestLenLarge(slot_id) = keyLength;
        memcpy(getRemainderLarge(slot_id), key, keyLength);
        if (is_leaf)
        {
            assert(payload != nullptr);
            memcpy(getPayloadLarge(slot_id), payload, getPayloadLength(slot_id));
        }
    }

    void storeSmallKeyValue(unsigned slot_id, u8 *key, unsigned keyLength, u8 *payload)
    {
        slot[slot_id].remainderLen = keyLength;
        memcpy(getRest(slot_id), key, keyLength);
        if (is_leaf)
        {
            assert(payload != nullptr);
            memcpy(getPayload(slot_id), payload, getPayloadLength(slot_id));
        }
    }

    void storePayload(unsigned slot_id, u8 *key, unsigned keyLength, SwipType value, u8 *payload = nullptr)
    {

        key += prefix_len;
        keyLength -= prefix_len;

        slot[slot_id].headLen = (keyLength >= sizeof(u32)) ? sizeof(u32) : keyLength;
        slot[slot_id].head = extractKeyHead(key, keyLength);

        if (!allocateSpaceForKeyValue(slot_id, keyLength, value, is_leaf))
        {
            throw std::invalid_argument("THERE IS NOT ENOGUH SPACE!");
        }

        if (keyLength > limit)
        {
            storeLargeKeyValue(slot_id, key, keyLength, payload);
        }
        else
        {
            storeSmallKeyValue(slot_id, key, keyLength, payload);
        }
    }

    unsigned
    calculateSlotSpace(BTreeNode *node, unsigned slotIndex)
    {
        unsigned space = sizeof(SwipType) + (node->isLarge(slotIndex) ? (node->getRestLenLarge(slotIndex) + sizeof(u16)) : node->getRemainderLength(slotIndex));
        if (node->is_leaf)
            space += node->getPayloadLength(slotIndex);
        return space;
    }

    void copySlotData(BTreeNode *dst, BTreeNode *src, unsigned dstSlotIndex, unsigned srcSlotIndex)
    {
        unsigned space = calculateSlotSpace(src, srcSlotIndex);

        dst->free_offset -= space;

        dst->space_used += space;

        dst->slot[dstSlotIndex].offset = dst->free_offset;
        memcpy(reinterpret_cast<u8 *>(dst) + dst->free_offset, src->ptr() + src->slot[srcSlotIndex].offset, space);
    }

    void copyKeyValueRange(BTreeNode *dst, unsigned dstSlot, unsigned srcSlot, unsigned count)
    {
        if (!is_leaf && is_eyt)
            throw std::invalid_argument("CopyValueRange fails");
        if (prefix_len == dst->prefix_len)
        {
            memcpy(dst->slot + dstSlot, slot + srcSlot, sizeof(PageSlot) * count);
            for (unsigned i = 0; i < count; i++)
            {
                copySlotData(dst, this, dstSlot + i, srcSlot + i);
            }
        }
        else
        {
            for (unsigned i = 0; i < count; i++)
            {
                copyKeyValue(srcSlot + i, dst, dstSlot + i);
            }
        }
        dst->count += count;
        assert((ptr() + dst->free_offset) >= reinterpret_cast<u8 *>(slot + count));
    }

    void copyKeyValue(u16 srcSlot, BTreeNode *dst, u16 dstSlot)
    {
        unsigned fullLength = getFullKeyLength(srcSlot);
        u8 key[fullLength];
        copyKeyOut(srcSlot, key, fullLength);
        dst->storePayload(dstSlot, key, fullLength, getChild(srcSlot), (isLarge(srcSlot) ? getPayloadLarge(srcSlot) : getPayload(srcSlot)));
    }
    void insertFence(FenceKey &fk, u8 *key, unsigned keyLength)
    {
        if (!key)
            return;
        assert(freeSpace() >= keyLength);
        free_offset -= keyLength;
        space_used += keyLength;
        fk.offset = free_offset;
        fk.length = keyLength;
        memcpy(ptr() + free_offset, key, keyLength);
    }

    void setFences(u8 *lowerKey, unsigned lowerLen, u8 *upperKey, unsigned upperLen)
    {
        insertFence(lower_fence, lowerKey, lowerLen);
        insertFence(upper_fence, upperKey, upperLen);
        for (prefix_len = 0; (prefix_len < min(lowerLen, upperLen)) && (lowerKey[prefix_len] == upperKey[prefix_len]); prefix_len++)
            ;
    }

    BTreeNode *createNewNode(bool isLeaf, u8 *lowerKey, unsigned lowerLength, u8 *upperKey, unsigned upperLength)
    {
        BTreeNode *newNode = new BTreeNode(isLeaf);
        newNode->setFences(lowerKey, lowerLength, upperKey, upperLength);
        return newNode;
    }
    void performCopyAndUpdate(BTreeNode *leftNode, BTreeNode *rightNode, unsigned sepSlot, bool isLeaf)
    {
        if (isLeaf)
        {
            copyKeyValueRange(leftNode, 0, 0, sepSlot + 1);

            copyKeyValueRange(rightNode, 0, leftNode->count, count - (sepSlot + 1));

        }
        else
        {
            copyKeyValueRange(leftNode, 0, 0, sepSlot);
            copyKeyValueRange(rightNode, 0, leftNode->count + 1, count - leftNode->count - 1);
            leftNode->upper = getChild(leftNode->count);
            rightNode->upper = upper;
        }
        leftNode->makeHint();
        rightNode->makeHint();
    }

    void split(BTreeNode *parent, unsigned sepSlot, u8 *sepKey, unsigned sepLength)
    {
        assert(sepSlot < (BTreeNodeHeader::PAGE_SIZE / sizeof(SwipType)));
        if (!is_leaf && is_eyt)
            convertFromEytzinger(slot, count, slot[count]);

        BTreeNode *nodeLeft = createNewNode(is_leaf, getLowerFenceKey(), lower_fence.length, sepKey, sepLength);
        BTreeNode tmp(is_leaf);
        BTreeNode *nodeRight = &tmp;
        nodeRight->setFences(sepKey, sepLength, getUpperFenceKey(), upper_fence.length);

        bool success = parent->insert(sepKey, sepLength, nodeLeft);

        assert(success);
        static_cast<void>(success); //for -DNDEBUG -WUnused
        performCopyAndUpdate(nodeLeft, nodeRight, sepSlot, is_leaf);

        memcpy(reinterpret_cast<char *>(this), nodeRight, sizeof(BTreeNode));
    }

    struct SeparatorInfo
    {
        unsigned length;
        unsigned slot;
        bool trunc;
    };

    // generic prefix finder
    unsigned commonPrefix(unsigned posA, unsigned posB)
    {

        if ((slot[posA].head == slot[posB].head) && (slot[posA].headLen == slot[posB].headLen))
        {
            unsigned lenA, lenB;
            u8 *ptrA, *ptrB;
            if (isLarge(posA))
            {
                ptrA = getRemainderLarge(posA);
                lenA = getRestLenLarge(posA);
            }
            else
            {
                ptrA = getRest(posA);
                lenA = getRemainderLength(posA);
            }
            if (isLarge(posB))
            {
                ptrB = getRemainderLarge(posB);
                lenB = getRestLenLarge(posB);
            }
            else
            {
                ptrB = getRest(posB);
                lenB = getRemainderLength(posB);
            }
            unsigned i;
            for (i = 0; i < min(lenA, lenB); i++)
                if (ptrA[i] != ptrB[i])
                    break;
            return i + slot[posA].headLen;
        }
        unsigned limit = min(slot[posA].headLen, slot[posB].headLen);
        unsigned i;
        for (i = 0; i < limit; i++)
            if (slot[posA].headBytes[3 - i] != slot[posB].headBytes[3 - i])
                return i;
        return i;
    }

    SeparatorInfo findSep()
    {
        if (isInner())
            return SeparatorInfo{getFullKeyLength(count / 2), static_cast<unsigned>(count / 2), false};

        unsigned lower = count / 2 - count / 16;
        unsigned upper = count / 2 + count / 16;
        assert(upper < count);
        unsigned maxPos = count / 2;
        int maxPrefix = commonPrefix(maxPos, 0);
        for (unsigned i = lower; i < upper; i++)
        {
            int prefix = commonPrefix(i, 0);
            if (prefix > maxPrefix)
            {
                maxPrefix = prefix;
                maxPos = i;
            }
        }
        unsigned common = commonPrefix(maxPos, maxPos + 1);
        if ((common > sizeof(u32)) && (getFullKeyLength(maxPos) - prefix_len > common) && (getFullKeyLength(maxPos + 1) - prefix_len > common + 2))
        {
            return SeparatorInfo{static_cast<unsigned>(prefix_len + common + 1), maxPos, true};
        }
        return SeparatorInfo{getFullKeyLength(maxPos), maxPos, false};
    }

    void getSep(u8 *sepKeyOut, SeparatorInfo info)
    {
        copyKeyOut(info.slot, sepKeyOut, info.length);
        if (info.trunc)
        {
            u8 *k = isLarge(info.slot + 1) ? getRemainderLarge(info.slot + 1) : getRest(info.slot + 1);
            sepKeyOut[info.length - 1] = k[info.length - prefix_len - sizeof(u32) - 1];
        }
    }

    BTreeNode *lookupInner(u8 *key, unsigned keyLength)
    {
        if (!is_leaf && !is_eyt)
            convertToEytzinger(slot, count);
        unsigned pos2 = lowerBoundEytzinger<false>(key, keyLength);
        BTreeNode *y;
        if (pos2 >= count)
            y = upper;
        else
            y = getChild(pos2);
        return y;
    }

    unsigned lookupInnerPos(u8 *key, unsigned keyLength)
    {
        if (!is_leaf && !is_eyt)
        {
            convertToEytzinger(slot, count);
        }
        unsigned pos = lowerBoundEytzinger<false>(key, keyLength);

        if (pos >= count)
        {
            pos = count;
        }
        return pos;
    }
    std::string string_to_hex(const std::string &input)
    {
        static const char hex_digits[] = "0123456789ABCDEF";

        std::string output;
        output.reserve(input.length() * 2);
        for (unsigned char c : input)
        {
            output.push_back(hex_digits[c >> 4]);
            output.push_back(hex_digits[c & 15]);
        }
        return output;
    }
    /**
     * @brief deletes every node in order
     * @todo solve double free problem
     */
    void destroy()
    {
        if (isInner())
        {
            for (unsigned i = 0; i < count; i++)
                getChild(i)->destroy();
            upper->destroy();
        }
        delete this;
        return;
    }

    

    void print()
    {
        return;
        if (isInner())
        {
            EytzingerIterator it(count);
            for (unsigned i = it.hasNext() ? it.next() : count;; i = it.next())
            {
                getChild(i)->print();
                if (!it.hasNext())
                    break;
            }
            upper->print();
        }
        else
        {
            for (unsigned i = 0; i < count; i++)
            {
                u8 keyout[256];
                copyKeyOut(i, keyout, getFullKeyLength(i));
                string str(reinterpret_cast<const char *>(keyout), getFullKeyLength(i));
                cout << counter++ << ": " << keyout << endl;
            }
        }
    }

    void print(uint8_t *key, unsigned keyLength, uint8_t *keyOut,
               const std::function<bool(unsigned int, uint8_t *, unsigned int)>
                   &found_callback)
    {
        auto root = this;
        auto node = this;
        auto parent = this;

        auto index = 0;
        while (node->isInner())
        {
            root = parent;
            parent = node;

            node = node->lookupInner(key, keyLength);
        }
        index = parent->lookupInnerPos(key, keyLength);
        parent->getChild(index)->print();
        parent->print2(index, key, keyLength, keyOut, found_callback);
        static_cast<void>(root);

        // auto index2 = root->lookupInnerPos(key, keyLength);
        // // root->getChild(index2)->print();
        // // parent->print();
        // // root->upper->getChild(25)->print();
        // // root->getChild(25)->print();
        // bool one_time = true;
        // while (root != nullptr)
        // {
        //     EytzingerIterator it(root->count);
        //     int x = -1;
        //     while (x != index2 && it.hasNext())
        //     {
        //         x = it.next();
        //     }
        //     bool continueloop = true;
        //     size_t i = it.hasNext() ? (one_time ? it.next() : x) : root->count;

        //     for (; i < root->count && continueloop && cont; i = it.next())
        //     {

        //         // root->getChild(i)->getChild(findSmallestChildEyt(root->getChild(i)->count))->print();
        //         // cout << "4++++++++++++++++++++++++++++++++++++++++++" << endl;
        //         // cout << "THJIS IS A LEAF??????????" << getChild(i)->is_eyt << " and is leaf? : " << root->getChild(i)->is_leaf << endl;
        //         // cout << "findSmallestChildEyt(getChild(i)->count)" << findSmallestChildEyt(root->count) << " and count " << root->count << endl;

        //         if (!root->getChild(i)->is_eyt)
        //         {

        //             root->getChild(i)->print2(0, key, keyLength, keyOut, found_callback);
        //         }
        //         else
        //         {

        //             root->getChild(i)->print2(findSmallestChildEyt(root->getChild(i)->count), key, keyLength, keyOut, found_callback);
        //         }
        //         if (!it.hasNext())
        //             break;
        //     }
        //     root = root->upper;
        //     if (root == nullptr)
        //         break;
        //     int asd = root->count;
        //     index2 = root->lookupInnerPos(key, keyLength);
        //     // root->getChild(index2)->print();
        //     one_time = false;
        // }
        // if (cont && tmp && parent->upper != nullptr)
        // {
        //     cout << "we are here" << endl;
        //     cout << "root hasLP: " << root->count << ", and this count: " << index << endl;
        //     cout << "++++++++++++++++++++++++++++++++++++++++++" << endl;

        //     parent->upper->print();

        //     cout << "++++++++++++++++++++++++++++++++++++++++++" << endl;

        //     root->print();
        //     cout << "++++++++++++++++++++++++++++++++++++++++++" << endl;

        //     parent->upper->print2(findSmallestChildEyt(upper->count), key, keyLength, keyOut, found_callback);
        // }
        // auto index2 = root->lookupInnerPos(key,keyLength);
        // cout << "THe root index : " << index2 << " and the total are: " << root->count << endl;
        // if(index2< root->count && cont){
        //     cout << "Hi"  << endl;
        //     root->print2(index2 +1,key, keyLength, keyOut, found_callback );
        // }

        // if(root->upper != NULL && cont ){
        //     cout << "last one" << endl;
        //     root->upper->print2(0,key, keyLength, keyOut, found_callback );
        // }
    }

    class EytzingerIterator
    {
    private:
        void pushLeft(int index)
        {
            while (index < size)
            {
                stack.push(index);
                index = 2 * index + 1; // Move to left child
            }
        }

    public:
        std::stack<int> stack;
        int size;
        EytzingerIterator(int size) : size(size)
        {
            pushLeft(0); // Initialize stack with leftmost path from root
        }

        bool hasNext()
        {
            return !stack.empty();
        }

        int next()
        {
            if (!hasNext())
            {
                throw std::out_of_range("No more elements in the iterator");
            }

            int currentIndex = stack.top();
            stack.pop();

            // If there is a right child, push all the way down its leftmost path
            int rightChild = 2 * currentIndex + 2;
            if (rightChild < size)
            {
                pushLeft(rightChild);
            }

            return currentIndex;
        }

        void findIndex(int index)
        {
            bool cont = true;
            while (cont)
            {
                if (stack.top() == index)
                {
                    stack.pop();
                    return;
                }
                stack.pop();
            }
        }
    };
    unsigned findSmallestChildEyt(unsigned size)
    {
        if (size <= 1)
        {
            return 0;
        }
        unsigned int p = 1;

        // Keep shifting p to the right until we find a power of 2 that is less than n
        while (p < size)
        {
            p <<= 1;
        }
        p >>= 1;
        return p - 1;
    }

    bool print2(int index, uint8_t *key, unsigned keyLength, uint8_t *keyOut,
                const std::function<bool(unsigned int, uint8_t *, unsigned int)>
                    &found_callback)
    {
        if (isInner() && cont)
        {
            if (!is_leaf && !is_eyt)
            {
                // convertFromEytzinger(slot, count, slot[count]);
                cout << "is not eyt" << endl;
            }
            // cout << "the index is: " << index << ", count: " << count << endl;
            EytzingerIterator it(count);
            int x = -1;
            while (x != index && it.hasNext())
            {
                x = it.next();
            }
            unsigned i;

            bool continueloop = true;
            for (i = index; i < count && continueloop && cont; i = it.next())
            {
                getChild(i)->print2(0, key, keyLength, keyOut, found_callback);

                // cout << "i: " << i << endl;
                if (!it.hasNext())
                    break;
            }
            // cout << "we called this: " << findSmallestChildEyt(upper->count) << ", " << upper->count << endl;
            // cout << "Upper is leaf?: " << upper->is_leaf << endl;
            // upper->print();
            upper->print2(findSmallestChildEyt(upper->count), key, keyLength, keyOut, found_callback); // must find the smallest in the eytzinger layout
            // cout << "we returned to this" << endl;
        }
        else if (cont)
        {

            int pos = lowerBound<false>(key, keyLength);
            // cout << "The key is : " << key << endl;
            // cout << "The length is: " << keyLength << endl;
            // cout << "Prefix: " << prefix_len << endl;
            // cout << "http://www.bbc.co.uk/programmes/b00wfxh3.rdf " << lowerBound<true>(reinterpret_cast<u8 *>(bbc), 40) << endl;

            // cout << "http://www.bbc.co.uk/programmes/b00wfxh3.rdf " << lowerBound<false>(reinterpret_cast<u8 *>(bbc), 40) << endl;
            // cout << "The key is : " << key << endl;
            // cout << "The length is: " << keyLength << endl;
            // cout << "count is " << count << endl;
            if (pos == -1)
                pos = 0;
            for (unsigned i = pos; i < count; i++)
            {
                u8 keyout[getFullKeyLength(i)]; // Adjust size as needed.
                copyKeyOut(i, keyout, getFullKeyLength(i));

                // // If the key is less than the input key, skip this iteration.
                // if (cmpKeys(a.data(), keyout, a.size(), getFullKeyLength(i)) > 0)
                // {
                //     continue;
                // }
                // cout << "We are now doint the " << i << "th iteration out of " << count << " iterationms and is eyt + leaf" << is_eyt << " " << is_leaf << endl;
                // Processing the key and payload.
                auto fullKeyLength = getFullKeyLength(i);
                copyKeyOut(i, keyOut, fullKeyLength);
                auto payload = (isLarge(i)) ? getPayloadLarge(i) : getPayload(i);
                auto payloadLength = getPayloadLength(i);
                string str(reinterpret_cast<const char *>(keyout), getFullKeyLength(i));
                // cout << counter++ << ": " << string_to_hex(str) << endl;
                // Here you would place your processing logic. For example, if you were previously printing
                // the keys, you might instead call a function to process them as needed.
                bool shouldContinue = found_callback(fullKeyLength, payload, payloadLength);
                // cout << "The shouldcontinue is: " << shouldContinue << endl;
                // If the callback indicates not to continue, set cont to false and break out of the loop.
                if (!shouldContinue)
                {

                    cont = false;
                    return false;
                }
            }
        }
        // else if (cont && upper != nullptr)
        // {
        //     upper->print2(findSmallestChildEyt(upper->count), key, keyLength, keyOut, found_callback); // must find the smallest in the eytzinger layout
        // }
        return true;
    }
};

struct BTree
{
    BTreeNode *root;
    BTree();
    bool lookup(u8 *key, unsigned keyLength, u64 &payloadLength, u8 *result);
    void lookupInner(u8 *key, unsigned keyLength);
    void splitNode(BTreeNode *node, BTreeNode *parent, u8 *key, unsigned keyLength);
    void splitInner(BTreeNode *toSplit, u8 *key, unsigned keyLength);
    void insert(u8 *key, unsigned keyLength, u64 payloadLength, u8 *payload = nullptr);
    bool remove(u8 *key, unsigned keyLength);
    u64 getPayloadLenLookup(u8 *key, unsigned keyLength);
    bool merge_help(u8 *, unsigned, BTreeNode *);
    void makeAllEyt(BTreeNode *node)
    {
        if (!node->is_leaf)
        {
            if (node->is_eyt)
            {
                // for (size_t i = 0; i < node->slotnum; i++)
                // {
                //     cout << "[head: " << node->slot[i].head << ", offset: " << node->slot[i].offset << "], ";
                // }
                node->convertFromEytzinger(node->slot, node->count, node->slot[node->count]);
            }
            // cout << endl;
            // cout << endl; //[head: 1677721600, offset: 4088],

            // for (size_t i = 0; i < node->slotnum; i++)
            // {
            //     cout << node->slot[i].head << ", ";
            // }
            // cout << endl;
            // cout << endl;
            // cout << endl;
            // cout << endl;
            for (size_t i = 0; i < node->count; i++)
            {
                makeAllEyt(node->getChild(i));
            }
            node->convertToEytzinger(node->slot, node->count);
        }
    }

    void makeAllSorted(BTreeNode *node)
    {
        if (!node->is_leaf)
        {
            if (node->is_eyt)
            {
                // for (size_t i = 0; i < node->slotnum; i++)
                // {
                //     cout << "[head: " << node->slot[i].head << ", offset: " << node->slot[i].offset << "], ";
                // }
                node->convertFromEytzinger(node->slot, node->count, node->slot[node->count]);
            }
            // cout << endl;
            // cout << endl; //[head: 1677721600, offset: 4088],

            // for (size_t i = 0; i < node->slotnum; i++)
            // {
            //     cout << node->slot[i].head << ", ";
            // }
            // cout << endl;
            // cout << endl;
            // cout << endl;
            // cout << endl;
            for (size_t i = 0; i < node->count; i++)
            {
                makeAllSorted(node->getChild(i));
            }
            // cout <<"DONE" <<endl;
            makeAllSorted(node->upper);
        }
    }

    ~BTree();
};

// create a new tree and return a pointer to it
BTree *btree_create();

// destroy a tree created by btree_create
void btree_destroy(BTree *);

// return true iff the key was present
bool btree_remove(BTree *tree, uint8_t *key, uint16_t keyLength);

// replaces exising record if any
void btree_insert(BTree *tree, uint8_t *key, uint16_t keyLength, uint8_t *value,
                  uint16_t valueLength);

// returns a pointer to the associated value if present, nullptr otherwise
uint8_t *btree_lookup(BTree *tree, uint8_t *key, uint16_t keyLength,
                      uint16_t &payloadLengthOut);

// invokes the callback for all records greater than or equal to key, in order.
// the key should be copied to keyOut before the call.
// the callback should be invoked with keyLength, value pointer, and value
// length iteration stops if there are no more keys or the callback returns
// false.
void btree_scan(BTree *tree, uint8_t *key, unsigned keyLength, uint8_t *keyOut,
                const std::function<bool(unsigned int, uint8_t *, unsigned int)>
                    &found_callback);