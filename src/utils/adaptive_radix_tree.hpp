#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <stack>
#include <string_view>

template <typename Value> class AdaptiveRadixTree {
    struct Node;
    struct Node4;
    struct Node16;
    struct Node48;
    struct Node256;

  public:
    struct Leaf;

    AdaptiveRadixTree() = default;
    AdaptiveRadixTree(const AdaptiveRadixTree &other) = delete;
    AdaptiveRadixTree &operator=(const AdaptiveRadixTree &other) = delete;

    AdaptiveRadixTree(const AdaptiveRadixTree &&other) = default;
    AdaptiveRadixTree &operator=(AdaptiveRadixTree &&other) = default;
    ~AdaptiveRadixTree() { delete root; }

    void insert(std::string_view key, Value value);
    std::optional<Value> search(std::string_view key);
    bool remove(std::string_view key);

    void debug();
    class Iterator;
    Iterator begin();
    Iterator end();

    class Iterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Leaf;
        using pointer = value_type *;
        using reference = value_type &;

        Iterator() = default;
        explicit Iterator(Node *node) {
            stack.push(node);
            if (!next_leaf(0)) { // empty tree
                stack = std::stack<Node *>();
            }
        }

        reference operator*() {
            assert(stack.size() > 0);
            return *stack.top()->leaf_;
        }

        pointer operator->() {
            assert(stack.size() > 0);
            return stack.top()->leaf_;
        }

        bool operator==(const Iterator &other) const {
            if (stack.size() != other.stack.size()) {
                return false;
            }
            if (stack.empty() || (stack.size() > 0 && stack.top() == other.stack.top())) {
                return true;
            }
            return false;
        }

        bool operator!=(const Iterator &other) const { return !(*this == other); }

        Iterator &operator++() {
            assert(stack.size() > 0);
            uint16_t start_key = 0;
            while (!stack.empty() && !next_leaf(start_key)) { // not new leaf, pop top
                if (!stack.top()->prefix_.empty()) {          // non root node, only root
                                                              // node prefix is empty
                    uint16_t next_key = static_cast<uint16_t>(stack.top()->prefix_.front()) + 1;
                    start_key = next_key;
                }
                stack.pop();
            }
            return *this;
        }

        // Iterator operator++(int) {
        //   Iterator tmp = *this;
        //   ++(*this);
        //   return tmp;
        // }

      private:
        bool next_leaf(uint16_t start_key) {
            assert(stack.size() > 0);
            // std::cerr << stack.top().second->prefix << " " <<
            // static_cast<char>(start_key) << std::endl;
            bool new_leaf = false;
            do {
                Node *current = stack.top();
                if (auto result = current->next_or_equal_key(start_key); result) {
                    auto *next = result->first;
                    stack.push(next);
                    start_key = 0;
                    new_leaf = true;
                } else {
                    break;
                }
            } while (stack.top()->leaf_ == nullptr);
            return new_leaf;
        }
        std::stack<Node *> stack;
    };

  private:
    Node *root = new Node4{""};
};

template <typename Value> struct AdaptiveRadixTree<Value>::Node {
    static constexpr size_t MaxChilds = 256;
    std::string prefix_;
    size_t size_ = 0;
    Leaf *leaf_{};

    explicit Node(std::string_view prefix) : prefix_(prefix) {}
    Node(Node &&node) noexcept
        : prefix_(std::move(node.prefix_)), size_(node.size_), leaf_(node.leaf_) {
        node.size_ = 0;
        node.prefix_.clear();
        node.leaf_ = nullptr;
    }
    Node &operator=(Node &&node) noexcept {
        std::swap(prefix_, node.prefix_);
        std::swap(leaf_, node.leaf_);
        std::swap(size_, node.size_);
        return *this;
    }

    [[nodiscard]] size_t match(std::string_view match_prefix) const {
        // std::cerr << "prefix = " << this->prefix_ << " match = " << match_prefix << std::endl;
        size_t i = 0;
        for (i = 0; i < match_prefix.size() && i < this->prefix_.size(); i++) {
            if (this->prefix_[i] != match_prefix[i]) {
                break;
            }
        }
        return i;
    }

    void debug(size_t width) {
        std::string str = std::string(width, ' ');
        str.append(this->prefix_);
        str.append(leaf_ ? "(l)" : "(n)");
        std::cerr << str << std::endl;
        auto current = first_key();
        while (current) {
            current->first->debug(str.length());
            current = next_or_equal_key(current->second + 1);
        }
    }

    Node() = default;
    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;
    virtual ~Node() { delete leaf_; }

    [[nodiscard]] virtual size_t size() const noexcept { return size_; }
    [[nodiscard]] virtual bool is_full() const noexcept = 0;
    [[nodiscard]] virtual bool should_shrink() const noexcept = 0;
    virtual bool insert(uint8_t key, Node *node) = 0;
    virtual Node **find(uint8_t key) = 0;
    virtual void remove(uint8_t key) = 0;
    virtual std::optional<std::pair<Node *, uint8_t>>
    next_or_equal_key(size_t key) const noexcept = 0;
    virtual std::optional<std::pair<Node *, uint8_t>>
    prev_or_equal_key(size_t key) const noexcept = 0;
    virtual std::optional<std::pair<Node *, uint8_t>> first_key() const noexcept = 0;
    virtual std::optional<std::pair<Node *, uint8_t>> last_key() const noexcept = 0;
    virtual Node *move() = 0;
    virtual Node *grow() = 0;
    virtual Node *shrink() = 0;
};

