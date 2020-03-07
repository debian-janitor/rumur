#include <cassert>
#include <cstddef>
#include "CLikeGenerator.h"
#include "generate_c.h"
#include <gmpxx.h>
#include <iostream>
#include "resources.h"
#include <rumur/rumur.h>
#include <string>
#include <utility>
#include <vector>

using namespace rumur;

namespace {

class CGenerator : public CLikeGenerator {

 public:
  CGenerator(std::ostream &out_, bool pack_): CLikeGenerator(out_, pack_) { }

  void visit_constdecl(const ConstDecl &n) final {
    *this << indentation() << "const ";
    if (n.type == nullptr) {
      *this << "int64_t";
    } else {
      *this << *n.type;
    }
    *this << " " << n.name << " = " << *n.value << ";\n";
  }

  void visit_function(const Function &n) final {
    *this << indentation();
    if (n.return_type == nullptr) {
      *this << "void";
    } else {
      *this << *n.return_type;
    }
    *this << " " << n.name << "(";
    bool first = true;
    for (const Ptr<VarDecl> &p : n.parameters) {
      if (!first) {
        *this << ", ";
      }
      *this << *p->type << " ";
      // if this is a var parameter, it needs to be a pointer
      if (!p->readonly) {
        *this << "*" << p->name << "_";
      } else {
        *this << p->name;
      }
      first = false;
    }
    *this << ") {\n";
    indent();
    // provide aliases of var parameters under their original name
    for (const Ptr<VarDecl> &p : n.parameters) {
      if (!p->readonly) {
        *this << "#define " << p->name << " (*" << p->name << "_)\n";
      }
    }
    for (const Ptr<Decl> &d : n.decls) {
      *this << *d;
    }
    for (const Ptr<Stmt> &s : n.body) {
      *this << *s;
    }
    // clean up var aliases
    for (const Ptr<VarDecl> &p : n.parameters) {
      if (!p->readonly) {
        *this << "#undef " << p->name << "\n";
      }
    }
    dedent();
    *this << "}\n";
  }

  void visit_model(const Model &n) final {

    // constants, types and variables
    for (const Ptr<Decl> &d : n.decls) {
      *this << *d;
    }

    *this << "\n";

    // functions and procedures
    for (const Ptr<Function> &f : n.functions) {
      *this << *f << "\n";
    }

    // flatten the rules so we do not have to deal with the hierarchy of
    // rulesets, aliasrules, etc.
    std::vector<Ptr<Rule>> flattened;
    for (const Ptr<Rule> &r : n.rules) {
      std::vector<Ptr<Rule>> rs = r->flatten();
      flattened.insert(flattened.end(), rs.begin(), rs.end());
    }

    // startstates, rules, invariants
    for (const Ptr<Rule> &r : flattened) {
      *this << *r << "\n";
    }
  }

  void visit_propertyrule(const PropertyRule &n) final {

    // function prototype
    *this << indentation() << "bool " << n.name << "(";

    // parameters
    bool first = true;
    for (const Quantifier &q : n.quantifiers) {
      if (!first) {
        *this << ", ";
      }
      if (auto t = dynamic_cast<const TypeExprID*>(q.type.get())) {
        *this << t->name;
      } else {
        *this << "int64_t";
      }
      *this << " " << q.name;
      first = false;
    }

    *this << ") {\n";
    indent();

    // any aliases this property uses
    for (const Ptr<AliasDecl> &a : n.aliases) {
      *this << *a;
    }

    *this << indentation() << "return " << *n.property.expr << ";\n";

    // clean up any aliases we defined
    for (const Ptr<AliasDecl> &a : n.aliases) {
      *this << "#undef " << a->name << "\n";
    }

    dedent();
    *this << "}\n";
  }

  void visit_quantifier(const Quantifier &n) final {

    if (n.type == nullptr) {
      bool down_count = n.from->constant() && n.to->constant()
        && n.to->constant_fold() < n.from->constant_fold();
      *this << "for (int64_t " << n.name << " = " << *n.from << "; " << n.name
        << " " << (down_count ? ">=" : "<=") << " " << *n.to << "; " << n.name
        << " += ";
      if (n.step == nullptr) {
        *this << "1";
      } else {
        *this << *n.step;
      }
      *this << ")";
      return;
    }

    const Ptr<TypeExpr> resolved = n.type->resolve();

    if (auto e = dynamic_cast<const Enum*>(resolved.get())) {
      if (e->members.empty()) {
        // degenerate loop
        *this << "for (int " << n.name << " = 0; " << n.name << " < 0; "
          << n.name << "++)";
      } else {
        // common case
        *this << "for (__typeof__(" << e->members[0].first << ") " << n.name
          << " = " << e->members[0].first << "; " << n.name << " <= "
          << e->members[e->members.size() - 1].first << "; " << n.name << "++)";
      }
      return;
    }

    if (auto r = dynamic_cast<const Range*>(resolved.get())) {
      *this << "for (int64_t " << n.name << " = " << *r->min << "; " << n.name
        << " <= " << *r->max << "; " << n.name << "++)";
      return;
    }

    if (auto s = dynamic_cast<const Scalarset*>(resolved.get())) {
      *this << "for (int64_t " << n.name << " = 0; " << n.name << " <= "
        << *s->bound << "; " << n.name << "++)";
      return;
    }

    assert(!"missing case in visit_quantifier()");
  }

