#pragma once
//#include <format>
#include <unordered_set>
#include <unordered_map>
#include "ast.h"
namespace EqP {


namespace out {

// deferred, originally intended for eg. arg order rearrangement
/*  
struct ArgInfo {
  int arg = -1; // index into Call::args; -1 = this slot's own index
  int prec = ast::kMinPrec; // == original DESIGN §6.3 SlotSpec::contextPrec
  bool needsAtom = false; // also wrap when !child.atomic  (the ^ base)
};*/


// prec is of the construct *as rendered*, not the
// input-side precedence: \frac{a}{b} is self-delimiting and needs no parens
// under * or -, whereas a/b does.
struct OutFunc {
  std::string_view name;
  int arity;
  // prec of the construct as rendered, may be different from
  // input-side prec for self bracketed args like \frac{a}{b} 
  int prec;
  //std::vector<ArgInfo> argInfo;
  std::vector<int> argPrecs;
  std::vector<std::string> affixes; // ["\frac{", "}{", "}"]
  bool atomic = true;
};

inline OutFunc newVerbatim(std::string_view name, int arity, bool inv) {
  std::vector<int> argPrecs(arity, ast::kMinPrec); 
  // single letter name default italic
  std::string pref = name.length() == 1?
    "\\mathit{" + std::string(name) : "\\mathrm{" + std::string(name);
  if (inv) pref += "^{-1}";
  pref += "}(";
  
  std::vector<std::string> affixes;
  if (arity == 0) { affixes.push_back(pref + ")"); }
  else {
    affixes.push_back(std::move(pref));
    for (int i = 1; i < arity; i++) { affixes.push_back(", "); }
    affixes.push_back(")");
  }
  return OutFunc{name, arity, ast::kAtomPrec, std::move(argPrecs), std::move(affixes)};
}

inline const std::unordered_map<std::string_view, OutFunc> outOpInfoMap = {
  // 11 operators including variants
  {"FracDiv", OutFunc{"FracDiv", 2, ast::kAtomPrec, {ast::kMinPrec, ast::kMinPrec}, {"\\frac{", "}{", "}"}, false}},
  // slash variant is not self-delimiting: denominator should outprec Div
  // otherwise eg. a/(b*c) can become a/b*c
  {"SlashDiv", OutFunc{"SlashDiv", 2, 20, {20, 21}, {"", "/", ""}}},
  {"CrossMul", OutFunc{"CrossMul", 2, 20, {20, 20}, {"", "\\times ", ""}}},
  {"ImplMul", OutFunc{"ImplMul", 2, 20, {20, 20}, {"", "", ""}}},
  {"DotMul", OutFunc{"DotMul", 2, 20, {20, 20}, {"", "\\cdot ", ""}}},
  {"Add", OutFunc{"Add", 2, 10, {10, 10}, {"", "+", ""}}},
  {"Sub", OutFunc{"Sub", 2, 10, {10, 11}, {"", "-", ""}}},
  {"Pow", OutFunc{"Pow", 2, 40, {41, ast::kMinPrec}, {"", "^{", "}"}}},
  {"Pos", OutFunc{"Pos", 1, 30, {30}, {"+", ""}}},
  {"Neg", OutFunc{"Neg", 1, 30, {30}, {"-", ""}}},
  {"Fact", OutFunc{"Fact", 1, 50, {50}, {"", "!"}}},
  // non-verbatim functions
  {"2Sqrt", OutFunc{"2Sqrt", 1, 40, {ast::kMinPrec}, {"\\sqrt{", "}"}}},
  {"NSqrt", OutFunc{"NSqrt", 2, 40, {ast::kMinPrec, ast::kMinPrec}, {"\\sqrt[", "]{", "}"}}},
  {"DefInt", OutFunc{"DefInt", 4, 40, {ast::kMinPrec, ast::kMinPrec, 40, 40}, {"\\int_{", "}^{", "}", "\\mathrm{d}", ""}}},
  // trailing space required: "\\int" + "f" is the reserved \intf
  {"IndefInt", OutFunc{"IndefInt", 2, 40, {40, 40}, {"\\int ", "\\mathrm{d}", ""}}},
  {"InlineDeriv", OutFunc{"IDeriv", 2, 40, {40, ast::kMinPrec}, {"\\frac{\\mathrm{d}", "}{\\mathrm{d}", "}"}}},
  {"OffsetDeriv", OutFunc{"ODeriv", 2, 10, {40, 40}, {"\\frac{\\mathrm{d}}{\\mathrm{d}", "}", ""}}},
  {"DefSum", OutFunc{"DefSum", 4, 20, {ast::kMinPrec, ast::kMinPrec, ast::kMinPrec, 20}, {"\\sum_{", "=", "}^{", "}", ""}}},
  {"IndefSum", OutFunc{"IndefSum", 1, 20, {20}, {"\\sum ", ""}}},
  {"binom", OutFunc{"Binom", 2, 40, {ast::kMinPrec, ast::kMinPrec}, {"\\binom{", "}{", "}"}}},
  // " \\to" + "a" is the control word \toa
  {"lim", OutFunc{"Lim", 3, 40, {ast::kMinPrec, ast::kMinPrec, 40}, {"\\displaystyle\\lim_{", " \\to ", "}", ""}}},
  {"sin", OutFunc{"sin", 1, 40, {41}, {"\\sin ", ""}}},
  {"cos", OutFunc{"cos", 1, 40, {41}, {"\\cos ", ""}}},
  {"tan", OutFunc{"tan", 1, 40, {41}, {"\\tan ", ""}}},
  {"asin", OutFunc{"asin", 1, 40, {41}, {"\\sin^{-1} ", ""}}},
  {"acos", OutFunc{"acos", 1, 40, {41}, {"\\cos^{-1} ", ""}}},
  {"atan", OutFunc{"atan", 1, 40, {41}, {"\\tan^{-1} ", ""}}},
  {"lg", OutFunc{"lg", 1, 40, {41}, {"\\lg ", ""}}},
  {"ln", OutFunc{"ln", 1, 40, {41}, {"\\ln ", ""}}},
  {"log", OutFunc{"tan", 2, 40, {ast::kMinPrec, 41}, {"\\log_{","}", ""}}},

  {"Eq", OutFunc{"Eq", 2, 1, {10, 10}, {"", "=", ""}}},

};

inline const std::unordered_set<std::string_view> greekSymbolSet = {
  "alpha", "beta", "gamma", "delta", "epsilon", 
  "zeta", "eta", "theta", "kappa", "iota",
  "lambda", "mu", "nu", "xi", /*omicron*/ 
  "pi", "rho",  "sigma", "tau", "upsilon",
  "phi", "chi", "psi", "omega",
  "Gamma", "Delta", "Theta", "Lambda",
  "Xi", "Pi", "Sigma", "Upsilon", 
  "Phi", "Psi", "Omega",
  "infty" //
};


struct ArityError: DiagError {
  ArityError(std::string message, Span where):
    DiagError({std::move(message), where}) {}
};


}}