template <typename Value> struct AdaptiveRadixTree<Value>::Leaf {
    std::string key;
    Value value;

    explicit Leaf(std::string_view key, Value val) : key(key), value(std::move(val)) {}
};

template <typename Value> struct AdaptiveRadixTree<Value>::Node4 : public Node {
    friend struct Node16;

    constexpr static size_t SIZE = 4;
    std::array<uint8_t, SIZE> keys_{};
    std::array<Node *, SIZE> childs_{};

    Node4(const Node4 &) = delete;
    Node4 &operator=(const Node4 &) = delete;
    explicit Node4(std::string_view prefix) : Node(prefix) {}
    Node4(Node4 &&node) noexcept
        : Node(std::move(node)), keys_(std::move(node.keys_)), childs_(std::move(node.childs_)) {
        node.childs_.fill(nullptr);
    }
    Node4 &operator=(Node4 &&node) noexcept {
        std::swap(Node::prefix_, node.prefix_);
        std::swap(Node::leaf_, node.leaf_);
        std::swap(Node::size_, node.size_);
        std::swap(keys_, node.keys_);
        std::swap(childs_, node.childs_);
        return *this;
    }
    explicit Node4(Node16 &&node);

    ~Node4() override {
        for (uint8_t i = 0; i < Node::size(); i++) {
            delete childs_[i];
        }
    }

    [[nodiscard]] bool is_full() const noexcept override { return Node::size() == SIZE; }
    [[nodiscard]] bool should_shrink() const noexcept override { return false; }
    bool insert(uint8_t key, Node *node) override {
        auto index = static_cast<size_t>(
            std::lower_bound(keys_.begin(), keys_.begin() + Node::Node::size(), key) -
            keys_.begin());
        if (index == Node::Node::size() || keys_[index] != key) {
            for (size_t i = Node::Node::size(); i > index; i--) {
                keys_[i] = keys_[i - 1];
                childs_[i] = childs_[i - 1];
            }
            keys_[index] = static_cast<uint8_t>(key);
            childs_[index] = node;
            Node::size_ += 1;
            return true;
        }
        return false;
    }

    void remove(uint8_t key) override {
        auto index = static_cast<size_t>(
            std::lower_bound(keys_.begin(), keys_.begin() + Node::Node::size(), key) -
            keys_.begin());
        if (index == Node::Node::size() || keys_[index] != key) {
            return;
        }
        delete childs_[index];
        for (size_t i = index; i < Node::Node::size() - 1; i++) {
            keys_[i] = keys_[i + 1];
            childs_[i] = childs_[i + 1];
        }
        Node::size_ -= 1;
    }

    Node **find(uint8_t key) override {
        auto index = static_cast<size_t>(
            std::lower_bound(keys_.begin(), keys_.begin() + Node::size(), key) - keys_.begin());
        if (index == Node::size() || keys_[index] != key) {
            return nullptr;
        }
        return &childs_[index];
    }

    std::optional<std::pair<Node *, uint8_t>>
    prev_or_equal_key(size_t key) const noexcept override {
        auto index = static_cast<size_t>(
            std::lower_bound(keys_.begin(), keys_.begin() + Node::size(), key) - keys_.begin());
        if (index != Node::size() && keys_[index] == key) {
            return std::make_optional(std::make_pair(childs_[index], key));
        }

        if (index == 0) {
            return std::nullopt;
        }
        return std::make_optional(std::make_pair(childs_[index - 1], keys_[index - 1]));
    }

    std::optional<std::pair<Node *, uint8_t>>
    next_or_equal_key(size_t key) const noexcept override {
        auto index = static_cast<size_t>(
            std::lower_bound(keys_.begin(), keys_.begin() + Node::size(), key) - keys_.begin());
        if (index == Node::size()) {
            return std::nullopt;
        }
        return std::make_optional(std::make_pair(childs_[index], keys_[index]));
    }

    std::optional<std::pair<Node *, uint8_t>> first_key() const noexcept override {
        if (Node::size() == 0) {
            return std::nullopt;
        }
        return std::make_optional(std::make_pair(childs_[0], keys_[0]));
    }
    std::optional<std::pair<Node *, uint8_t>> last_key() const noexcept override {
        if (Node::size() == 0) {
            return std::nullopt;
        }
        return std::make_optional(
            std::make_pair(childs_[Node::size() - 1], keys_[Node::size() - 1]));
    }

    Node *move() override { return new Node4(std::move(*this)); }
    Node *shrink() override;
    Node *grow() override;
};

