/**
 * @file btree.hpp
 * @brief B+ tree implementation
 * @date 2023-11-19
 * @author Ismail Safa Toy
 * inspiration from leanstore
 */

#include "btree.hpp"
BTree::BTree()
    : root(BTreeNode::makeLeaf()) {}
bool BTree::lookup(u8 *key, unsigned keyLength, u64 &payloadLength, u8 *result)
{
   BTreeNode *node = root;
   while (node->isInner())
      node = node->lookupInner(key, keyLength);
   int pos = node->lowerBound<true>(key, keyLength);
   if (pos != -1)
   {
      payloadLength = u64(node->getChild(pos));
      if (node->isLarge(pos))
      {
         memcpy(result, node->getPayloadLarge(pos), node->getPayloadLength(pos));
      }
      else
      {
         memcpy(result, node->getPayload(pos), node->getPayloadLength(pos) - node->padding);
      }
      return true;
   }
   return false;
}
void BTree::insert(u8 *key, unsigned keyLength, u64 payloadLength, u8 *payload)
{
   BTreeNode *node = root;
   BTreeNode *parent = nullptr;
   while (node->isInner())
   {
      parent = node;
      node = node->lookupInner(key, keyLength);
   }
   if (node->insert(key, keyLength, SwipType(payloadLength + (node->padding)), payload))
      return;
   splitNode(node, parent, key, keyLength);
   insert(key, keyLength, payloadLength, payload);
}
void BTree::lookupInner(u8 *key, unsigned keyLength)
{
   BTreeNode *node = root;
   while (node->isInner())
      node = node->lookupInner(key, keyLength);
   assert(node);
}
void BTree::splitNode(BTreeNode *node, BTreeNode *parent, u8 *key, unsigned keyLength)
{
   if (!parent)
   {
      parent = BTreeNode::makeInner();
      parent->upper = node;
      root = parent;
   }
   BTreeNode::SeparatorInfo sepInfo = node->findSep();
   unsigned spaceNeededParent = BTreeNode::spaceNeeded(sepInfo.length, parent->prefix_len);
   if (parent->allocateSpace(spaceNeededParent))
   { 
      u8 sepKey[sepInfo.length];
      node->getSep(sepKey, sepInfo);
      node->split(parent, sepInfo.slot, sepKey, sepInfo.length);
   }
   else
   {
      splitInner(parent, key, keyLength);
   }
}
void BTree::splitInner(BTreeNode *splitingNode, u8 *key, unsigned keyLength)
{
   BTreeNode *node = root;
   BTreeNode *parent = nullptr;
   while (node->isInner() && (node != splitingNode))
   {
      parent = node;
      node = node->lookupInner(key, keyLength);
   }
   splitNode(splitingNode, parent, key, keyLength);
}
bool BTree::remove(u8 *key, unsigned keyLength)
{
   BTreeNode *node = root;
   BTreeNode *parent = nullptr;
   int pos = 0;
   while (node->isInner())
   {
      parent = node;
      pos = node->lowerBound<false>(key, keyLength);
      node = (pos == node->count) ? node->upper : node->getChild(pos);
   }
   static_cast<void>(parent);
   if (!node->remove(key, keyLength))
      return false;
   if (node->spacePostCompact() >= BTreeNodeHeader::under_full)
   {
      if (node != root && (parent->count >= 2) && (pos + 1) < parent->count)
      {
         BTreeNode *right = parent->getChild(pos + 1);
         if (right->spacePostCompact() >= BTreeNodeHeader::under_full)
            return node->merge(pos, parent, right);
      }
   }
   return true;
}
BTree::~BTree()
{
   root->destroy();
}
BTree *btree_create()
{
   return new BTree();
}

void btree_destroy(BTree *btree)
{
   btree->~BTree();
}
// replaces exising record if any
void btree_insert(BTree *btree, u8 *key, u16 keyLength, u8 *payload, u16 payloadLength)

{
   if (!key || !payload)
      return;
   u64 payloadLength16;
   bool result = btree->lookup(key, keyLength, payloadLength16, payload);
   if (result)
   {
      btree->remove(key, keyLength);
   }
   btree->insert(key, keyLength, payloadLength, payload);
}

u8 *btree_lookup(BTree *btree, u8 *key, u16 keyLength, u16 &payloadLength)
{
   if (keyLength == 0 || !key)
      return nullptr;
   u8 *result = new u8[1024];
   u64 payloadLength64;
   if (btree->lookup(key, keyLength, payloadLength64, result))
   {
      payloadLength = payloadLength64 - btree->root->padding;
      return result;
   }
   else
   {
      payloadLength = 0;
      return nullptr;
   }
}

bool btree_remove(BTree *btree, u8 *key, u16 keyLength)
{
   return btree->remove(key, keyLength);
}

