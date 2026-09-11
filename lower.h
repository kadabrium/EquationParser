#pragma once
#include "ast.h"        
#include "parser.h"     
// factory module: parse tree -> AST
namespace EqP {



namespace lower {

// -- individual ASTNode construction --

// Leaves.  Number keeps the original text verbatim -- rewriting "1.50" to
// "1.5" is a semantic change the user did not ask for.
static inline ast::ASTNodePtr newNumber(std::string value, ast::Span s) {
  return std::make_unique<ast::ASTNode>(ast::ASTNode{
    ast::Number{value}, s});
}
static inline ast::ASTNodePtr newIdentifier(std::string name, ast::Span s) {
  return std::make_unique<ast::ASTNode>(
    ast::ASTNode{ast::Identifier{name}, s});
}

// Assert the op's Assoc matches the arity being built (operations.h stage 2),
// so a mis-dispatch in the fold fails at the construction site rather than
// surfacing as strange LaTeX much later.
static inline ast::ASTNodePtr newUnary(ast::ASTOp op, ast::ASTNodePtr operand, ast::Span s) {
  return std::make_unique<ast::ASTNode>(ast::ASTNode{
    ast::Unary{op, std::move(operand)}, s});
}
static inline ast::ASTNodePtr newBinary(ast::ASTOp op, ast::ASTNodePtr l, ast::ASTNodePtr r, ast::Span s) {
  return std::make_unique<ast::ASTNode>(ast::ASTNode{
    ast::Binary{op, std::move(l), std::move(r)}, s});
}

// Call takes the callee by value; `def` stays commented out until the
// registry exists (section 6.3), and resolution is a later pass anyway.
static inline ast::ASTNodePtr newCall(std::string callee, std::vector<ast::ASTNodePtr> args, ast::Span s, bool inv) {
  return std::make_unique<ast::ASTNode>(ast::ASTNode{
    ast::Call{std::move(callee), std::move(args), inv}
    , s});
}

// List has no grammar yet -- `[1,2]` is currently a parse error -- so this
// one is written now and exercised when matrices land.
static inline ast::ASTNodePtr newList(std::vector<ast::ASTNodePtr> elems, ast::Span s) {
  return std::make_unique<ast::ASTNode>(ast::ASTNode{ast::List{std::move(elems)}, s});
}


// -- span helpers --

// A Binary's span is lhs.begin..rhs.end, which no single parse-tree node carries.
static inline ast::Span mergeSpan(ast::Span a, ast::Span b) {
  return ast::Span{a.begin, b.end};
}

static inline ast::Span spanOf(const parse::ParseNode& pn) {
  // text_position carries a byte count alongside line/column
  return ast::Span{pn.begin.count, pn.end.count};
}


// -- ParseTree to AST conversion. Impl in .cpp --
ast::AST makeAST(const parse::ParseNode& root, const std::string& source);

//   For this milestone, failures throw (see stage 5).  Section 6.6's Diagnostic
//   vector and section 7's Result<T> replace that later; keeping it simple now
//   avoids standing up the expected<> shim before there is anything to report.

// -- dispatch ---------------------------------------------------------
ast::ASTNodePtr lowerNode(const parse::ParseNode& n);
ast::Span spanOf(const parse::ParseNode& n);

// -- fold ---------------------------------------------------------
struct Cursor;
ast::ASTNodePtr foldExpr(const parse::ParseNode& exprNode);
ast::ASTNodePtr climb(Cursor& cur, int minPrec);
ast::ASTNodePtr foldPrefix(Cursor& cur);
ast::ASTNodePtr foldPostfix(const parse::ParseNode& n);
ast::ASTNodePtr lowerCall(const parse::ParseNode& n);

// -- errors and limits ------------------------------------------------
struct LowerError : DiagError {
  LowerError(std::string message, ast::Span s):
    DiagError({std::move(message), s}) {}
};
//     Carries the offending span so the eventual Diagnostic (section 6.6) can
//     draw a caret without re-deriving position.

//   Almost nothing here can fail on a tree the grammar accepted; the realistic
//   cases are a trailing infix operator and an operator where an operand was
//   expected, both of which the grammar already rejects.  So the throw sites
//   are mostly assertions in disguise, and that is fine to say out loud.
//
//   Depth cap (section 6.1): the recursion is climb -> foldprefix -> lowerNode ->
//   foldExpr -> climb, one level per nested paren.  Enforce the cap of 64 by
//   carrying a depth counter in Cursor and throwing LowerError past it, which
//   turns a stack overflow on `((((...))))` into an ordinary error.  Cheaper and
//   more direct than PEGTL-side depth limiting, because the parse tree is
//   already built by the time we get here -- but note that means the *parser*
//   still needs its own guard, so this cap is a second line of defence rather
//   than the only one.

}}