template <typename Value> struct AdaptiveRadixTree<Value>::Node16 : public Node {
    friend struct Node4;
    friend struct Node48;

    constexpr static size_t SIZE = 16;
    std::array<uint8_t, SIZE> keys_{};
    std::array<Node *, SIZE> childs_{};

    Node16(const Node16 &) = delete;
    Node16 &operator=(const Node16 &) = delete;
    explicit Node16(std::string_view prefix) : Node(prefix) {}
    Node16(Node16 &&node) noexcept
        : Node(std::move(node)), keys_(std::move(node.keys_)), childs_(std::move(node.childs_)) {
        node.childs_.fill(nullptr);
    }
    Node16 &operator=(Node16 &&node) noexcept {
        std::swap(Node::prefix_, node.prefix_);
        std::swap(Node::leaf_, node.leaf_);
        std::swap(Node::size_, node.size_);
        std::swap(keys_, node.keys_);
        std::swap(childs_, node.childs_);
        return *this;
    }
    explicit Node16(Node4 &&node);
    explicit Node16(Node48 &&node);
    ~Node16() override {
        for (uint8_t i = 0; i < Node::size(); i++) {
            delete childs_[i];
        }
    }

    [[nodiscard]] bool is_full() const noexcept override { return Node::size() == SIZE; }
    [[nodiscard]] bool should_shrink() const noexcept override {
        return Node::size() <= Node4::SIZE;
    }
    bool insert(uint8_t key, Node *node) override {
        auto index = static_cast<size_t>(
            std::lower_bound(keys_.begin(), keys_.begin() + Node::size(), key) - keys_.begin());
        if (index == Node::size() || keys_[index] != key) {
            for (size_t i = Node::size(); i > index; i--) {
                keys_[i] = keys_[i - 1];
                childs_[i] = childs_[i - 1];
            }
            keys_[index] = static_cast<uint8_t>(key);
            childs_[index] = node;
            Node::size_ += 1;
            return true;
        }
        return false;
    }

    void remove(uint8_t key) override {
        auto index = static_cast<size_t>(
            std::lower_bound(keys_.begin(), keys_.begin() + Node::size(), key) - keys_.begin());
        if (index == Node::size() || keys_[index] != key) {
            return;
        }
        delete childs_[index];
        for (size_t i = index; i < Node::size() - 1; i++) {
            keys_[i] = keys_[i + 1];
            childs_[i] = childs_[i + 1];
        }
        Node::size_ -= 1;
    }

    Node **find(uint8_t key) override {
        auto index = static_cast<size_t>(
            std::lower_bound(keys_.begin(), keys_.begin() + Node::size(), key) - keys_.begin());
        if (index == Node::size() || keys_[index] != key) {
            return nullptr;
        }
        return &childs_[index];
    }

    std::optional<std::pair<Node *, uint8_t>>
    prev_or_equal_key(size_t key) const noexcept override {
        auto index = static_cast<size_t>(
            std::lower_bound(keys_.begin(), keys_.begin() + Node::size(), key) - keys_.begin());
        if (index != Node::size() && keys_[index] == key) {
            return std::make_optional(std::make_pair(childs_[index], key));
        }

        if (index == 0) {
            return std::nullopt;
        }
        return std::make_optional(std::make_pair(childs_[index - 1], keys_[index - 1]));
    }

    std::optional<std::pair<Node *, uint8_t>>
    next_or_equal_key(size_t key) const noexcept override {
        auto index = static_cast<size_t>(
            std::lower_bound(keys_.begin(), keys_.begin() + Node::size(), key) - keys_.begin());
        if (index == Node::size()) {
            return std::nullopt;
        }
        return std::make_optional(std::make_pair(childs_[index], keys_[index]));
    }

    std::optional<std::pair<Node *, uint8_t>> first_key() const noexcept override {
        if (Node::size() == 0) {
            return std::nullopt;
        }
        return std::make_optional(std::make_pair(childs_[0], keys_[0]));
    }
    std::optional<std::pair<Node *, uint8_t>> last_key() const noexcept override {
        if (Node::size() == 0) {
            return std::nullopt;
        }
        return std::make_optional(
            std::make_pair(childs_[Node::size() - 1], keys_[Node::size() - 1]));
    }

    Node *move() override { return new Node16(std::move(*this)); }
    Node *shrink() override;
    Node *grow() override;
};

