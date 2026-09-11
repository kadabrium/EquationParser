// EquationParser -- the abstract syntax tree
// The node set is closed and small: everything below is already the final
// shape.  What is missing is the machinery around it -- construction, spans,
// traversal, and a printable form -- which is what lower.h and the renderer
// will consume.
#pragma once
#include <variant>
#include <vector>
#include <memory>
#include <ostream>
#include <string>
//#include <string_view>
#include "astops.h"
#include "diagnostic.h"
namespace EqP {


namespace ast{

// Span is EqP::Span -- a range in the source text, shared with the parse and
// render stages (diagnostic.h).  Aliased here so ast::Span keeps working.
using EqP::Span;

struct ASTNode;
using ASTNodePtr = std::unique_ptr<ASTNode>;

// ------ node types ----------
struct Number { std::string value; }; // verbatim, never reformatted
struct Identifier { std::string name; };
struct Unary { 
  ASTOp op; 
  ASTNodePtr operand; 
};
struct Binary { 
  ASTOp op; 
  ASTNodePtr left;
  ASTNodePtr right; 
}; // no distinction between pre/in/suffix at this level
struct Call { 
  std::string callee; 
  std::vector<ASTNodePtr> args;
  bool inv = false;
};   
struct List { 
  std::vector<ASTNodePtr> nodes; 
};

struct ASTNode {
  std::variant<Number, Identifier, Unary, Binary, Call, List> kind;
  Span span;
  bool parenthesized = false;
public:
  int precedence() const;
  size_t depthUnder() const;
  std::string toStringln() const;
  void printTree(std::ostream& os, int indent = 0) const;
  bool equals(const ASTNode& other) const;

  template <typename Fn> decltype(auto) visit(Fn&& accessor) const {
    return std::visit(std::forward<Fn>(accessor), this->kind);
  }
  template <typename Fn> decltype(auto) visit(Fn&& accessor) {
    return std::visit(std::forward<Fn>(accessor), this->kind);
  }
};

struct AST {
  ASTNodePtr root;
  std::string source; // for caret rendering in diagnostics
public:
  AST(ASTNodePtr root, const std::string& source):
    root(std::move(root)), source(source) {  
  }
  std::string toStringln() const;
};


// -- what is deliberately absent -------------------------------------
//
//   No arena, no string_view payloads -- section 6.2 and section 11.  An arena
//   would buy pointer stability and bulk allocation for a few hundred nodes;
//   string_views would alias AST::source and make an AST uncopyable and unable
//   to outlive its text.  Owning std::string costs allocations nobody can
//   measure at these input sizes and removes the whole lifetime hazard.
//
//   No simplification, no evaluation, no canonicalisation.  `b+a` stays `b+a`
//   (section 5.2) -- that is the entire reason this is not a CAS front end.
//
// ==============================================================================


}}