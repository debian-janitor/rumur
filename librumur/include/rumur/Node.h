#pragma once

#include <climits>
#include <cstddef>
#include <iostream>
#include "location.hh"
#include <utility>
#include <vector>

namespace rumur {

struct Node {

  location loc;
  size_t unique_id = SIZE_MAX;

  Node() = delete;
  Node(const location &loc_);
  virtual ~Node() = default;

  virtual Node *clone() const = 0;

  virtual bool operator==(const Node &other) const = 0;
  bool operator!=(const Node &other) const {
    return !(*this == other);
  }

  /* Confirm that data structure invariants hold. This function throws
   * rumur::Errors if invariants are violated.
   */
  virtual void validate() const { }

  // Iteration-supporting infrastructure. You do not need to pay much attention
  // to the following details, but their purpose is to allow you to use C++11
  // range-based for loops on AST nodes:
  //
  //   for (Node *n : myAST) {
  //     ...
  //   }
  //
  // The traversal order is not guaranteed, so do not use this API if you
  // specifically require pre-order or post-order.

  class PreorderIterator {

   private:
    std::vector<Node*> remaining;

   public:
    explicit PreorderIterator();
    explicit PreorderIterator(Node &base);
    PreorderIterator &operator++();
    PreorderIterator operator++(int);
    Node *operator*();
    bool operator==(const PreorderIterator &other) const;
    bool operator!=(const PreorderIterator &other) const;
  };

  using Iterator = PreorderIterator;

  Iterator begin();
  Iterator end();

  class ConstPreorderIterator {

   private:
    std::vector<const Node*> remaining;

   public:
    explicit ConstPreorderIterator();
    explicit ConstPreorderIterator(const Node &base);
    ConstPreorderIterator &operator++();
    ConstPreorderIterator operator++(int);
    const Node *operator*();
    bool operator==(const ConstPreorderIterator &other) const;
    bool operator!=(const ConstPreorderIterator &other) const;
  };

  using ConstIterator = ConstPreorderIterator;

  ConstIterator begin() const;
  ConstIterator end() const;

  class PreorderWrapper {

   private:
    Node &root;

   public:
    explicit PreorderWrapper(Node &root_);
    PreorderIterator begin();
    PreorderIterator end();
  };

  // API for explicit pre-order traversal. Use as:
  //
  //   for (Node *n : myAST.preorder()) {
  //     ...
  //   }
  PreorderWrapper preorder();

  class ConstPreorderWrapper {

   private:
    const Node &root;

   public:
    explicit ConstPreorderWrapper(const Node &root_);
    ConstPreorderIterator begin();
    ConstPreorderIterator end();
  };

  // API for explicit pre-order traversal on a const Node
  ConstPreorderWrapper preorder() const;

  class PostorderIterator {

   private:
    // pending nodes and whether they have been expanded or not
    std::vector<std::pair<Node*, bool>> remaining;

    void expand_head();

   public:
    explicit PostorderIterator();
    explicit PostorderIterator(Node &root);
    PostorderIterator &operator++();
    PostorderIterator operator++(int);
    Node *operator*();
    bool operator==(const PostorderIterator &other) const;
    bool operator!=(const PostorderIterator &other) const;
  };

  class PostorderWrapper {

   private:
    Node &root;

   public:
    explicit PostorderWrapper(Node &root_): root(root_) { }
    PostorderIterator begin() { return PostorderIterator(root); }
    PostorderIterator end() { return PostorderIterator(); }
  };

  // API for explicit post-order traversal
  PostorderWrapper postorder() { return PostorderWrapper(*this); }

  class ConstPostorderIterator {

   private:
    std::vector<std::pair<const Node*, bool>> remaining;

    void expand_head();

   public:
    explicit ConstPostorderIterator();
    explicit ConstPostorderIterator(const Node &root);
    ConstPostorderIterator &operator++();
    ConstPostorderIterator operator++(int);
    const Node *operator*();
    bool operator==(const ConstPostorderIterator &other) const;
    bool operator!=(const ConstPostorderIterator &other) const;
  };

  class ConstPostorderWrapper {

   private:
    const Node &root;

   public:
    explicit ConstPostorderWrapper(const Node &root_): root(root_) { }
    ConstPostorderIterator begin() { return ConstPostorderIterator(root); }
    ConstPostorderIterator end() { return ConstPostorderIterator(); }
  };

  ConstPostorderWrapper postorder() const { return ConstPostorderWrapper(*this); }
};

}