template <typename Value> struct AdaptiveRadixTree<Value>::Node48 : public Node {
    friend struct Node16;
    friend struct Node256;
    static constexpr int SIZE = 48;
    std::array<uint8_t, Node::MaxChilds> keys_{};
    std::array<Node *, SIZE> childs_{};

    Node48(const Node48 &) = delete;
    Node48 &operator=(const Node48 &) = delete;
    explicit Node48(std::string_view prefix) : Node(prefix) { keys_.fill(SIZE); }
    Node48(Node48 &&node) noexcept
        : Node(std::move(node)), keys_(std::move(node.keys_)), childs_(std::move(node.childs_)) {
        node.keys_.fill(SIZE);
        node.childs_.fill(nullptr);
    }
    Node48 &operator=(Node48 &&node) noexcept {
        std::swap(Node::prefix_, node.prefix_);
        std::swap(Node::leaf_, node.leaf_);
        std::swap(Node::size_, node.size_);
        std::swap(keys_, node.keys_);
        std::swap(childs_, node.childs_);
        return *this;
    }
    explicit Node48(Node16 &&node);
    explicit Node48(Node256 &&node);
    ~Node48() {
        for (uint8_t i = 0; i < Node::size(); i++) {
            delete childs_[i];
        }
    }

    [[nodiscard]] bool is_full() const noexcept override { return Node::size() == SIZE; }
    [[nodiscard]] bool should_shrink() const noexcept override {
        return Node::size() <= Node16::SIZE;
    }
    bool insert(uint8_t key, Node *node) override {
        uint8_t index = keys_[key];
        if (index != SIZE) {
            return false;
        }

        for (index = 0; index < SIZE && childs_[index]; ++index) {
            ;
        }
        Node::size_ += 1;
        keys_[key] = index;
        childs_[index] = node;
        return true;
    }

    void remove(uint8_t key) override {
        if (uint8_t index = keys_[key]; index != SIZE) {
            keys_[key] = SIZE;
            delete childs_[index];
            childs_[index] = nullptr;
            Node::size_ -= 1;
        }
    }

    Node **find(uint8_t key) override {
        if (uint8_t index = keys_[key]; index != SIZE) {
            return &childs_[index];
        }
        return nullptr;
    }

    std::optional<std::pair<Node *, uint8_t>>
    next_or_equal_key(size_t key) const noexcept override {
        while (key < Node::MaxChilds && keys_[key] == SIZE) {
            key += 1;
        }
        if (key == Node::MaxChilds) {
            return std::nullopt;
        }
        return std::make_optional(std::make_pair(childs_[keys_[key]], static_cast<uint8_t>(key)));
    }

    std::optional<std::pair<Node *, uint8_t>>
    prev_or_equal_key(size_t key) const noexcept override {
        while (key > 0 && keys_[key] == SIZE) {
            key -= 1;
        }
        if (key == 0 && keys_[key] == SIZE) {
            return std::nullopt;
        }
        return std::make_optional(std::make_pair(childs_[keys_[key]], static_cast<uint8_t>(key)));
    }

    std::optional<std::pair<Node *, uint8_t>> first_key() const noexcept override {
        return next_or_equal_key(0);
    }

    std::optional<std::pair<Node *, uint8_t>> last_key() const noexcept override {
        return prev_or_equal_key(Node::MaxChilds - 1);
    }

    Node *move() override { return new Node48(std::move(*this)); }
    Node *shrink() override;
    Node *grow() override;
};

