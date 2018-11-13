#include <cassert>
#include <cstddef>
#include <iostream>
#include "location.hh"
#include <memory>
#include <rumur/Decl.h>
#include <rumur/except.h>
#include <rumur/Expr.h>
#include <rumur/Property.h>
#include <rumur/Ptr.h>
#include <rumur/Rule.h>
#include <rumur/Stmt.h>
#include <rumur/traverse.h>
#include <string>
#include "utils.h"
#include <vector>

namespace rumur {

namespace {
  /* A traversal pass that checks any return statements within a rule do not
   * have a trailing expression.
   */
  class ReturnChecker : public ConstTraversal {

   public:
    /* Avoid recursing into functions, that may have return statements with an
     * expression.
     */
    void visit(const Function&) final { }
    void visit(const FunctionCall&) final { }
    void visit(const ProcedureCall&) final { }

    void visit(const Return &n) final {
      if (n.expr != nullptr)
        throw Error("return statement in rule or startstate returns a value",
          n.loc);

      // No need to recurse into the return statement's child.
    }

    static void check(const Node &n) {
      ReturnChecker c;
      c.dispatch(n);
    }
  };
}

Rule::Rule(const std::string &name_, const location &loc_):
  Node(loc_), name(name_) { }

Rule::Rule(const Rule &other):
  Node(other), name(other.name), quantifiers(other.quantifiers) {
  for (const std::shared_ptr<AliasDecl> &a : other.aliases)
    aliases.emplace_back(a->clone());
}

std::vector<Ptr<Rule>> Rule::flatten() const {
  return { Ptr<Rule>(clone()) };
}

AliasRule::AliasRule(std::vector<std::shared_ptr<AliasDecl>> &&aliases_,
  const std::vector<Ptr<Rule>> &rules_, const location &loc_):
  Rule("", loc_), rules(rules_) {

  aliases = aliases_;
}

AliasRule::AliasRule(const AliasRule &other):
  Rule(other), rules(other.rules) { }

AliasRule &AliasRule::operator=(AliasRule other) {
  swap(*this, other);
  return *this;
}

void swap(AliasRule &x, AliasRule &y) noexcept {
  using std::swap;
  swap(x.loc, y.loc);
  swap(x.unique_id, y.unique_id);
  swap(x.name, y.name);
  swap(x.quantifiers, y.quantifiers);
  swap(x.aliases, y.aliases);
  swap(x.rules, y.rules);
}

AliasRule *AliasRule::clone() const {
  return new AliasRule(*this);
}

bool AliasRule::operator==(const Node &other) const {
  auto o = dynamic_cast<const AliasRule*>(&other);
  if (o == nullptr)
    return false;
  if (name != o->name)
    return false;
  if (quantifiers != o->quantifiers)
    return false;
  if (!vector_eq(aliases, o->aliases))
    return false;
  if (!vector_eq(rules, o->rules))
    return false;
  return true;
}

std::vector<Ptr<Rule>> AliasRule::flatten() const {
  std::vector<Ptr<Rule>> rs;
  for (const Ptr<Rule> &r : rules) {
    for (Ptr<Rule> &f : r->flatten()) {
      for (const std::shared_ptr<AliasDecl> &a : aliases)
        f->aliases.insert(f->aliases.begin(),
          std::shared_ptr<AliasDecl>(a->clone()));
      rs.push_back(f);
    }
  }
  return rs;
}

SimpleRule::SimpleRule(const std::string &name_, std::shared_ptr<Expr> guard_,
  std::vector<std::shared_ptr<Decl>> &&decls_,
  const std::vector<Ptr<Stmt>> &body_, const location &loc_):
  Rule(name_, loc_), guard(guard_), decls(decls_), body(body_) { }

SimpleRule::SimpleRule(const SimpleRule &other):
  Rule(other), guard(other.guard == nullptr ? nullptr : other.guard->clone()),
  body(other.body) {
  for (const std::shared_ptr<Decl> &d : other.decls)
    decls.emplace_back(d->clone());
}

SimpleRule &SimpleRule::operator=(SimpleRule other) {
  swap(*this, other);
  return *this;
}

void swap(SimpleRule &x, SimpleRule &y) noexcept {
  using std::swap;
  swap(x.loc, y.loc);
  swap(x.unique_id, y.unique_id);
  swap(x.name, y.name);
  swap(x.decls, y.decls);
  swap(x.body, y.body);
  swap(x.quantifiers, y.quantifiers);
  swap(x.aliases, y.aliases);
}

SimpleRule *SimpleRule::clone() const {
  return new SimpleRule(*this);
}

bool SimpleRule::operator==(const Node &other) const {
  auto o = dynamic_cast<const SimpleRule*>(&other);
  if (o == nullptr)
    return false;
  if (name != o->name)
    return false;
  if (quantifiers != o->quantifiers)
    return false;
  if (!vector_eq(aliases, o->aliases))
    return false;
  if (guard == nullptr) {
    if (o->guard != nullptr)
      return false;
  } else {
    if (o->guard == nullptr || *guard != *o->guard)
      return false;
  }
  if (!vector_eq(decls, o->decls))
    return false;
  if (!vector_eq(body, o->body))
    return false;
  return true;
}

void SimpleRule::validate() const {
  ReturnChecker::check(*this);
}

StartState::StartState(const std::string &name_,
  std::vector<std::shared_ptr<Decl>> &&decls_,
  const std::vector<Ptr<Stmt>> &body_, const location &loc_):
  Rule(name_, loc_), decls(decls_), body(body_) { }

StartState::StartState(const StartState &other):
  Rule(other), body(other.body) {
  for (const std::shared_ptr<Decl> &d : other.decls)
    decls.emplace_back(d->clone());
}

StartState &StartState::operator=(StartState other) {
  swap(*this, other);
  return *this;
}

void swap(StartState &x, StartState &y) noexcept {
  using std::swap;
  swap(x.loc, y.loc);
  swap(x.unique_id, y.unique_id);
  swap(x.name, y.name);
  swap(x.decls, y.decls);
  swap(x.body, y.body);
  swap(x.quantifiers, y.quantifiers);
  swap(x.aliases, y.aliases);
}

StartState *StartState::clone() const {
  return new StartState(*this);
}

bool StartState::operator==(const Node &other) const {
  auto o = dynamic_cast<const StartState*>(&other);
  if (o == nullptr)
    return false;
  if (name != o->name)
    return false;
  if (quantifiers != o->quantifiers)
    return false;
  if (!vector_eq(aliases, o->aliases))
    return false;
  if (!vector_eq(decls, o->decls))
    return false;
  if (!vector_eq(body, o->body))
    return false;
  return true;
}

void StartState::validate() const {
  ReturnChecker::check(*this);
}

PropertyRule::PropertyRule(const std::string &name_, const Property &property_,
  const location &loc_):
  Rule(name_, loc_), property(property_) { }

PropertyRule::PropertyRule(const PropertyRule &other):
  Rule(other), property(other.property) { }

PropertyRule &PropertyRule::operator=(PropertyRule other) {
  swap(*this, other);
  return *this;
}

void swap(PropertyRule &x, PropertyRule &y) noexcept {
  using std::swap;
  swap(x.loc, y.loc);
  swap(x.unique_id, y.unique_id);
  swap(x.name, y.name);
  swap(x.property, y.property);
  swap(x.quantifiers, y.quantifiers);
  swap(x.aliases, y.aliases);
}

PropertyRule *PropertyRule::clone() const {
  return new PropertyRule(*this);
}

bool PropertyRule::operator==(const Node &other) const {
  auto o = dynamic_cast<const PropertyRule*>(&other);
  if (o == nullptr)
    return false;
  if (name != o->name)
    return false;
  if (quantifiers != o->quantifiers)
    return false;
  if (!vector_eq(aliases, o->aliases))
    return false;
  if (property != o->property)
    return false;
  return true;
}

Ruleset::Ruleset(const std::vector<Quantifier> &quantifiers_,
  const std::vector<Ptr<Rule>> &rules_, const location &loc_):
  Rule("", loc_), rules(rules_) {
  quantifiers = quantifiers_;
}

Ruleset::Ruleset(const Ruleset &other):
  Rule(other), rules(other.rules) { }

Ruleset &Ruleset::operator=(Ruleset other) {
  swap(*this, other);
  return *this;
}

void swap(Ruleset &x, Ruleset &y) noexcept {
  using std::swap;
  swap(x.loc, y.loc);
  swap(x.unique_id, y.unique_id);
  swap(x.name, y.name);
  swap(x.quantifiers, y.quantifiers);
  swap(x.aliases, y.aliases);
  swap(x.rules, y.rules);
}

Ruleset *Ruleset::clone() const {
  return new Ruleset(*this);
}

bool Ruleset::operator==(const Node &other) const {
  auto o = dynamic_cast<const Ruleset*>(&other);
  if (o == nullptr)
    return false;
  if (name != o->name)
    return false;
  if (quantifiers != o->quantifiers)
    return false;
  if (!vector_eq(aliases, o->aliases))
    return false;
  if (!vector_eq(rules, o->rules))
    return false;
  return true;
}

std::vector<Ptr<Rule>> Ruleset::flatten() const {
  std::vector<Ptr<Rule>> rs;
  for (const Ptr<Rule> &r : rules) {
    for (Ptr<Rule> &f : r->flatten()) {
      for (const Quantifier &q : quantifiers)
        f->quantifiers.insert(f->quantifiers.begin(), q);
      rs.push_back(f);
    }
  }
  return rs;
}

}