bool cont = true;
void inner_rec(BTreeNode *node, uint8_t *key, unsigned keyLength, uint8_t *keyOut,
               const std::function<bool(unsigned int, uint8_t *, unsigned int)>
                   &found_callback)
{
   if (node->is_leaf && cont)
   {
      bool shouldContinue = true;
      for (int i = 0; i < node->count; i++)
      {
         u8 key_inner[1024];
         node->copyKeyOut(i, key_inner, node->getFullKeyLength(i));
         if (BTreeNode::cmpKeys(key, key_inner, keyLength, node->getFullKeyLength(i)) > 0)
         {
            continue;
         }
         auto fullKeyLength = node->getFullKeyLength(i);
         node->copyKeyOut(i, keyOut, fullKeyLength);
         auto payload = (node->isLarge(i)) ? node->getPayloadLarge(i) : node->getPayload(i);
         auto payloadLength = node->getPayloadLength(i) - node->padding;
         shouldContinue = found_callback(fullKeyLength, payload, payloadLength);
         if (!shouldContinue)
         {
            cont = false;
            return;
         }
      }
   }
   else if (cont)
   {
      u8 key_inner[1024];
      node->copyKeyOut(node->count - 1, key_inner, node->getFullKeyLength(node->count - 1));
      auto x = BTreeNode::cmpKeys(key, key_inner, keyLength, node->getFullKeyLength(node->count - 1)) > 0;
      if (x)
      {
         inner_rec(node->upper, key, keyLength, keyOut, found_callback);
         return;
      }
      for (int i = 0; i < node->count; i++)
      {
         inner_rec(node->getChild(i), key, keyLength, keyOut, found_callback);
      }
      inner_rec(node->upper, key, keyLength, keyOut, found_callback);
   }
}

// invokes the callback for all records greater than or equal to key, in order.
// the key should be copied to keyOut before the call.
// the callback should be invoked with keyLength, value pointer, and value
// length iteration stops if there are no more keys or the callback returns
// false.
void btree_scan(BTree *tree, uint8_t *key, unsigned keyLength, uint8_t *keyOut,
                const std::function<bool(unsigned int, uint8_t *, unsigned int)>
                    &found_callback)
{
   if (!tree || !tree->root)
   {
      std::cout << "tree or tree->root is null" << std::endl;
      return;
   }
   cont = true;
   // auto node = tree->root;
   // auto parent = node;
   // while (node->isInner())
   // {
   //    parent = node;
   //    node = node->lookupInner(key, keyLength);
   // }
   inner_rec(tree->root, key, keyLength, keyOut, found_callback);
}

// parent =
// node = node->upper;
// pos = 0;

//  if(pos_outer + 1 < parent->count && parent != node && pos_outer + 1 > pos)
//       {
//          pos_outer++;
//          node = parent->getValue(pos_outer);
//          pos = 0;
//          std :: cout << "pos_outer: " << pos_outer << std::endl;
//          continue;
//       }
//       else

//       if (node->upper == nullptr)
//       {
//          cout << "upper is null" << endl;
//          return;
//       }
//       node = node->upper;

// auto node = tree->root;
//    auto parent = node;
//    unsigned pos_outer = 0;
//    while (node->isInner())
//    {
//       parent = node;
//       node = node->lookupInner(key, keyLength);
//       pos_outer = parent->lookupInnerPos(key, keyLength);
//    }

//    bool single_height = parent == node;

//    std::cout << "node->count: " << node->count << std::endl;
//    std ::cout << "node->isLeaf: " << node->isLeaf << std::endl;
//    std::cout << "The key: " << key << std::endl;
//    std::cout << "the parent count: " << parent->count << std::endl;
//    bool shouldContinue = true;
//    auto pos = node->lowerBound<true>(key, keyLength);

//    while (shouldContinue)
//    {
//       if (pos_outer == -1)
//          pos_outer = 0;
//       for (int i = pos_outer; (parent); i++)
//       {
//          if (pos == -1)
//          {
//             pos = 0;
//          }
//          // node = parent->getValue(i);
//          std ::cout << "pos: " << pos << std::endl;
//          for (; pos < node->count; pos++)
//          {
//             std::cout << "pos: " << pos << std::endl;
//             std ::cout.flush();
//             auto fullKeyLength = node->getFullKeyLength(pos);
//             node->copyFullKey(pos, keyOut, fullKeyLength);
//             std ::cout << "keyOut: " << keyOut << std::endl;
//             auto payload = (node->isLarge(pos)) ? node->getPayloadLarge(pos) : node->getPayload(pos);
//             auto payloadLength = node->getPayloadLength(pos) - tree->root->padding;
//             shouldContinue = found_callback(fullKeyLength, payload, payloadLength);
//             std ::cout << "shouldContinue: " << shouldContinue << std::endl;
//             if (!shouldContinue)
//             {
//                return;
//             }
//          }
//          if (single_height)
//             return;
//          node = parent->getValue(i + 1);
//          std ::cout << "node->count: " << node->count << std::endl;
//          std ::cout << "node->isLeaf: " << node->isLeaf << std::endl;
//          std ::cout << "parent->count: " << parent->count << std::endl;
//          std ::cout << "i: " << i << std::endl;

//          pos = 0;
//       }
//       // if (parent == nullptr)
//       // {
//       //    std::cout << "parent is null" << std::endl;
//       //    return;
//       // }

//       // if (parent->upper == nullptr)
//       // {
//       //    std::cout << "upper is null" << std::endl;
//       //    return;
//       // }

//       // if(node -> upper == nullptr){
//       //    std::cout << "node->upper is null" << std::endl;
//       //    return;
//       // }
//    }