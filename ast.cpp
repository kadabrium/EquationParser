// operations on ASTs with variant-visit
#include <algorithm> 
#include "ast.h"
namespace EqP {


namespace ast {

namespace {   
  int precVariant(const Unary& u)  { return precedence(u.op); }
  int precVariant(const Binary& b) { return precedence(b.op); }
  int precVariant(const auto&) { return kAtomPrec; } // num, id, list, call
}
int ASTNode::precedence() const {
  return this->visit([](const auto& kind) -> int {
    return precVariant(kind);
  });
}

namespace {   
  std::string strVariant(const Unary& u)  { 
    std::string res = std::string(opInfo(u.op).name); 
    res += '(';
    res += (u.operand)->toStringln();
    res += ')';
    return res;
  }
  std::string strVariant(const Binary& b)  { 
    std::string res = std::string(opInfo(b.op).name); 
    res += '(';
    res += ((b.left)->toStringln() + ' ' + (b.right)->toStringln());
    res += ')';
    return res;
  }
  std::string strVariant(const Call& c)  { 
    std::string res = c.callee;
    res += '(';
    int len = c.args.size();
    for (int i = 0; i < len; i++) {
      res += c.args[i]->toStringln();
      if (i < len-1) res += ' ';
    }
    res += ')';
    return res;
  }
  std::string strVariant(const List& l)  { 
    std::string res = "LIST(";
    int len = l.nodes.size();
    for (int i = 0; i < len; i++) {
      res += l.nodes[i]->toStringln();
      if (i < len-1) res += " ";
    }
    res += ')';
    return res;
  }
  std::string strVariant(const Number& num) { return num.value; } 
  std::string strVariant(const Identifier& id) { return id.name; } 
}
std::string ASTNode::toStringln() const {
  // raw visit() cross check
  return std::visit(
    [](const auto& k) -> std::string { return strVariant(k); }, 
    this->kind);
}

// Structural equality *ignoring spans*.  Needed because spans differ
// between an expression and the same expression with extra whitespace,
// and every AST-level test wants to compare shape only.
namespace {
  bool eqAll(const std::vector<ASTNodePtr>& x, const std::vector<ASTNodePtr>& y) {
    if (x.size() != y.size()) return false;
    for (std::size_t i = 0; i < x.size(); ++i) if (!x[i]->equals(*y[i])) return false;
    return true;
  }
  bool eqVariant(const Unary& a, const Unary& b) { 
    return a.op == b.op && a.operand->equals(*(b.operand)); 
  }
  bool eqVariant(const Binary& a, const Binary& b) { 
    return a.op == b.op && a.left->equals(*b.left) && a.right->equals(*b.right); 
  }
  bool eqVariant(const Call& a, const Call& b) { 
    return a.callee == b.callee && eqAll(a.args, b.args); 
  }
  bool eqVariant(const List& a, const List& b) { 
    return eqAll(a.nodes, b.nodes); 
  }
  bool eqVariant(const Number& n1, const Number& n2) {
    return n1.value == n2.value;
  }
  bool eqVariant(const Identifier& i1, const Identifier& i2) {
    return i1.name == i2.name;
  }
}
bool ASTNode::equals(const ASTNode& other) const {
  if (this->kind.index() != other.kind.index()) return false;
  return visit([&](const auto& a) -> bool {
    // cast to only compare same types
    return eqVariant(a, std::get<std::decay_t<decltype(a)>>(other.kind));
  });
}


// Indented multi-line form, mirroring parse::printTree's 
// layout so a parse tree and the AST it lowered to can be diffed by eye.
// This is also the only place span and 'parenthesized' are observable
namespace {
  std::string labelVariant(const Unary& u)      { return std::string(opInfo(u.op).name); }
  std::string labelVariant(const Binary& b)     { return std::string(opInfo(b.op).name); }
  std::string labelVariant(const Call& c)       { return "Call  \"" + c.callee + '"'; }
  std::string labelVariant(const List&)         { return "LIST"; }
  std::string labelVariant(const Number& n)     { return "Number  \"" + n.value + '"'; }
  std::string labelVariant(const Identifier& i) { return "Identifier  \"" + i.name + '"'; }
  // concrete struct visit cross check
  struct kidsVariant {
    // No auto fallback; unregistered node kind should fail to compile here 
    // rather than silently dump as a leaf, the one place it would go unnoticed.
    void operator()(const Unary& u, std::ostream& os, int indent) {
      u.operand->printTree(os, indent);
    }
    void operator()(const Binary& b, std::ostream& os, int indent) {
      b.left->printTree(os, indent);
      b.right->printTree(os, indent);
    }
    void operator()(const Call& c, std::ostream& os, int indent) {
      for (const auto& a : c.args) a->printTree(os, indent);
    }
    void operator()(const List& l, std::ostream& os, int indent) {
      for (const auto& n : l.nodes) n->printTree(os, indent);
    }
    void operator()(const Number&, std::ostream&, int) {}
    void operator()(const Identifier&, std::ostream&, int) {}
    };
}
void ASTNode::printTree(std::ostream& os, int indent) const {
  os << std::string(indent * 2, ' ')
     << this->visit([](const auto& k) -> std::string { return labelVariant(k); })
     << "  [" << this->span.begin << ',' << this->span.end << ')';
  if (this->parenthesized) { os << "  (parens)"; }
  os << '\n';

  this->visit([&](const auto& k) { kidsVariant{}(k, os, indent + 1); });
}


namespace {
  // inline raw visit() cross check
  // struct inheriting multiple lambdas 
  template <typename... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  // map construction args to template type args
  template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;
}
size_t ASTNode::depthUnder() const {
  return std::visit(overloaded{
    [](const Number&) -> size_t {
      return 0;
    },
    [](const Identifier&) -> size_t {
      return 0;
    },
    [](const Unary& u) -> size_t {
      return u.operand->depthUnder() + 1;
    },
    [](const Binary& b) -> size_t {
      return std::max(b.left->depthUnder(), b.right->depthUnder()) + 1;
    },
    [](const Call& c) -> size_t {
      size_t res = 0;
      for (auto& a: c.args) {
        res = std::max(res, a->depthUnder());
      }
      return res + 1;
    },
    [](const List& l) -> size_t {
      size_t res = 0;
      for (auto& i: l.nodes) {
        res = std::max(res, i->depthUnder());
      }
      return res + 1;
    },
  }, this->kind);
}

std::string AST::toStringln() const {
  return "AST " + this->root->toStringln();
}

}}

