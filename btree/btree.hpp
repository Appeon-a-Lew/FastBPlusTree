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
#include "common.h"
// #include "eytzinger.h"
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

struct BTreeNodeHeader
{
    static const unsigned PAGE_SIZE = 1024 * 4;
    static const unsigned under_full = PAGE_SIZE * 0.6;
    static constexpr u8 limit = 254;
    static constexpr u8 marker = 255;

    static const unsigned padding = 0;

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
    };
    const static size_t slotnum = (PAGE_SIZE - sizeof(BTreeNodeHeader)) / (sizeof(PageSlot));
    PageSlot slot[slotnum]; // 0 index

    PageSlot slot_eytzinger[slotnum + 1]; // 1 index

    BTreeNode(bool is_leaf)
        : BTreeNodeHeader(is_leaf) {}

    // here  could be some fuckery:
    unsigned freeSpace() { return free_offset - (reinterpret_cast<u8 *>(slot + count) - ptr()); }
    unsigned freeSpaceEyt() { return free_offset - (reinterpret_cast<u8 *>(slot_eytzinger + count) - ptr()); }

    unsigned spacePostCompact() { return PAGE_SIZE - (reinterpret_cast<u8 *>(slot + count) - ptr()) - space_used; }
    unsigned spacePostCompactEyt() { return PAGE_SIZE - (reinterpret_cast<u8 *>(slot_eytzinger + count) - ptr()) - space_used; }
    //============================================================
    bool allocateSpace(unsigned spaceNeeded)
    {
        auto space = freeSpace();
        if (spaceNeeded <= space)
            return true;
        if (spaceNeeded <= spacePostCompact())
        {
            compact();
            return true;
        }
        return false;
    }

    bool allocateSpaceEyt(unsigned spaceNeeded)
    {
        auto space = freeSpaceEyt();
        if (spaceNeeded <= space)
            return true;
        if (spaceNeeded <= spacePostCompactEyt())
        {
            compactEyt();
            return true;
        }
        return false;
    }
    //============================================================

    static BTreeNode *makeLeaf() { return new BTreeNode(true); }
    static BTreeNode *makeInner() { return new BTreeNode(false); }
    //============================================================
    inline u8 *getRest(unsigned slot_id)
    {
        assert(!isLarge(slot_id));
        return ptr() + slot[slot_id].offset + sizeof(SwipType);
    }
    inline u8 *getRestEyt(unsigned slot_id)
    {
        assert(!isLargeEyt(slot_id));
        return ptr() + slot_eytzinger[slot_id].offset + sizeof(SwipType);
    }
    //============================================================F

    inline unsigned getRemainderLength(unsigned slot_id)
    {
        assert(!isLarge(slot_id));
        return slot[slot_id].remainderLen;
    }
    inline unsigned getRemainderLengthEyt(unsigned slot_id)
    {
        assert(!isLargeEyt(slot_id));
        return slot_eytzinger[slot_id].remainderLen;
    }
    //============================================================

    //============================================================
    inline u8 *getPayload(unsigned slot_id)
    {
        assert(!isLarge(slot_id));
        return ptr() + slot[slot_id].offset + sizeof(SwipType) + getRemainderLength(slot_id);
    }
    inline u8 *getPayloadEyt(unsigned slot_id)
    {
        assert(!isLargeEyt(slot_id));
        return ptr() + slot_eytzinger[slot_id].offset + sizeof(SwipType) + getRemainderLengthEyt(slot_id);
    }
    //============================================================

    //============================================================
    inline u8 *getRemainderLarge(unsigned slot_id)
    {
        assert(isLarge(slot_id));
        return ptr() + slot[slot_id].offset + sizeof(SwipType) + sizeof(u16);
    }
    inline u8 *getRemainderLargeEyt(unsigned slot_id)
    {
        assert(isLargeEyt(slot_id));
        return ptr() + slot_eytzinger[slot_id].offset + sizeof(SwipType) + sizeof(u16);
    }
    //============================================================

    //============================================================
    inline u16 &getRestLenLarge(unsigned slot_id)
    {
        assert(isLarge(slot_id));
        return *reinterpret_cast<u16 *>(ptr() + slot[slot_id].offset + sizeof(SwipType));
    }
    inline u16 &getRestLenLargeEyt(unsigned slot_id)
    {
        assert(isLargeEyt(slot_id));
        return *reinterpret_cast<u16 *>(ptr() + slot_eytzinger[slot_id].offset + sizeof(SwipType));
    }
    //============================================================

    //============================================================
    inline bool isLarge(unsigned slot_id) { return slot[slot_id].remainderLen == marker; }
    inline bool isLargeEyt(unsigned slot_id) { return slot_eytzinger[slot_id].remainderLen == marker; }

    inline void setLarge(unsigned slot_id) { slot[slot_id].remainderLen = marker; }
    inline void setLargeEyt(unsigned slot_id) { slot_eytzinger[slot_id].remainderLen = marker; }
    //============================================================

    //============================================================
    inline u8 *getPayloadLarge(unsigned slot_id)
    {
        assert(isLarge(slot_id));
        return ptr() + slot[slot_id].offset + sizeof(SwipType) + sizeof(u16) + getRestLenLarge(slot_id);
    }
    inline u8 *getPayloadLargeEyt(unsigned slot_id)
    {
        assert(isLargeEyt(slot_id));
        return ptr() + slot_eytzinger[slot_id].offset + sizeof(SwipType) + sizeof(u16) + getRestLenLargeEyt(slot_id);
    }
    //============================================================

    //============================================================
    inline u64 getPayloadLength(unsigned slot_id) { return *reinterpret_cast<u64 *>(ptr() + slot[slot_id].offset); }
    inline u64 getPayloadLengthEyt(unsigned slot_id) { return *reinterpret_cast<u64 *>(ptr() + slot_eytzinger[slot_id].offset); }

    inline SwipType &getChild(unsigned slot_id) { return *reinterpret_cast<SwipType *>(ptr() + slot[slot_id].offset); }
    inline SwipType &getChildEyt(unsigned slot_id) { return *reinterpret_cast<SwipType *>(ptr() + slot_eytzinger[slot_id].offset); }

    inline unsigned getFullKeyLength(unsigned slot_id) { return prefix_len + slot[slot_id].headLen + (isLarge(slot_id) ? getRestLenLarge(slot_id) : getRemainderLength(slot_id)); }
    inline unsigned getFullKeyLengthEyt(unsigned slot_id) { return prefix_len + slot_eytzinger[slot_id].headLen + (isLargeEyt(slot_id) ? getRestLenLargeEyt(slot_id) : getRemainderLengthEyt(slot_id)); }
    //============================================================

    //============================================================

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

    inline void copyKeyOutEyt(unsigned slot_id, u8 *out, unsigned key_len)
    {
        std::copy_n(getLowerFenceKey(), prefix_len, out);
        out += prefix_len;
        key_len -= prefix_len;

        auto &current_slot = slot_eytzinger[slot_id];
        auto headLen = current_slot.headLen;

        if (key_len >= headLen)
        {
            *reinterpret_cast<u32 *>(out) = swap(current_slot.head);
            memcpy(out + headLen, (isLargeEyt(slot_id) ? getRemainderLargeEyt(slot_id) : getRestEyt(slot_id)), key_len - headLen);
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
    //============================================================

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

    // wont be used at the final
    void makeHintEyt()
    {
        unsigned dist = count / (hintCount + 1);
        for (unsigned i = 0; i < hintCount; i++)
            hint[i] = slot_eytzinger[dist * (i + 1)].head;
    }

    //////////////////////////////////////////////////////////////////
    int eytzinger(int i = 0, int k = 1)
    {
        if (k <= count)
        {
            i = eytzinger(i, 2 * k);
            slot_eytzinger[k] = slot[i++];
            i = eytzinger(i, 2 * k + 1);
        }
        return i;
    }
    int search(int x)
    {
        int k = 1;
        while (k <= slotnum)
        {
            if (slot_eytzinger[k].head >= x)
                k = 2 * k;
            else
                k = 2 * k + 1;
        }
        k >>= __builtin_ffs(~k);
        return k;
    }

    template <bool equalityOnly = false>
    unsigned lowerBoundEytzinger(u8 *key, unsigned keyLength)
    {
        eytzinger();
        // std::string s; 
        // for (auto i = 0; i < slotnum; i++)
        // {
        //     // if (slot_eytzinger[i] != NULL)
        //     s.append(to_string(slot_eytzinger[i].head));
        //     s.append(", ");
        // }
        // cout << s << endl;
        // s.clear();
        // cout << "------------" << endl;
        // for (auto i = 0; i < slotnum; i++)
        // {
        //     // if (slot[i] != NULL)
        //     s.append(to_string(slot[i].head));
        //     s.append(", ");
        // }
        // cout << s << endl;

        // key += prefix_len;
        // keyLength -= prefix_len;

        // return search(extractKeyHead(key, keyLength));
        if (equalityOnly)
        {
            if ((keyLength < prefix_len) || (bcmp(key, getLowerFenceKey(), prefix_len) != 0))
                return -1;
        }
        else
        {
            int prefixCmp = cmpKeys(key, getLowerFenceKey(), min<unsigned>(keyLength, prefix_len), prefix_len);
            if (prefixCmp < 0)
                return 0;
            else if (prefixCmp > 0)
                return count;
        }
        key += prefix_len;
        keyLength -= prefix_len;

        unsigned oldKeyLength = keyLength;
        u32 keyHead = extractKeyHead(key, keyLength);

        int k = 1;
        // full binary search
        while (k <= slotnum)
        {
            if (slot_eytzinger[k].head > keyHead)
                k = 2 * k;
            else if ((slot_eytzinger[k].head < keyHead))
                k = 2 * k + 1;

            else if (slot_eytzinger[k].remainderLen == 0)
            {
                cout << "THE oldkeylen: " << oldKeyLength << "  :  " << slot_eytzinger[k].headLen << endl;
                if (oldKeyLength < slot_eytzinger[k].headLen)
                {
                    k = 2 * k;
                }
                else if (oldKeyLength > slot_eytzinger[k].headLen)
                {
                    k = 2 * k + 1;
                }
                else
                {
                    cout << slot_eytzinger[k].head << " and the " << keyHead << endl;
                    k >>= __builtin_ffs(~k);
                    return k;
                }
            }
            else
            {
                int cmp;
                cout << "AAAAAAAAAAAAA " << endl;
                if (isLargeEyt(k))
                {
                    cmp = cmpKeys(key, getRemainderLargeEyt(k), keyLength, getRestLenLargeEyt(k));
                }
                else
                {
                    cmp = cmpKeys(key, getRestEyt(k), keyLength, getRemainderLengthEyt(k));
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
                    k >>= __builtin_ffs(~k);

                    return k;
                }
            }
        }
        // there is no exact match
        if (equalityOnly)
            return -1;
        k >>= __builtin_ffs(~k);

        return k;
    }

    //////////////////////////////////////////////////////////////////
    template <bool equalityOnly = false>
    unsigned lowerBound(u8 *key, unsigned keyLength)
    {
        auto valeyt = lowerBoundEytzinger<equalityOnly>(key, keyLength);
        // std::cout << "We arrived here" <<std::endl;
        auto valbin = lowerBound1<equalityOnly>(key, keyLength);
        // eytzinger();
        std::cout << "Valeyt is : " << valeyt << " : ValBIN: " << valbin << std::endl;
        // std::cout << slot_eytzinger[valeyt].head << " : " << slot[valbin].head << std::endl;
        try
        {
            // std::cout << slot_eytzinger[valeyt].head << " : " << slot[valbin].head << std::endl;
            if (valeyt != 0xffffffff && valbin != 0xffffffff)
                assert(slot_eytzinger[valeyt].head == slot[valbin].head);
        }
        catch (const std::exception &e)
        {
            std::cout << "AAAAAAAAAAAAAAAA" << std::endl;
            std::cerr << slot_eytzinger[valeyt].head << "  and   " << slot[valbin].head << e.what() << '\n';
        }
        return valbin;
    }

    template <bool equalityOnly = false>
    unsigned lowerBound1(u8 *key, unsigned keyLength)
    {

        // if (lower_fence.offset)
        //     assert(cmpKeys(key, getLowerFenceKey(), keyLength, lower_fence.length) > 0);
        // if (upper_fence.offset)
        //     assert(cmpKeys(key, getUpperFenceKey(), keyLength, upper_fence.length) <= 0);

        if (equalityOnly)
        {
            if ((keyLength < prefix_len) || (bcmp(key, getLowerFenceKey(), prefix_len) != 0))
                return -1;
        }
        else
        {
            int prefixCmp = cmpKeys(key, getLowerFenceKey(), min<unsigned>(keyLength, prefix_len), prefix_len);
            if (prefixCmp < 0)
                return 0;
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
    { // TODO: fix this, must be a leaf
        const u16 space_needed = (is_leaf) ? u64(value) + spaceNeeded(keyLength, prefix_len) : spaceNeeded(keyLength, prefix_len);
        if (!allocateSpace(space_needed))
            return false; // not enough space insert fails
        unsigned slot_id = lowerBound<false>(key, keyLength);
        memmove(slot + slot_id + 1, slot + slot_id, sizeof(PageSlot) * (count - slot_id)); // move the bigger fences one slot higher
        storePayload(slot_id, key, keyLength, value, payload);                             // store the key value pair
        count++;
        updateHints(slot_id);
        assert(lowerBound<true>(key, keyLength) == slot_id); // duplicate check
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
        int slot_id = lowerBound<true>(key, keyLength);
        if (slot_id == -1)
            return false;
        return removeSlot(slot_id);
    }
    //============================================================
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
        // assert(freeSpace() == should);
    }

    void compactEyt()
    {
        BTreeNode tmp(is_leaf);
        tmp.setFences(getLowerFenceKey(), lower_fence.length, getUpperFenceKey(), upper_fence.length);
        copyKeyValueRange(&tmp, 0, 0, count);
        tmp.upper = upper;
        memcpy(reinterpret_cast<char *>(this), &tmp, sizeof(BTreeNode));
        makeHint();
    }
    //============================================================
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
        parent->removeSlot(slot_id);
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
        if (is_leaf)
        {
            return mergeLeafNodes(slot_id, parent, right);
        }
        else
        {
            return mergeInnerNodes(slot_id, parent, right);
        }
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
            // Handle the case where there is not enough space
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

    unsigned calculateSlotSpace(BTreeNode *node, unsigned slotIndex)
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
            copyKeyValueRange(rightNode, 0, leftNode->count, count - leftNode->count);
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

        BTreeNode *nodeLeft = createNewNode(is_leaf, getLowerFenceKey(), lower_fence.length, sepKey, sepLength);
        BTreeNode tmp(is_leaf);
        BTreeNode *nodeRight = &tmp;
        nodeRight->setFences(sepKey, sepLength, getUpperFenceKey(), upper_fence.length);

        bool success = parent->insert(sepKey, sepLength, nodeLeft);
        assert(success);

        performCopyAndUpdate(nodeLeft, nodeRight, sepSlot, is_leaf);

        memcpy(reinterpret_cast<char *>(this), nodeRight, sizeof(BTreeNode));
    }

    struct SeparatorInfo
    {
        unsigned length;
        unsigned slot;
        bool trunc;
    };

    // generic prefix finder and truncator
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
        unsigned pos = lowerBound<false>(key, keyLength);
        if (pos == count)
            return upper;

        return getChild(pos);
    }

    unsigned lookupInnerPos(u8 *key, unsigned keyLength)
    {
        unsigned pos = lowerBound<false>(key, keyLength);
        if (pos == count)
            return count;
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
        if (isInner())
        {
            for (unsigned i = 0; i < count; i++)
            {
                getChild(i)->print();
            }
            upper->print();
        }
        else
        {
            for (unsigned i = 0; i < count; i++)
            {
                u8 keyout[128];
                copyKeyOut(i, keyout, getFullKeyLength(i));
                string str(reinterpret_cast<const char *>(keyout), getFullKeyLength(i));
                cout << counter++ << ": " << string_to_hex(str) << endl;
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
        // cout << index << endl;
        while (node->isInner())
        {
            root = parent;
            parent = node;

            node = node->lookupInner(key, keyLength);
        }
        index = parent->lookupInnerPos(key, keyLength);
        // cout << "index is : " << index << endl;
        // cout << "Total indexes are: " << parent->count << endl;
        // if(parent->upper->is_leaf && root == parent) cout<< "There is no more itearions" << endl;
        parent->print2(index, key, keyLength, keyOut, found_callback);

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

    void print2(int index, uint8_t *key, unsigned keyLength, uint8_t *keyOut,
                const std::function<bool(unsigned int, uint8_t *, unsigned int)>
                    &found_callback)
    {
        if (isInner() && cont)
        {

            for (unsigned i = index; i < count; i++)
            {
                getChild(i)->print2(0, key, keyLength, keyOut, found_callback);
            }
            // cout << "we called this" << endl;
            upper->print2(0, key, keyLength, keyOut, found_callback);
            // cout << "we returned to this" << endl;
        }
        else if (cont)
        {
            int pos = lowerBound<true>(key, keyLength);
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
                // cout << "We are now doint the "<< i << "th iteration out of " << count <<" iterationms" << endl;
                // Processing the key and payload.
                auto fullKeyLength = getFullKeyLength(i);
                copyKeyOut(i, keyOut, fullKeyLength);
                auto payload = (isLarge(i)) ? getPayloadLarge(i) : getPayload(i);
                auto payloadLength = getPayloadLength(i) - padding;
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
                    return;
                }
            }
        }
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