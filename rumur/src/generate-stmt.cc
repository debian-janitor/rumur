#include <cassert>
#include "generate.h"
#include <iostream>
#include <rumur/rumur.h>
#include <string>

using namespace rumur;

namespace {

class Generator : public ConstStmtTraversal {
 
 private:
  std::ostream *out;

 public:
  Generator(std::ostream &o): out(&o) { }
    
  void visit(const Assignment &s) final {

    if (!s.lhs->type()->is_simple())
      assert(!"TODO");

    const std::string lb = s.lhs->type()->lower_bound();
    const std::string ub = s.lhs->type()->upper_bound();

    *out << "handle_write(s, " << lb << ", " << ub << ", ";
    generate_lvalue(*out, *s.lhs);
    *out << ", ";
    generate_rvalue(*out, *s.rhs);
    *out << ")";
  }

  void visit(const Clear&) final {
    assert(!"TODO");
  }

  void visit(const ErrorStmt &s) final {
    *out << "error(s, false, \"" << s.message << "\")";
  }

  void visit(const For &s) final {
    generate_quantifier_header(*out, *s.quantifier);
    for (const std::shared_ptr<Stmt> &st : s.body) {
      *out << "  ";
      generate_stmt(*out, *st);
      *out << ";\n";
    }
    generate_quantifier_footer(*out, *s.quantifier);
  }

  void visit(const If &s) final {
    bool first = true;
    for (const IfClause &c : s.clauses) {
      if (!first)
        *out << "else ";
      if (c.condition != nullptr) {
        *out << "if (";
        generate_rvalue(*out, *c.condition);
        *out  << ") ";
      }
      *out << " {\n";
      for (const std::shared_ptr<Stmt> &st : c.body) {
        generate_stmt(*out, *st);
        *out << ";\n";
      }
      *out << "}\n";
      first = false;
    }
  }

  void visit(const ProcedureCall&) final {
    assert(!"TODO");
  }

  void visit(const PropertyStmt &s) final {
    switch (s.property.category) {

      case Property::DISABLED:
        *out << "do { } while (0)";
        break;

      case Property::ASSERTION:
        *out << "if (__builtin_expect(!";
        generate_property(*out, s.property);
        *out << ", 0)) {\nerror(s, false, \"" << s.message << "\");\n}";
        break;

      case Property::ASSUMPTION:
        *out << "if (__builtin_expect(!";
        generate_property(*out, s.property);
        *out
          << ", 0)) {\n"
          << "  assert(JMP_BUF_NEEDED && \"longjmping without a setup jmp_buf\");\n"
          << "  longjmp(checkpoint, 1);\n"
          << "}";
        break;

    }
  }

  void visit(const Return &s) final {
    *out << "return";

    if (s.expr != nullptr)
      assert(!"TODO");
  }

  void visit(const Undefine &s) final {
    *out << "handle_zero(";
    generate_lvalue(*out, *s.rhs);
    *out << ")";
  }

  virtual ~Generator() { }
};

}

void generate_stmt(std::ostream &out, const rumur::Stmt &s) {
  Generator g(out);
  g.dispatch(s);
}