/*
namespace {
  // The single place that knows each alternative's child layout.  Structural
  // passes go through this instead of respelling the six cases each time.
  template <class F>
  void eachChild(const ASTNode& n, F&& f) {
    n.visit([&](const auto& k) {
      using K = std::decay_t<decltype(k)>;
      if      constexpr (std::is_same_v<K, Unary>)  { f(*k.operand); }
      else if constexpr (std::is_same_v<K, Binary>) { f(*k.left); f(*k.right); }
      else if constexpr (std::is_same_v<K, Call>)   { for (const auto& a : k.args)  f(*a); }
      else if constexpr (std::is_same_v<K, List>)   { for (const auto& c : k.nodes) f(*c); }
      else static_assert(std::is_same_v<K, Number> || std::is_same_v<K, Identifier>,
                         "eachChild: unhandled node kind");
    });
  }
}
  void ASTNode::printTree(std::ostream& os, int indent) const {
  os << std::string(indent * 2, ' ')
     << this->visit([](const auto& k) -> std::string { return labelVariant(k); })
     << "  [" << span.begin << ',' << span.end << ')';
  if (parenthesized) os << "  (parens)";
  os << '\n';
  eachChild(*this, [&](const ASTNode& c) { c.printTree(os, indent + 1); });
}

size_t ASTNode::depthUnder() const {
  size_t d = 0;
  eachChild(*this, [&](const ASTNode& c) { d = std::max(d, c.depthUnder() + 1); });
  return d;
}

*/