  void visit_simplerule(const SimpleRule &n) final {
    *this << indentation() << "bool guard_" << n.name << "(";

    // parameters
    bool first = true;
    for (const Quantifier &q : n.quantifiers) {
      if (!first) {
        *this << ", ";
      }
      if (auto t = dynamic_cast<const TypeExprID*>(q.type.get())) {
        *this << t->name;
      } else {
        *this << "int64_t";
      }
      *this << " " << q.name;
      first = false;
    }

    *this << ") {\n";
    indent();

    // any aliases that are defined in an outer scope
    for (const Ptr<AliasDecl> &a : n.aliases) {
      *this << *a;
    }

    *this << indentation() << "return ";
    if (n.guard == nullptr) {
      *this << "true";
    } else {
      *this << *n.guard;
    }
    *this << ";\n";

    // clean up aliases
    for (const Ptr<AliasDecl> &a : n.aliases) {
      *this << "#undef " << a->name << "\n";
    }

    dedent();
    *this << indentation() << "}\n\n";

    *this << indentation() << "void rule_" << n.name << "(";

    // parameters
    first = true;
    for (const Quantifier &q : n.quantifiers) {
      if (!first) {
        *this << ", ";
      }
      if (auto t = dynamic_cast<const TypeExprID*>(q.type.get())) {
        *this << t->name;
      } else {
        *this << "int64_t";
      }
      *this << " " << q.name;
      first = false;
    }

    *this << ") {\n";
    indent();

    // aliases, variables, local types, etc.
    for (const Ptr<AliasDecl> &a : n.aliases) {
      *this << *a;
    }
    for (const Ptr<Decl> &d : n.decls) {
      *this << *d;
    }

    for (const Ptr<Stmt> &s : n.body) {
      *this << *s;
    }

    // clean up any aliases we defined
    for (const Ptr<Decl> &d : n.decls) {
      if (auto a = dynamic_cast<const AliasDecl*>(d.get())) {
        *this << "#undef " << a->name << "\n";
      }
    }
    for (const Ptr<AliasDecl> &a : n.aliases) {
      *this << "#undef " << a->name << "\n";
    }

    dedent();
    *this << indentation() << "}\n";
  }

  void visit_startstate(const StartState &n) final {
    *this << indentation() << "void startstate_" << n.name << "(";

    // parameters
    bool first = true;
    for (const Quantifier &q : n.quantifiers) {
      if (!first) {
        *this << ", ";
      }
      if (auto t = dynamic_cast<const TypeExprID*>(q.type.get())) {
        *this << t->name;
      } else {
        *this << "int64_t";
      }
      *this << " " << q.name;
      first = false;
    }

    *this << ") {\n";
    indent();

    // aliases, variables, local types, etc.
    for (const Ptr<AliasDecl> &a : n.aliases) {
      *this << *a;
    }
    for (const Ptr<Decl> &d : n.decls) {
      *this << *d;
    }

    for (const Ptr<Stmt> &s : n.body) {
      *this << *s;
    }

    // clean up any aliases we defined
    for (const Ptr<Decl> &d : n.decls) {
      if (auto a = dynamic_cast<const AliasDecl*>(d.get())) {
        *this << "#undef " << a->name << "\n";
      }
    }
    for (const Ptr<AliasDecl> &a : n.aliases) {
      *this << "#undef " << a->name << "\n";
    }

    dedent();
    *this << indentation() << "}\n\n";
  }

  void visit_vardecl(const VarDecl &n) final {
    *this << indentation() << *n.type << " " << n.name << ";\n";
  }

  virtual ~CGenerator() = default;
};

}

void generate_c(const Node &n, bool pack, std::ostream &out) {

  // write the static prefix to the beginning of the source file
  for (size_t i = 0; i < resources_c_prefix_c_len; i++)
    out << (char)resources_c_prefix_c[i];

  CGenerator gen(out, pack);
  gen.dispatch(n);
}
