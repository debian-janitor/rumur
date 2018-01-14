#include <iostream>
#include "location.hh"
#include <rumur/Decl.h>
#include <rumur/except.h>
#include <rumur/Expr.h>
#include <rumur/Indexer.h>
#include <rumur/Node.h>
#include <string>

namespace rumur {

Decl::Decl(const std::string &name, const location &loc)
  : Node(loc), name(name) {
}

Decl::~Decl() {
}

ConstDecl::ConstDecl(const std::string &name, const Expr *value,
  const location &loc, Indexer&)
  : Decl(name, loc), value(value->clone()) {
}

ConstDecl::ConstDecl(const ConstDecl &other):
  Decl(other), value(other.value->clone()) {
}

ConstDecl &ConstDecl::operator=(ConstDecl other) {
    swap(*this, other);
    return *this;
}

void swap(ConstDecl &x, ConstDecl &y) noexcept {
    using std::swap;
    swap(x.loc, y.loc);
    swap(x.name, y.name);
    swap(x.value, y.value);
}

ConstDecl *ConstDecl::clone() const {
    return new ConstDecl(*this);
}

void ConstDecl::validate() const {
    value->validate();
    if (!value->constant())
        throw RumurError("const definition is not a constant", value->loc);
}

void ConstDecl::define(std::ostream &out) const {
    out << "static int64_t model_" << name << "(const State*s __attribute__((unused))){return ";
    value->rvalue(out);
    out << ";}";
}

ConstDecl::~ConstDecl() {
    delete value;
}

TypeDecl::TypeDecl(const std::string &name, TypeExpr *value,
  const location &loc, Indexer&)
  : Decl(name, loc), value(value) {
}

TypeDecl::TypeDecl(const TypeDecl &other):
  Decl(other), value(other.value->clone()) {
}

TypeDecl &TypeDecl::operator=(TypeDecl other) {
    swap(*this, other);
    return *this;
}

void swap(TypeDecl &x, TypeDecl &y) noexcept {
    using std::swap;
    swap(x.loc, y.loc);
    swap(x.name, y.name);
    swap(x.value, y.value);
}

TypeDecl *TypeDecl::clone() const {
    return new TypeDecl(*this);
}

void TypeDecl::validate() const {
    value->validate();
}

void TypeDecl::define(std::ostream &out) const {
    value->define(out);
}

TypeDecl::~TypeDecl() {
    delete value;
}

VarDecl::VarDecl(const std::string &name, TypeExpr *type,
  const location &loc, Indexer&)
  : Decl(name, loc), type(type) {
}

VarDecl::VarDecl(const VarDecl &other):
  Decl(other), type(other.type->clone()), local(other.local) {
}

VarDecl &VarDecl::operator=(VarDecl other) {
    swap(*this, other);
    return *this;
}

void swap(VarDecl &x, VarDecl &y) noexcept {
    using std::swap;
    swap(x.loc, y.loc);
    swap(x.name, y.name);
    swap(x.type, y.type);
    swap(x.local, y.local);
}

VarDecl *VarDecl::clone() const {
    return new VarDecl(*this);
}

void VarDecl::define(std::ostream&) const {
    // TODO
}

VarDecl::~VarDecl() {
    delete type;
}

}
