//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// skiplist.cpp
//
// Identification: src/primer/skiplist.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "primer/skiplist.h"
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "common/macros.h"
#include "fmt/core.h"
#include <iostream>

namespace bustub {

/** @brief Checks whether the container is empty. */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Empty() -> bool {
  return header_->Next(LOWEST_LEVEL) == nullptr;
}

/** @brief Returns the number of elements in the skip list. */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Size() -> size_t {
  return size_;  
}

/**
 * @brief Iteratively deallocate all the nodes.
 *
 * We do this to avoid stack overflow when the skip list is large.
 *
 * If we let the compiler handle the deallocation, it will recursively call the destructor of each node,
 * which could block up the the stack.
 */
SKIPLIST_TEMPLATE_ARGUMENTS void SkipList<K, Compare, MaxHeight, Seed>::Drop() {
  for (size_t i = 0; i < MaxHeight; i++) {
    auto curr = std::move(header_->links_[i]);
    while (curr != nullptr) {
      // std::move sets `curr` to the old value of `curr->links_[i]`,
      // and then resets `curr->links_[i]` to `nullptr`.
      curr = std::move(curr->links_[i]);
    }
  }
}

/**
 * @brief Removes all elements from the skip list.
 *
 * Note: You might want to use the provided `Drop` helper function.
 */
SKIPLIST_TEMPLATE_ARGUMENTS void SkipList<K, Compare, MaxHeight, Seed>::Clear() {
  this->Drop();
}

/**
 * @brief Inserts a key into the skip list.
 *
 * Note: `Insert` will not insert the key if it already exists in the skip list.
 *
 * @param key key to insert.
 * @return true if the insertion is successful, false if the key already exists.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Insert(const K &key) -> bool {
  size_t random_height = RandomHeight();
  std::shared_ptr<SkipNode> new_node = std::make_shared<SkipNode>(random_height, key);

  std::vector<std::shared_ptr<SkipNode>> update(MaxHeight, nullptr);
  std::shared_ptr<SkipNode> current = header_;
  for (int i = MaxHeight - 1; i >= 0; --i) {
    while (current->Next(i) != nullptr && compare_(current->Next(i)->Key(), key)) {
      if (!compare_(key, current->Key()) && !compare_(current->Key(), key)) {
        return false;  // Found the key.
      }
      if (compare_(key, current->Key())) {
        break;  // Key is less than current node's key, move to the next level.
      }
      current = current->Next(i);
    }
    update[i] = current;  // Store the last node before the insertion point.
  }

  for (int i = random_height-1; i >= 0 ; i--) {
      new_node->SetNext(i, update[i]->Next(i));
      update[i]->SetNext(i,new_node);
  }
  size_ += 1;
  return true;
}

/**
 * @brief Erases the key from the skip list.
 *
 * @param key key to erase.
 * @return bool true if the element got erased, false otherwise.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Erase(const K &key) -> bool {
  if( not Contains(key)) {
    return false;
  }
  for (size_t i = MaxHeight-1; i >= 0 ; i--) {
    auto node = Header();
    while (node != nullptr) {
      if (!compare_(key, node->Next(i)->Key()) && !compare_(node->Next(i)->Key(), key)) {
        node->SetNext(i, node->Next(i)->Next(i));
      }
      if (compare_(key, node->Key())) {
        break;  // Key is less than current node's key, move to the next level.
      }
      node = node->Next(i);  // Move to the next node at this level.
    }
  }
  return true;
}

/**
 * @brief Checks whether a key exists in the skip list.
 *
 * @param key key to look up.
 * @return bool true if the element exists, false otherwise.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Contains(const K &key) -> bool {
  // Following the standard library: Key `a` and `b` are considered equivalent if neither compares less
  // than the other: `!compare_(a, b) && !compare_(b, a)`.
  for (size_t i = MaxHeight-1; i >= 0 ; i--) {
    auto node = header_->Next(i);
    while (node != nullptr) {
      if (!compare_(key, node->Key()) && !compare_(node->Key(), key)) {
        return true;  // Found the key.
      }
      if (compare_(key, node->Key())) {
        break;  // Key is less than current node's key, move to the next level.
      }
      node = node->Next(i);  // Move to the next node at this level.
    }
  }
  return false;
}

/**
 * @brief Prints the skip list for debugging purposes.
 *
 * Note: You may modify the functions in any way and the output is not tested.
 */
SKIPLIST_TEMPLATE_ARGUMENTS void SkipList<K, Compare, MaxHeight, Seed>::Print() {
  auto node = header_->Next(LOWEST_LEVEL);
  while (node != nullptr) {
    fmt::println("Node {{ key: {}, height: {} }}", node->Key(), node->Height());
    node = node->Next(LOWEST_LEVEL);
  }
}

/**
 * @brief Generate a random height. The height should be cappped at `MaxHeight`.
 * Note: we implement/simulate the geometric process to ensure platform independence.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::RandomHeight() -> size_t {
  // Branching factor (1 in 4 chance), see Pugh's paper.
  static constexpr unsigned int branching_factor = 4;
  // Start with the minimum height
  size_t height = 1;
  while (height < MaxHeight && (rng_() % branching_factor == 0)) {
    height++;
  }
  return height;
}

/**
 * @brief Gets the current node height.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::SkipNode::Height() const -> size_t {
  return links_.size();
}

/**
 * @brief Gets the next node by following the link at `level`.
 *
 * @param level index to the link.
 * @return std::shared_ptr<SkipNode> the next node, or `nullptr` if such node does not exist.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::SkipNode::Next(size_t level) const
    -> std::shared_ptr<SkipNode> {
  return links_[level];
}

/**
 * @brief Set the `node` to be linked at `level`.
 *
 * @param level index to the link.
 */
SKIPLIST_TEMPLATE_ARGUMENTS void SkipList<K, Compare, MaxHeight, Seed>::SkipNode::SetNext(
    size_t level, const std::shared_ptr<SkipNode> &node) {
  links_[level] = std::move(node);
}

/** @brief Returns a reference to the key stored in the node. */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::SkipNode::Key() const -> const K & {
  return key_;
}

// Below are explicit instantiation of template classes.
template class SkipList<int>;
template class SkipList<std::string>;
template class SkipList<int, std::greater<>>;
template class SkipList<int, std::less<>, 8>;

}  // namespace bustub

// using namespace bustub;

// int main() {
//   SkipList<int> list;

//   // All the keys we will insert into the skip list (1 to 20).
//   std::vector<int> keys = {12, 16, 2, 6, 15, 8, 13, 1, 11, 14, 0, 4, 19, 10, 9, 5, 7, 3, 17, 18};

//   // Heights of the nodes in the skip list.
//   // These will not change since we fixed the seed of the random number generator.
//   std::vector<uint32_t> heights = {2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 2, 1, 3, 1, 1, 2, 1, 1, 2, 3};

//   // Insert the keys
//   for (auto key : keys) {
//     list.Insert(key);
//   }

//   // Sort the keys
//   std::sort(keys.begin(), keys.end());

//   list.Print(); 
// }