template <typename Value> struct AdaptiveRadixTree<Value>::Node256 : public Node {
    friend struct Node48;
    static constexpr int SIZE = 256;
    std::array<Node *, SIZE> childs_{};
    uint8_t size_{};

    Node256(const Node256 &) = delete;
    Node256 &operator=(const Node256 &) = delete;
    explicit Node256(std::string_view prefix) : Node(prefix) {}
    Node256(Node256 &&node) noexcept : Node(std::move(node)), childs_(std::move(node.childs_)) {
        node.childs_.fill(nullptr);
    }
    Node256 &operator=(Node256 &&node) noexcept {
        std::swap(Node::prefix_, node.prefix_);
        std::swap(Node::leaf_, node.leaf_);
        std::swap(childs_, node.childs_);
        std::swap(size_, node.size_);
        return *this;
    }
    explicit Node256(Node48 &&node);
    ~Node256() {
        for (size_t i = 0; i < SIZE && Node::size() > 0; i++) {
            delete childs_[i];
        }
    }

    [[nodiscard]] bool is_full() const noexcept override { return Node::size() == SIZE; }
    [[nodiscard]] bool should_shrink() const noexcept override {
        return Node::size() <= Node48::SIZE;
    }
    bool insert(uint8_t key, Node *node) override {
        if (childs_[key] == nullptr) {
            childs_[key] = node;
            Node::size_ += 1;
            return true;
        }
        return false;
    }

    void remove(uint8_t key) override {
        if (childs_[key] != nullptr) {
            delete childs_[key];
            childs_[key] = nullptr;
            Node::size_ -= 1;
        }
    }

    Node **find(uint8_t key) override {
        if (childs_[key] == nullptr) {
            return nullptr;
        }
        return &childs_[key];
    }

    std::optional<std::pair<Node *, uint8_t>>
    next_or_equal_key(size_t key) const noexcept override {
        while (key < Node::MaxChilds && childs_[key] == nullptr) {
            key += 1;
        }
        if (key == Node::MaxChilds) {
            return std::nullopt;
        }
        return std::make_optional(std::make_pair(childs_[key], static_cast<uint8_t>(key)));
    }

    std::optional<std::pair<Node *, uint8_t>>
    prev_or_equal_key(size_t key) const noexcept override {
        while (key > 0 && childs_[key] == nullptr) {
            key -= 1;
        }
        if (key == 0 && childs_[key] == nullptr) {
            return std::nullopt;
        }
        return std::make_optional(std::make_pair(childs_[key], static_cast<uint8_t>(key)));
    }

    std::optional<std::pair<Node *, uint8_t>> first_key() const noexcept override {
        return next_or_equal_key(0);
    }

    std::optional<std::pair<Node *, uint8_t>> last_key() const noexcept override {
        return prev_or_equal_key(Node::MaxChilds - 1);
    }

    Node *move() override { return new Node256(std::move(*this)); }
    Node *shrink() override;
    Node *grow() override;
};

template <typename Value>
AdaptiveRadixTree<Value>::Node4::Node4(Node16 &&node) : Node4("") { // 16 -> 4
    assert(node.size_ <= SIZE);
    Node::prefix_ = std::move(node.prefix_);
    std::copy_n(node.keys_.begin(), std::min(SIZE, node.size()), keys_.begin());
    std::copy_n(node.childs_.begin(), std::min(SIZE, node.size()), childs_.begin());
    Node::size_ = node.size_;
    Node::leaf_ = std::move(node.leaf_);
    node.childs_.fill(nullptr);
    node.leaf_ = nullptr;
    node.size_ = 0;
}

template <typename Value> AdaptiveRadixTree<Value>::Node *AdaptiveRadixTree<Value>::Node4::grow() {
    return new Node16(std::move(*this));
}

template <typename Value>
AdaptiveRadixTree<Value>::Node *AdaptiveRadixTree<Value>::Node4::shrink() {
    assert(false);
    return nullptr;
}

