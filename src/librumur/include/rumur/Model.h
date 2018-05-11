#pragma once

#include <cstdint>
#include <iostream>
#include "location.hh"
#include <rumur/Decl.h>
#include <rumur/Node.h>
#include <rumur/Rule.h>
#include <vector>

namespace rumur {

class Model : public Node {

 public:
  std::vector<Decl*> decls;
  std::vector<Rule*> rules;

  Model() = delete;
  Model(std::vector<Decl*> &&decls_, std::vector<Rule*> &&rules_, const location &loc_);
  Model(const Model &other);
  Model &operator=(Model other);
  friend void swap(Model &x, Model &y) noexcept;
  virtual ~Model();
  Model *clone() const final;

  // Get the size of the state data in bits.
  uint64_t size_bits() const;

  void generate(std::ostream &out) const;
  bool operator==(const Node &other) const final;
};

static inline std::ostream &operator<<(std::ostream &out, const Model &m) {
  m.generate(out);
  return out;
}

}
