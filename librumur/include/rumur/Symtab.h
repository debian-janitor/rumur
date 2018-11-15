#pragma once

#include <cassert>
#include <cstddef>
#include "location.hh"
#include <memory>
#include <rumur/except.h>
#include <rumur/Node.h>
#include <rumur/Ptr.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace rumur {

class Symtab {

 private:
  std::vector<std::unordered_map<std::string, Ptr<Node>>> scope;

 public:
  void open_scope() {
    scope.emplace_back();
  }

  void close_scope() {
    assert(!scope.empty());
    scope.pop_back();
  }

  void declare(const std::string &name, const Ptr<Node> &value) {
    assert(!scope.empty());
    scope.back()[name] = value;
  }

  void declare(const std::string &name, const std::shared_ptr<Node> &value) {
    declare(name, Ptr<Node>(value->clone()));
  }

  template<typename U>
  std::shared_ptr<U> lookup(const std::string &name, const location &loc) {
    for (auto it = scope.rbegin(); it != scope.rend(); it++) {
      auto it2 = it->find(name);
      if (it2 != it->end()) {
        if (auto ret = dynamic_cast<U*>(it2->second.get())) {
          return std::shared_ptr<U>(ret->clone());
        } else {
          break;
        }
      }
    }
    throw Error("unknown symbol: " + name, loc);
  }

  // Whether we are in the top-level scope.
  bool is_global_scope() const {
    return scope.size() == 1;
  }

};

}