template <typename Value>
AdaptiveRadixTree<Value>::Node16::Node16(Node4 &&node) : Node16("") { // 4 -> 16
    Node::prefix_ = std::move(node.prefix_);
    std::copy_n(node.keys_.begin(), std::min(SIZE, node.size()), keys_.begin());
    std::copy_n(node.childs_.begin(), std::min(SIZE, node.size()), childs_.begin());
    Node::size_ = node.size_;
    Node::leaf_ = std::move(node.leaf_);
    node.childs_.fill(nullptr);
    node.leaf_ = nullptr;
    node.size_ = 0;
}

template <typename Value>
AdaptiveRadixTree<Value>::Node16::Node16(Node48 &&node) : Node16("") { // 48 -> 16
    assert(node.size_ <= SIZE);
    Node::prefix_ = std::move(node.prefix_);
    size_t index = 0;
    for (size_t i = 0; i < Node::MaxChilds; i++) {
        Node **child = node.find(i);
        if (child) {
            keys_[index] = static_cast<uint8_t>(i);
            childs_[index] = *child;
            index += 1;
        }
    }
    Node::size_ = node.size_;
    Node::leaf_ = std::move(node.leaf_);
    node.childs_.fill(nullptr);
    node.leaf_ = nullptr;
    node.size_ = 0;
}

template <typename Value> AdaptiveRadixTree<Value>::Node *AdaptiveRadixTree<Value>::Node16::grow() {
    return new Node48(std::move(*this));
}

template <typename Value>
AdaptiveRadixTree<Value>::Node *AdaptiveRadixTree<Value>::Node16::shrink() {
    return new Node4(std::move(*this));
}

template <typename Value>
AdaptiveRadixTree<Value>::Node48::Node48(Node16 &&node) : Node48("") { // 16 -> 48
    Node::prefix_ = std::move(node.prefix_);
    for (size_t i = 0; i < node.size(); ++i) {
        keys_[node.keys_[i]] = static_cast<uint8_t>(i);
        childs_[i] = node.childs_[i];
    }
    Node::size_ = node.size_;
    Node::leaf_ = std::move(node.leaf_);
    node.childs_.fill(nullptr);
    node.leaf_ = nullptr;
    node.size_ = 0;
}

template <typename Value>
AdaptiveRadixTree<Value>::Node48::Node48(Node256 &&node) : Node48("") { // 256 -> 48
    assert(node.size_ <= SIZE);
    Node::prefix_ = std::move(node.prefix_);
    uint8_t index = 0;
    for (size_t key = 0; key < Node::MaxChilds; ++key) {
        if (childs_[key]) {
            keys_[key] = index;
            childs_[index] = childs_[key];
            index += 1;
        }
    }
    Node::size_ = node.size_;
    Node::leaf_ = std::move(node.leaf_);
    node.childs_.fill(nullptr);
    node.leaf_ = nullptr;
    node.size_ = 0;
}

template <typename Value> AdaptiveRadixTree<Value>::Node *AdaptiveRadixTree<Value>::Node48::grow() {
    return new Node256(std::move(*this));
}
template <typename Value>
AdaptiveRadixTree<Value>::Node *AdaptiveRadixTree<Value>::Node48::shrink() {
    return new Node16(std::move(*this));
}

template <typename Value>
AdaptiveRadixTree<Value>::Node256::Node256(Node48 &&node) : Node256("") { // 48 -> 256
    Node::prefix_ = std::move(node.prefix_);
    for (size_t key = 0; key < Node::MaxChilds; ++key) {
        Node **child = node.find(key);
        if (child) {
            childs_[key] = *child;
        }
    }
    Node::size_ = node.size_;
    Node::leaf_ = std::move(node.leaf_);
    node.childs_.fill(nullptr);
    node.leaf_ = nullptr;
    node.size_ = 0;
}

template <typename Value>
AdaptiveRadixTree<Value>::Node *AdaptiveRadixTree<Value>::Node256::grow() {
    assert(false);
    return nullptr;
}

template <typename Value>
AdaptiveRadixTree<Value>::Node *AdaptiveRadixTree<Value>::Node256::shrink() {
    return new Node256(std::move(*this));
}

