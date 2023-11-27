#include "btree.hpp"
#include <cstring>
#include <map>
#include <vector>

inline std::vector<uint8_t> toByteVector(uint8_t *b, unsigned l) {
  return std::vector<uint8_t>(b, b + l);
}

struct BTree {
  std::map<std::vector<uint8_t>, std::vector<uint8_t>> stdMap;
};

// create a new tree and return a pointer to it
BTree *btree_create() { return new BTree(); };

// destroy a tree created by btree_create
void btree_destroy(BTree *t) { delete t; }

// return true iff the key was present
bool btree_remove(BTree *tree, uint8_t *key, uint16_t keyLength) {
  auto keyVec = toByteVector(key, keyLength);
  auto it = tree->stdMap.find(keyVec);
  bool wasPresent = it != tree->stdMap.end();
  if (wasPresent) {
    tree->stdMap.erase(it);
  }
  return wasPresent;
}

// replaces exising record if any
void btree_insert(BTree *tree, uint8_t *key, uint16_t keyLength, uint8_t *value,
                  uint16_t valueLength) {
  tree->stdMap[toByteVector(key, keyLength)] = toByteVector(value, valueLength);
}

// returns a pointer to the associated value if present, nullptr otherwise
uint8_t *btree_lookup(BTree *tree, uint8_t *key, uint16_t keyLength,
                      uint16_t &payloadLengthOut) {
  static uint8_t EmptyKey;
  auto keyVec = toByteVector(key, keyLength);
  auto it = tree->stdMap.find(keyVec);
  if (it == tree->stdMap.end()) {
    return nullptr;
  } else {
    payloadLengthOut = it->second.size();
    if (payloadLengthOut > 0)
      return it->second.data();
    else
      return &EmptyKey;
  }
}

// invokes the callback for all records greater than or equal to key, in order.
// the key should be copied to keyOut before the call.
// the callback should be invoked with keyLength, value pointer, and value
// length iteration stops if there are no more keys or the callback returns
// false.
void btree_scan(BTree *tree, uint8_t *key, unsigned keyLength, uint8_t *keyOut,
                const std::function<bool(unsigned int, uint8_t *, unsigned int)>
                    &found_callback) {
  auto keyVec = toByteVector(key, keyLength);
  for (auto it = tree->stdMap.lower_bound(keyVec); it != tree->stdMap.end();
       ++it) {
    if (it->first.size() > 0) {
      memcpy(keyOut, it->first.data(), it->first.size());
    }
    if (!found_callback(it->first.size(), it->second.data(),
                        it->second.size())) {
      break;
    }
  }
}
