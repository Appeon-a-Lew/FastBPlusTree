#ifndef DSE_PRACTICAL_COURSE_BTREE_TEMPLATE_TESTER_HPP
#define DSE_PRACTICAL_COURSE_BTREE_TEMPLATE_TESTER_HPP

#include "btree/btree.hpp"
#include <cassert>
#include <cstring>
#include <map>
#include <vector>

struct Tester {
  BTree *btree;
  std::map<std::vector<uint8_t>, std::vector<uint8_t>> stdMap;

  Tester() : btree(btree_create()), stdMap() {}

  ~Tester() { btree_destroy(btree); }

  void insert(std::vector<uint8_t> &key, std::vector<uint8_t> &value) {
    stdMap[key] = value;
    btree_insert(btree, key.data(), key.size(), value.data(), value.size());
  }

  void lookup(std::vector<uint8_t> &key) {
    auto it = stdMap.find(key);
    uint16_t lenOut = 0;
    uint8_t *value = btree_lookup(btree, key.data(), key.size(), lenOut);
    if (it == stdMap.end()) {
      assert(value == nullptr);
    } else {
      assert(value != nullptr);
      assert(lenOut == it->second.size());
      if (lenOut > 0)
        assert(memcmp(value, it->second.data(), lenOut) == 0);
    }
  }

  void remove(std::vector<uint8_t> &key) {
    auto it = stdMap.find(key);
    bool wasPresent = it != stdMap.end();
    if (wasPresent) {
      stdMap.erase(it);
    }
    bool wasPresentBtree = btree_remove(btree, key.data(), key.size());
    assert(wasPresent == wasPresentBtree);
  }

  void scan(std::vector<uint8_t> &key,
            const std::function<bool(uint16_t, uint8_t *, uint16_t)>
                &found_record_cb) {
    uint8_t keyOut[1 << 10];
    bool shouldContinue = true;
    auto std_iterator = stdMap.lower_bound(key);
    btree_scan(
        btree, key.data(), key.size(), keyOut,
        [&](unsigned keyLen, uint8_t *payload, unsigned payloadLen) {
          assert(shouldContinue);
          assert(std_iterator != stdMap.end());
          assert(std_iterator->first.size() == keyLen);
          if (keyLen)
            assert(memcmp(std_iterator->first.data(), keyOut, keyLen) == 0);
          assert(std_iterator->second.size() == payloadLen);
          if (payloadLen)
            assert(memcmp(std_iterator->second.data(), payload, payloadLen) ==
                   0);
          shouldContinue = found_record_cb(keyLen, payload, payloadLen);
          ++std_iterator;
          return shouldContinue;
        });
    if (shouldContinue) {
      assert(std_iterator == stdMap.end());
    }
  }
};

#endif // DSE_PRACTICAL_COURSE_BTREE_TEMPLATE_TESTER_HPP
