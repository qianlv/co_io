#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <stack>
#include <string_view>

template <typename Value> class AdaptiveRadixTree {
  struct Node;
  struct Leaf;

  struct Node4;
  struct Node16;
  struct Node48;
  struct Node256;

public:
  void insert(std::string_view key, Value value);
  std::optional<Value> search(std::string_view key);
  void debug();
  // void erase();
  class Iterator;
  Iterator begin();
  Iterator end();

  ~AdaptiveRadixTree() { delete root; }

  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<std::string, Value>;
    using pointer = value_type *;
    using reference = value_type &;

    Iterator() = default;
    explicit Iterator(Node *node) {
      stack.push({node->prefix, node});
      if (!next_leaf(0)) { // empty tree
        stack = std::stack<std::pair<std::string, Node *>>();
      } else {
        value = {stack.top().first, stack.top().second->leaf->value};
      }
    }

    reference operator*() {
      assert(stack.size() > 0);
      return value;
    }

    bool operator==(const Iterator &other) const {
      if (stack.size() != other.stack.size()) {
        return false;
      }
      if (stack.empty() || (stack.size() > 0 &&
                            stack.top().second == other.stack.top().second)) {
        return true;
      }
      return false;
    }

    bool operator!=(const Iterator &other) const { return !(*this == other); }

    pointer operator->() {
      assert(stack.size() > 0);
      return &value;
    }

    Iterator &operator++() {
      assert(stack.size() > 0);
      uint16_t start_key = 0;
      while (!stack.empty() && !next_leaf(start_key)) { // not new leaf, pop top
        if (!stack.top().second->prefix.empty()) { // non root node, only root
                                                   // node prefix is empty
          uint16_t next_key =
              static_cast<uint16_t>(stack.top().second->prefix.front()) + 1;
          start_key = next_key;
        }
        stack.pop();
      }
      if (!stack.empty()) {
        value = {stack.top().first, stack.top().second->leaf->value};
      }
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

  private:
    bool next_leaf(uint16_t start_key) {
      assert(stack.size() > 0);
      // std::cerr << stack.top().second->prefix << " " << static_cast<char>(start_key) << std::endl;
      bool new_leaf = false;
      do {
        auto &[prefix, current] = stack.top();
        uint16_t next_key = 256;
        if (current->nchilds > 0 &&
            (next_key = current->lower_bound_key(start_key)) != 256) {
          Node **next = current->get_node(next_key);
          assert(next != nullptr);
          stack.push({prefix + (*next)->prefix, *next});
          start_key = 0;
          new_leaf = true;
        } else {
          break;
        }
      } while (stack.top().second->leaf == nullptr);
      return new_leaf;
    }
    std::pair<std::string, Value> value;
    std::stack<std::pair<std::string, Node *>> stack;
  };

private:
  Node *root = new Node4{};
};

template <typename Value> struct AdaptiveRadixTree<Value>::Leaf {
  Value value;

  Leaf(Value v) : value(std::move(v)) {}
};

template <typename Value> struct AdaptiveRadixTree<Value>::Node {
  std::string prefix = "";
  uint8_t nchilds = {0};
  Leaf *leaf = {nullptr};

  size_t match(std::string_view match_prefix) const {
    // std::cerr << "prefix = " << this->prefix << " match = " << match_prefix
    // << std::endl;
    size_t i = 0;
    for (i = 0; i < match_prefix.size() && i < this->prefix.size(); i++) {
      if (this->prefix[i] != match_prefix[i]) {
        break;
      }
    }
    return i;
  }

  Node() = default;
  Node(std::string_view prefix) : prefix(prefix) {}
  Node(const Node &) = default;
  virtual ~Node() {
    if (leaf) {
      delete leaf;
    }
  }

  void copy(Node *node) {
    this->prefix = std::move(node->prefix);
    this->nchilds = node->nchilds;
    this->leaf = node->leaf;
    node->leaf = nullptr;
  }

  void debug(size_t width) {
    std::string s = std::string(width, ' ');
    s.append(this->prefix);
    s.append(leaf ? "(l)" : "(n)");
    std::cerr << s << std::endl;
    for (unsigned i = 0; i < 256; ++i) {
      Node **node = get_node(static_cast<uint8_t>(i));
      if (node) {
        (*node)->debug(s.length());
      }
    }
  }

  virtual void add_node(uint8_t k, Node *node) = 0;
  virtual void del_node(uint8_t k) = 0;
  virtual Node **get_node(uint8_t k) = 0;
  virtual void clear_childs() = 0;
  virtual Node *grow() = 0;
  virtual Node *shrink() = 0;
  virtual bool is_full() = 0;
  virtual Node *clone() = 0;
  virtual uint16_t lower_bound_key(uint16_t k) = 0;
};

