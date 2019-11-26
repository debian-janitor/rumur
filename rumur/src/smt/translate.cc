#include <cstddef>
#include <rumur/rumur.h>
#include "except.h"
#include <locale>
#include "logic.h"
#include "../options.h"
#include "translate.h"
#include <sstream>
#include <string>

using namespace rumur;

namespace smt {

namespace { class Translator : public ConstExprTraversal {

 private:
  std::ostringstream buffer;

 public:
  std::string str() const {
    return buffer.str();
  }

  Translator &operator<<(const std::string &s) {
    buffer << s;
    return *this;
  }

  Translator &operator<<(const Expr &e) {
    dispatch(e);
    return *this;
  }

  void visit_add(const Add &n) {
    *this << "(" << add() << " " << *n.lhs << " " << *n.rhs << ")";
  }

  void visit_and(const And &n) {
    *this << "(and " << *n.lhs << " " << *n.rhs << ")";
  }

  void visit_element(const Element &n) {
    *this << "(select " << *n.array << " " << *n.index << ")";
  }

  void visit_exprid(const ExprID &n) {
    *this << mangle(n.id, n.value->unique_id);
  }

  void visit_eq(const Eq &n) {
    *this << "(= " << *n.lhs << " " << *n.rhs << ")";
  }

  void visit_exists(const Exists &n) {
    throw Unsupported(n);
  }

  void visit_div(const Div &n) {
    *this << "(" << div() << " " << *n.lhs << " " << *n.rhs << ")";
  }

  void visit_field(const Field &n) {
    // the record type that forms the root of this expression will have
    // previously been defined as a synthesised type (see define-records.cc)

    // determine the (mangled) SMT name the root was given
    const Ptr<TypeExpr> type = n.record->type()->resolve();
    const std::string root = mangle("", type->unique_id);

    // we can now compute the accessor for this field (see define-records.cc)
    const std::string getter = root + "_" + n.field;

    // now construct the SMT expression
    *this << "(" << getter << " " << *n.record << ")";
  }

  void visit_forall(const Forall &n) {
    throw Unsupported(n);
  }

  void visit_functioncall(const FunctionCall &n) {
    throw Unsupported(n);
  }

  void visit_geq(const Geq &n) {
    *this << "(" << geq() << " " << *n.lhs << " " << *n.rhs << ")";
  }

  void visit_gt(const Gt &n) {
    *this << "(" << gt() << " " << *n.lhs << " " << *n.rhs << ")";
  }

  void visit_implication(const Implication &n) {
    *this << "(=> " << *n.lhs << " " << *n.rhs << ")";
  }

  void visit_isundefined(const IsUndefined &n) {
    throw Unsupported(n);
  }

  void visit_leq(const Leq &n) {
    *this << "(" << leq() << " " << *n.lhs << " " << *n.rhs << ")";
  }

  void visit_lt(const Lt &n) {
    *this << "(" << lt() << " " << *n.lhs << " " << *n.rhs << ")";
  }

  void visit_mod(const Mod &n) {
    *this << "(" << mod() << " " << *n.lhs << " " << *n.rhs << ")";
  }

  void visit_mul(const Mul &n) {
    *this << "(" << mul() << " " << *n.lhs << " " << *n.rhs << ")";
  }

  void visit_negative(const Negative &n) {
    *this << "(" << neg() << " " << *n.rhs << ")";
  }

  void visit_neq(const Neq &n) {
    *this << "(not (= " << *n.lhs << " " << *n.rhs << "))";
  }

  void visit_number(const Number &n) {
    *this << numeric_literal(n.value);
  }

  void visit_not(const Not &n) {
    *this << "(not " << *n.rhs << ")";
  }

  void visit_or(const Or &n) {
    *this << "(or " << *n.lhs << " " << *n.rhs << ")";
  }

  void visit_sub(const Sub &n) {
    *this << "(" << sub() << " " << *n.lhs << " " << *n.rhs << ")";
  }

  void visit_ternary(const Ternary &n) {
    *this << "(ite " << *n.cond << " " << *n.lhs << " " << *n.rhs << ")";
  }
}; }

std::string translate(const Expr &expr) {
  Translator t;
  t.dispatch(expr);
  return t.str();
}

static std::string lower(const std::string &s) {
  std::string s1;
  for (char c : s)
    s1 += tolower(c);
  return s1;
}

std::string mangle(const std::string &s, size_t id) {

  // if you're debugging a bad translation to SMT, you can change this to `true`
  // to get the Murphi name of a symbol output as a comment in the SMT problem
  const std::string suffix = false ? "; " + s + "\n" : "";

  const std::string l = lower(s);

  // if this is a boolean literal, the solver already knows of it
  if (l == "true" || l == "false")
    return l + suffix;

  // if this is the boolean type, the solver already knows of it
  if (l == "boolean")
    return "Bool" + suffix;

  // otherwise synthesise a node-unique name for this
  return "s" + std::to_string(id) + suffix;
}

}