template <typename Value> void AdaptiveRadixTree<Value>::insert(std::string_view key, Value value) {
    Node *current = root;
    Node **parent = &root;
    std::string_view remain = key;
    while (!key.empty()) {
        size_t match_prefix = current->match(key);
        // std::cerr << "commmon_prefix = "
        //     << std::string_view(key.data(), match_prefix)
        //     << " current_remaing = "
        //     << std::string_view(current->prefix_.data() + match_prefix,
        //                         current->prefix_.length() - match_prefix)
        //     << " key_remaing = "
        //     << std::string_view(key.data() + match_prefix,
        //                         key.length() - match_prefix)
        //     << std::endl;
        if (current->prefix_.length() > match_prefix) { // split at current node
            Node *new_node = current->move();
            current->prefix_ = new_node->prefix_.substr(0, match_prefix);
            new_node->prefix_ = new_node->prefix_.substr(match_prefix);

            current->prefix_ = current->prefix_.substr(0, match_prefix);
            current->insert(static_cast<uint8_t>(new_node->prefix_[0]), new_node);
            if (key.length() > match_prefix) {
                current->insert(static_cast<uint8_t>(key[match_prefix]),
                                new Node4{
                                    key.substr(match_prefix),
                                });
            }
        }

        key.remove_prefix(match_prefix);
        if (!key.empty() && current->find(key[0]) == nullptr) {
            if (current->is_full()) {
                *parent = current->grow();
                delete current;
                current = *parent;
            }
            current->insert(key[0], new Node4{key});
        }

        if (!key.empty()) {
            parent = current->find(key[0]);
            assert(parent != nullptr);
            current = *parent;
        }
    }
    if (current->leaf_) {
        current->leaf_->value = value;
    } else {
        current->leaf_ = new Leaf{remain, value};
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
        if (current->prefix_.length() > match_prefix) { // prefix not complete match
            return std::nullopt;
        }
        key.remove_prefix(match_prefix);
        if (key.empty()) { // prefix complete match and remaining key is empty
            break;
        }
        if (Node **next = current->find(key[0]); next) { // remaing key not next node to match
            current = *next;
        } else {
            return std::nullopt;
        }
    }
    if (current->leaf_ != nullptr) {
        return {current->leaf_->value};
    }
    return std::nullopt;
}

template <typename Value> bool AdaptiveRadixTree<Value>::remove(std::string_view key) {
    Node **current = &root;
    std::stack<Node **> stack;
    while (!key.empty()) {
        stack.push(current);
        size_t match_prefix = (*current)->match(key);
        // std::cerr << "commmon_prefix = "
        //           << std::string_view(key.data(), match_prefix)
        //           << " current_remaing = "
        //           << std::string_view((*current)->prefix_.data() + match_prefix,
        //                               (*current)->prefix_.length() - match_prefix)
        //           << " key_remaing = "
        //           << std::string_view(key.data() + match_prefix,
        //                               key.length() - match_prefix)
        //           << std::endl;
        if ((*current)->prefix_.length() > match_prefix) { // prefix not complete match
            return false;
        }
        key.remove_prefix(match_prefix);
        if (key.empty()) { // prefix complete match and remaining key is empty
            // std::cerr << "break\n";
            break;
        }
        if (current = (*current)->find(key[0]); !current) {
            return false;
        }
    }
    if ((*current)->leaf_ == nullptr) {
        return false;
    }

    delete (*current)->leaf_;
    (*current)->leaf_ = nullptr;
    if ((*current)->size() == 0) {
        assert(stack.size() >= 2);
        uint8_t k = (*current)->prefix_[0];
        stack.pop();
        (*stack.top())->remove(k);
        if ((*stack.top())->should_shrink()) {
            Node* new_node = (*stack.top())->shrink();
            delete (*stack.top());
            *stack.top() = new_node;
        }
    }

    while (true) {
        current = stack.top();
        stack.pop();
        if ((*current)->size() > 1 || current == &root || (*current)->leaf_) {
            break;
        }
        // std::cerr << (*current)->prefix_ << "\n";
        if (auto first_key = (*current)->first_key(); first_key) {
            auto *next = first_key->first;
            // std::cerr << "next " << next->prefix_ << "\n";

            next->prefix_ = (*current)->prefix_ + next->prefix_;
            (*current)->size_ = 0;
            delete (*current);
            *current = next;
        }
    }
    return true;
}

template <typename Value> void AdaptiveRadixTree<Value>::debug() {
    this->root->debug(0);
    std::cerr << "-----------------------------------\n";
}

template <typename Value> AdaptiveRadixTree<Value>::Iterator AdaptiveRadixTree<Value>::begin() {
    return Iterator{root};
}

template <typename Value> AdaptiveRadixTree<Value>::Iterator AdaptiveRadixTree<Value>::end() {
    return {};
}