template <typename Value> struct AdaptiveRadixTree<Value>::Node4 : public Node {
  static constexpr uint8_t N = 4;

  uint8_t key[N];
  Node *childs[N];

  Node4() { clear_childs(); }

  Node4(std::string_view prefix) : Node{prefix} { clear_childs(); }

  Node4(const Node4 &node) : Node(node) {
    std::copy(node.key, node.key + N, key);
    std::copy(node.childs, node.childs + N, childs);
  }

  void clear_childs() override {
    std::fill(key, key + N, 0);
    std::fill(childs, childs + N, nullptr);
    Node::nchilds = 0;
  }

  uint16_t lower_bound_key(uint16_t k) override {
    uint8_t index = 0;
    while (index < Node::nchilds && key[index] < k) {
      index++;
    }
    if (index < Node::nchilds) {
      return key[index];
    } else {
      return 256;
    }
  }

  void add_node(uint8_t k, Node *node) override {
    assert(Node::nchilds < N);
    uint8_t index = 0;
    while (index < Node::nchilds && key[index] < k) {
      index += 1;
    }
    for (uint8_t i = Node::nchilds; i > index; i--) {
      key[i] = key[i - 1];
      childs[i] = childs[i - 1];
    }
    key[index] = k;
    childs[index] = node;
    Node::nchilds++;
  }

  void del_node(uint8_t k) override {
    for (uint8_t i = 0; i < N; i++) {
      if (childs[i] && key[i] == k) {
        childs[i] = nullptr;
        Node::nchilds--;
      }
    }
  }

  Node **get_node(uint8_t k) override {
    for (uint8_t i = 0; i < N; i++) {
      if (childs[i] && key[i] == k) {
        return &childs[i];
      }
    }
    return nullptr;
  }

  Node *grow() override {
    // std::cerr << "grow 4 -> 16\n";
    Node16 *node = new Node16{};
    std::copy(key, key + N, node->key);
    std::copy(childs, childs + N, node->childs);
    node->copy(this);
    clear_childs();
    return node;
  }
  Node *shrink() override { return nullptr; }
  bool is_full() override { return Node::nchilds == N; }

  Node *clone() override { return new Node4(*this); }

  ~Node4() override {
    for (uint8_t i = 0; i < N; i++) {
      if (childs[i]) {
        delete childs[i];
      }
    }
  }
};

template <typename Value>
struct AdaptiveRadixTree<Value>::Node16 : public Node {
  static constexpr uint8_t N = 16;

  uint8_t key[N];
  Node *childs[N];

  Node16() { clear_childs(); }
  Node16(std::string_view prefix) : Node{prefix} { clear_childs(); }
  Node16(const Node16 &node) : Node(node) {
    std::copy(node.key, node.key + N, key);
    std::copy(node.childs, node.childs + N, childs);
  }

  void clear_childs() override {
    std::fill(key, key + N, 0);
    std::fill(childs, childs + N, nullptr);
    Node::nchilds = 0;
  }

  uint16_t lower_bound_key(uint16_t k) override {
    uint8_t index = 0;
    while (index < Node::nchilds && key[index] < k) {
      // std::cerr << "k = " << k << " key[" << index << "] = " << static_cast<char>(key[index]) << std::endl;
      index++;
    }
    if (index < Node::nchilds) {
      return key[index];
    } else {
      return 256;
    }
  }

  void add_node(uint8_t k, Node *node) override {
    uint8_t index = 0;
    while (index < Node::nchilds && key[index] < k) {
      index += 1;
    }
    for (uint8_t i = Node::nchilds; i > index; i--) {
      key[i] = key[i - 1];
      childs[i] = childs[i - 1];
    }
    key[index] = k;
    childs[index] = node;
    Node::nchilds++;
  }

  void del_node(uint8_t k) override {
    for (uint8_t i = 0; i < N; i++) {
      if (childs[i] && key[i] == k) {
        childs[i] = nullptr;
        Node::nchilds--;
      }
    }
  }

