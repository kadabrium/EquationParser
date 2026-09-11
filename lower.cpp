//#include <iostream>
#include "lower.h"
namespace EqP {
using namespace ast; 
using namespace parse;

namespace lower {

// -- output from parse tree ---------------------------
//     x                     ROOT(x)
//     -x                    ROOT(expr(pre_neg x))
//     --x                   ROOT(expr(pre_neg pre_neg x))
//     n!                    ROOT(postfixed(n post_fact))
//     -n!                   ROOT(expr(pre_neg postfixed(n post_fact)))
//     -x^2                  ROOT(expr(pre_neg x op_pow 2))
//     a+b*c^d               ROOT(expr(a op_add b op_mul c op_pow d))
//     (a+b)*c               ROOT(expr(paren(expr(a op_add b)) op_mul c))
//     f(x)                  ROOT(call(f arglist(x)))
//     g()                   ROOT(call(g))
//     integrate(x+y,x,1,2)  ROOT(call(integrate arglist(expr(x op_add y) x 1 2)))

//     * expr no longer means "infix chain".  It is a flat stream that may
//       contain prefix operators, infix operators and operands in any legal
//       arrangement, and it survives even with a single operand eg. -x.


// -- fold ----------------------
struct Cursor { 
  const ParseNode& pnExpr; 
  std::size_t i = 0; // idx in ops list of expr node
};

// expr node (prefix + infix)
// Entry: builds a Cursor and calls climb(cur, kMinPrec), then asserts the
// cursor is exhausted.  A leftover item means the grammar admitted a shape
// the fold does not model.
ASTNodePtr foldExpr(const ParseNode& expr) {
  Cursor it = Cursor{expr, 0}; 
  auto res = climb(it, kMinPrec);

  if (it.i != expr.children.size()) {
    throw LowerError{"expression folding terminated unexpectedly", spanOf(expr)};
  }
  return res;
}

// Consumes a leading run of prefix operators, through a mutual recursive loop
// with climb(it, precedence(pre)), and only then moves on to infixes. That
// single detail is what gives -x^2 = neg(pow(x, 2)) (Pow 40 >= Neg 30, so 
// power is absorbed) while -a*b = mul(neg(a), b) (Mul 20 < Neg 30, so it is
// not).  Multiple prefixes nest right-to-left: --x = -(-x).
ASTNodePtr /*operand*/foldPrefix(Cursor& it) {
  const std::vector<std::unique_ptr<ParseNode>>& ops = it.pnExpr.children;
  assert(it.i < ops.size());
  std::optional<ASTOp> pre = prefixFromParse(shortType(ops[it.i]->type));
  // if no prefix, init one operand and jump to climb's infix section
  if (!pre) {                       
    auto n = lowerNode(*ops[it.i]);
    ++it.i;
    return n;
  }
  // else, memo one prefix at a time and send remainder mutually recursive to climb()
  // which will call this back while there are remaining prefixes
  const Span opSpan = spanOf(*ops[it.i]);
  ++it.i;
  ASTNodePtr inner = climb(it, precedence(*pre));  
  const Span s = mergeSpan(opSpan, inner->span);
  return newUnary(*pre, std::move(inner), s);
}

// Takes one operand via operand(), then while the next item is an infix (binary)
// operator whose precedence >= minPrec, consumes it and recurses 
ASTNodePtr climb(Cursor& it, int minPrec) {
  const std::vector<std::unique_ptr<ParseNode>>& ops = it.pnExpr.children;
  // entry of whole expr here, check if any prefixes remain
  ASTNodePtr acc = foldPrefix(it); 
  while (it.i < ops.size()) {
    // while acc is returned with prefixes remaining, this check jumps back
    std::optional<ASTOp> op = infixFromParse(shortType(ops[it.i]->type));
    if (!op || precedence(*op) < minPrec) break;
    // if check passes, start processing infixes
    Assoc assoc = ast::assoc(*op);
    it.i++;
    // +1 for left associativity makes a-b-c group as (a-b)-c and a^b^c as a^(b^c).
    ASTNodePtr nextRight = climb(it, precedence(*op) + (assoc == Assoc::Left? 1 : 0));
    const Span s = mergeSpan(acc->span, nextRight->span/* == spanOf(*ops[it.i-1])*/);
    acc = newBinary(*op, std::move(acc), std::move(nextRight), s);
  }
  return acc;
}


// postfix node 
// postfix children are [operand, post_op...].  Wrap left to right, so
// n!! = (n!)!. No precedence logic needed, the grammar already
// decided postfix binds tightest by putting this layer below expr.
ASTNodePtr foldPostfix(const ParseNode& postfix) {
  auto& ops = postfix.children;
  auto acc = lowerNode(*ops[0]);
  for (size_t i = 1; i < ops.size(); i++) {
    const auto op = postfixFromParse(shortType(ops[i]->type)); assert(op);
    // access should be explicitly before a move, not inline
    const Span s = mergeSpan(acc->span, spanOf(*ops[i]));  
    // declared outside, inline move is ok as long as no parallel access
    acc = newUnary(*op, std::move(acc), s);
  }
  return acc;
}

// function call node 
// arity or name checking delegated to ::out
ASTNodePtr lowerCall(const ParseNode& pnCall) {
  std::string callee = std::string(pnCall.children[0]->data);
  bool hasInv = (pnCall.children.size() >= 2 && shortType(pnCall.children[1]->type) == "inv_sup");

  // zero-argument call
  if (pnCall.children.size() == 1 || (pnCall.children.size() == 2 && hasInv)) {
    return newCall(callee, std::vector<ASTNodePtr>(), spanOf(pnCall), hasInv); 
  }
  // normal call
  auto& arglist = hasInv? *(pnCall.children[2]) : *(pnCall.children[1]);
  std::vector<ASTNodePtr> args;
  for (size_t i = 0; i < arglist.children.size(); i++) {
    args.push_back(lowerNode(*(arglist.children[i])));
  }
  return newCall(callee, std::move(args), spanOf(pnCall), hasInv);
}


// -- dispatch --
ASTNodePtr lowerNode(const ParseNode& pn) {
  std::string_view nodeKind = shortType(pn.type);
  if (nodeKind == "number") {
    return newNumber(std::string(pn.data), spanOf(pn));
  }
  if (nodeKind == "identifier") {
    return newIdentifier(std::string(pn.data), spanOf(pn));
  }
  if (nodeKind == "expr") {
    return foldExpr(pn);
  }
  if (nodeKind == "postfixed") {
    return foldPostfix(pn);
  }
  if (nodeKind == "call") {
    return lowerCall(pn);
  }
  if (nodeKind == "paren") {
    //return lowerNode(*(pn.children[0]));
    auto inner = lowerNode(*pn.children[0]);
    inner->parenthesized = true;
    // cover the delimiters
    inner->span = spanOf(pn);          
    return inner;
  }
  throw LowerError{"unhandled parse node kind '" + std::string(nodeKind) + "'", spanOf(pn)};
 // ASTNodePtr n(nullptr); 
 // assert(n.get()); return n;
}

// -- export --------------
AST makeAST(const parse::ParseNode& root, const std::string& source) {
  assert(root.is_root());
  // ROOT is dummy, data begins at children[0]
  ASTNodePtr astRoot = lowerNode(*root.children[0]);
  return AST(std::move(astRoot), source);

}




} // EqP::lower
} // EqP