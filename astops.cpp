#include "astops.h"
namespace EqP {
  

namespace ast {

  
  const OpInfo& opInfo(ASTOp op) {
    return ASTOpInfoMap[(size_t)op];
  }
  int precedence(ASTOp op) {
    return ASTOpInfoMap[(size_t)op].prec;
  }
  Assoc assoc(ASTOp op) {
    return ASTOpInfoMap[(size_t)op].assoc;
  }
  bool isPrefix(ASTOp op) {
    return ASTOpInfoMap[(size_t)op].assoc == Assoc::Prefix;
  }
  bool isPostfix(ASTOp op) {
    return ASTOpInfoMap[(size_t)op].assoc == Assoc::Postfix;
  }
  bool isInfix(ASTOp op) {
    return ASTOpInfoMap[(size_t)op].assoc == Assoc::Left 
      || ASTOpInfoMap[(size_t)op].assoc == Assoc::Right;
  }

  std::optional<ASTOp> infixFromParse(std::string_view shortParseType) {
    if (shortParseType == "op_add") { return ASTOp::Add; }
    else if (shortParseType == "op_sub") { return ASTOp::Sub; }
    else if (shortParseType == "op_mul") { return ASTOp::Mul; }
    else if (shortParseType == "op_mulv") { return ASTOp::MulVar; }
    else if (shortParseType == "op_div") { return ASTOp::Div; }
    else if (shortParseType == "op_pow") { return ASTOp::Pow; }

    else if (shortParseType == "op_eq") { return ASTOp::Eq; }
    else return std::nullopt;
  }
  std::optional<ASTOp> prefixFromParse(std::string_view shortParseType) {
    if (shortParseType == "pre_pos") { return ASTOp::Pos; }
    else if (shortParseType == "pre_neg") { return ASTOp::Neg; }
    else return std::nullopt;
  }
  std::optional<ASTOp> postfixFromParse(std::string_view shortParseType) {
    if (shortParseType == "post_fact") { return ASTOp::Fact; }
    else return std::nullopt;
  }


}

}