  Node **get_node(uint8_t k) override {
    for (uint8_t i = 0; i < N; i++) {
      if (childs[i] && key[i] == k) {
        return &childs[i];
      }
    }
    return nullptr;
  }

  Node *grow() override {
    // std::cerr << "grow 16 -> 48\n";
    Node48 *node = new Node48{};
    for (uint8_t i = 0; i < N; i++) {
      node->key[key[i]] = i;
      node->childs[i] = childs[i];
      childs[i] = nullptr;
    }
    node->copy(this);
    return node;
  }

  Node *shrink() override {
    assert(Node::nchilds <= Node4::N);
    Node4 *node = new Node4{};
    std::copy(key, key + Node4::N, node->key);
    std::copy(childs, childs + Node4::N, node->childs);
    node->copy(this);
    return node;
  }
  bool is_full() override { return Node::nchilds == N; }
  Node *clone() override { return new Node16(*this); }

  ~Node16() override {
    for (uint8_t i = 0; i < N; i++) {
      if (childs[i]) {
        delete childs[i];
      }
    }
  }
};

template <typename Value>
struct AdaptiveRadixTree<Value>::Node48 : public Node {
  uint8_t key[256];
  Node *childs[48];

  static constexpr uint8_t N = 48;

  Node48() { clear_childs(); }
  Node48(std::string_view prefix) : Node{prefix} { clear_childs(); }
  Node48(const Node48 &node) : Node(node) {
    std::copy(node.key, node.key + 256, key);
    std::copy(node.childs, node.childs + N, childs);
  }

  void clear_childs() override {
    std::fill(key, key + 256, N);
    std::fill(childs, childs + N, nullptr);
    Node::nchilds = 0;
  }

  uint16_t lower_bound_key(uint16_t k) override {
    uint16_t nk = k;
    while (nk < 256 && key[nk] == N) {
      nk += 1;
    }
    return nk;
  }

  void add_node(uint8_t k, Node *node) override {
    assert(Node::nchilds < N);
    if (key[k] == N) {
      for (uint8_t i = 0; i < N; i++) {
        if (!childs[i]) {
          key[k] = i;
          childs[i] = node;
          Node::nchilds++;
          return;
        }
      }
    }
  }

  void del_node(uint8_t k) override {
    if (key[k] != N) {
      childs[key[k]] = nullptr;
      key[k] = N;
      Node::nchilds--;
    }
  }

  Node **get_node(uint8_t k) override {
    if (key[k] != N) {
      return &childs[key[k]];
    }
    return nullptr;
  }

  Node *grow() override {
    // std::cerr << "grow 48 -> 256\n";
    Node256 *node = new Node256{};
    for (uint16_t i = 0; i < 256; i++) {
      if (key[i] != N) {
        node->childs[i] = childs[key[i]];
        childs[key[i]] = nullptr;
      }
    }
    node->copy(this);
    return node;
  }

  Node *shrink() override {
    assert(Node::nchilds <= Node16::N);
    Node16 *node = new Node16{};
    uint8_t j = 0;
    for (uint16_t i = 0; i < 256; ++i) {
      if (key[i] != N) {
        node->key[j] = key[i];
        node->childs[j] = childs[key[i]];
        j++;
      }
    }
    node->copy(this);
    return node;
  }
  bool is_full() override { return Node::nchilds == N; }
  Node *clone() override { return new Node48(*this); }

  ~Node48() override {
    for (uint8_t i = 0; i < N; i++) {
      if (childs[i]) {
        delete childs[i];
      }
    }
  }
};

template <typename Value>
struct AdaptiveRadixTree<Value>::Node256 : public Node {
  static constexpr uint16_t N = 256;
  Node *childs[N];

  Node256() { clear_childs(); }
  Node256(std::string_view prefix) : Node{prefix} { clear_childs(); }
  Node256(const Node256 &node) : Node(node) {
    std::copy(node.childs, node.childs + N, childs);
  }

  void clear_childs() override {
    std::fill(childs, childs + N, nullptr);
    Node::nchilds = 0;
  }

  uint16_t lower_bound_key(uint16_t k) override {
    uint16_t nk = k;
    while (nk < 256 && childs[nk] == nullptr) {
      nk += 1;
    }
    return nk;
  }

