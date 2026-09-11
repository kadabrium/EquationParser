// EquationParser -- the operator table.  See DESIGN.md section 3.2.
//
// Single source of truth for precedence and associativity.  Both the fold
// (lower, builds the tree) and the renderer (section 6.4, decides where
// parentheses go) read this table and nothing else, which is what keeps
// "add an operator" a one-row data change rather than a grammar change.
#pragma once 
//#include <memory>
//#include <ostream>
#include <array>
//#include <vector>
//#include <unordered_map>
//#include <string>
#include <string_view>
#include <optional>
namespace EqP {


namespace ast {


enum class ASTOp { 
  Add = 0, 
  Sub, Mul, MulVar,
  Div, Pow, Neg, Pos, Fact,
  Eq 
};

enum class Assoc {
  Left, Right, Prefix, Postfix 
};

struct OpInfo {
  ASTOp opcode;
  std::string_view lexeme; // "+", "^" diagnostics and round-tripping
  std::string_view name; 
  int prec;
  // Add/Sub 10, Mul/Div 20, Neg/Pos 30, Pow 40, Fact 50
  Assoc assoc;
};
// leaf without parens
inline constexpr int kAtomPrec = 100;   
// fold entry, and self-delimiting LaTeX groups (\frac{...}, ^{})
inline constexpr int kMinPrec  = 0;  

inline constexpr std::array<OpInfo, 10> ASTOpInfoMap = {
  OpInfo{ASTOp::Add, "+", "Add", 10, Assoc::Left}, {ASTOp::Sub, "-","Sub", 10, Assoc::Left},
  {ASTOp::Mul, "*", "Mul", 20, Assoc::Left}, {ASTOp::MulVar, "**", "MulVar", 20, Assoc::Left}, 
  {ASTOp::Div, "/","Div", 20, Assoc::Left},
  {ASTOp::Pow, "^", "Pow", 40, Assoc::Right}, {ASTOp::Neg, "-","Neg", 30, Assoc::Prefix},
  {ASTOp::Pos, "+", "Pos",30, Assoc::Prefix}, {ASTOp::Fact, "!","Fact", 50, Assoc::Postfix},

  {ASTOp::Eq, "=", "Eq", kMinPrec, Assoc::Left}
};
static_assert([]{ 
  for (std::size_t i = 0; i < ASTOpInfoMap.size(); ++i) {
    if (static_cast<std::size_t>(ASTOpInfoMap[i].opcode) != i) return false;
  }
  return true; }(), "Order mismatch between ASTOpInfoMap and ASTOp enum");


const OpInfo& opInfo(ASTOp op);
int precedence(ASTOp op);
Assoc assoc(ASTOp op);
bool isPrefix(ASTOp op);  
bool isPostfix(ASTOp op);  
bool isInfix(ASTOp op);

// rule-name -> ASTOp 
//   The parse tree identifies operators by demangled rule type -- ParseNode::type
//   is "EqP::grammar::op_add".  Lowering needs the reverse map.  
std::optional<ASTOp> infixFromParse(std::string_view shortParseType);
std::optional<ASTOp> prefixFromParse(std::string_view shortParseType);
std::optional<ASTOp> postfixFromParse(std::string_view shortParseType);
//     Each takes the *short* type ("op_add", "pre_neg") and returns the enum,
//     or nullopt if the node is an operand rather than an operator


// -- deferred: reverse direction ------------------------------------
//   `lexeme` above is already what a plain-text printer would need, but the
//   LaTeX spelling of an operator (`\cdot`, `\frac`, `!`) is *not* a property
//   of the operator alone -- it depends on Options (MultStyle, DivStyle) and on
//   the operands.  That belongs in the renderer, not here.  
//   Resist adding a `latex` field to OpInfo...

} // ast

}