  void add_node(uint8_t k, Node *node) override {
    assert(Node::nchilds < N);
    if (childs[k] == nullptr) {
      childs[k] = node;
      Node::nchilds++;
    }
  }

  void del_node(uint8_t k) override {
    if (childs[k] != nullptr) {
      childs[k] = nullptr;
      Node::nchilds--;
    }
  }

  Node **get_node(uint8_t k) override {
    return childs[k] != nullptr ? &childs[k] : nullptr;
  }

  Node *grow() override { return nullptr; }

  Node *shrink() override {
    assert(Node::nchilds <= Node48::N);
    Node48 *node = new Node48{};
    uint8_t j = 0;
    for (uint16_t i = 0; i < N; ++i) {
      if (childs[i] != nullptr) {
        node->key[i] = j;
        node->childs[j] = childs[i];
        j++;
      }
    }
    node->copy(this);
    return node;
  }
  bool is_full() override { return Node::nchilds == N; }
  Node *clone() override { return new Node256(*this); }

  ~Node256() override {
    for (uint16_t i = 0; i < N; i++) {
      if (childs[i]) {
        delete childs[i];
      }
    }
  }
};

template <typename Value>
void AdaptiveRadixTree<Value>::insert(std::string_view key, Value value) {
  Node *current = root;
  Node **parent = &root;
  while (!key.empty()) {
    size_t match_prefix = current->match(key);
    // std::cerr << "commmon_prefix = "
    //           << std::string_view(key.data(), match_prefix)
    //           << " current_remaing = "
    //           << std::string_view(current->prefix.data() + match_prefix,
    //                               current->prefix.length() - match_prefix)
    //           << " key_remaing = "
    //           << std::string_view(key.data() + match_prefix,
    //                               key.length() - match_prefix)
    //           << std::endl;
    if (current->prefix.length() > match_prefix) { // split at current node
      Node *new_node = current->clone();
      new_node->prefix = current->prefix.substr(match_prefix);

      current->prefix = current->prefix.substr(0, match_prefix);
      current->clear_childs();
      current->add_node(new_node->prefix[0], new_node);
      current->leaf = nullptr;
      if (key.length() > match_prefix) {
        current->add_node(key[match_prefix], new Node4{
                                                 key.substr(match_prefix),
                                             });
      }
    }

    key.remove_prefix(match_prefix);
    if (!key.empty() && current->get_node(key[0]) == nullptr) {
      if (current->is_full()) {
        *parent = current->grow();
        delete current;
        current = *parent;
      }
      current->add_node(key[0], new Node4{key});
    }

    if (!key.empty()) {
      parent = current->get_node(key[0]);
      current = *parent;
    }
  }
  if (current->leaf) {
    current->leaf->value = value;
  } else {
    current->leaf = new Leaf{value};
  }
}

template <typename Value>
std::optional<Value> AdaptiveRadixTree<Value>::search(std::string_view key) {
  Node *current = root;
  while (!key.empty()) {
    size_t match_prefix = current->match(key);
    // std::cerr << "commmon_prefix = "
    //           << std::string_view(key.data(), match_prefix)
    //           << " current_remaing = "
    //           << std::string_view(current->prefix.data() + match_prefix,
    //                               current->prefix.length() - match_prefix)
    //           << " key_remaing = "
    //           << std::string_view(key.data() + match_prefix,
    //                               key.length() - match_prefix)
    //           << std::endl;
    if (current->prefix.length() > match_prefix) { // prefix not complete match
      return {};
    }
    key.remove_prefix(match_prefix);
    if (key.empty()) { // prefix complete match and remaining key is empty
      break;
    }
    Node **p = current->get_node(key[0]);
    if (p == nullptr) { // remaing key not next node to match
      return {};
    }
    current = *p;
  }
  if (current->leaf != nullptr) {
    return {current->leaf->value};
  }
  return {};
}

template <typename Value> void AdaptiveRadixTree<Value>::debug() {
  this->root->debug(0);
  std::cerr << "-----------------------------------\n";
}

template <typename Value>
AdaptiveRadixTree<Value>::Iterator AdaptiveRadixTree<Value>::begin() {
  return Iterator{root};
}

template <typename Value>
AdaptiveRadixTree<Value>::Iterator AdaptiveRadixTree<Value>::end() {
  return {};
}
