// EquationParser -- the LaTeX backend
// The renderer is a single bottom-up walk.  Each node produces one OutFragment
// describing the LaTeX it rendered to *and* how that LaTeX behaves as a child,
// and the parent decides from the fragment alone whether to bracket it.
// Fragments are transient: one per live stack frame, concatenated into the
// parent and discarded.  Nothing is ever stored back onto a node.
#pragma once
#include <array>
#include <cassert>
//#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include "outops.h"
#include "options.h"
namespace EqP {


namespace out {

struct OutFragment {
  std::string latex;
  int prec = ast::kAtomPrec; // precedence of the outermost construct as rendered
  bool atomic = true; // safe as a ^ base without parentheses
};

class Renderer {
  std::array<std::string_view, 10> ops = {
  "Add", "Sub",                      
  // pick two for Mul in Options
  "ImplMul", "DotMul", //"CrossMul",  
  // pick one for Div 
  "FracDiv", //"SlashDiv",          
  "Pow", "Neg", "Pos", "Fact",
  "Eq"         
  };
  std::string_view mulType(MulStyle s) {
    switch (s) {
      case MulStyle::Implicit: return "ImplMul";
      case MulStyle::Cdot: return "DotMul";
      case MulStyle::Times: return "CrossMul";
    }
    return "ImplMul";
  }
  void updateOptions();
  OutFragment renderNode(const ast::ASTNode& node) const;
  OutFragment renderOp(const OutFunc& f, std::span<const ast::ASTNode*> args) const;
  const OutFunc& funcLookup(std::string_view name, int arity, Span where) const;
  template <typename... Ts> struct RenderVariant : Ts... { using Ts::operator()...; };
  template <typename... Ts> RenderVariant(Ts...) -> RenderVariant<Ts...>;
  
  // bracketing edge case helpers
  std::string wrapArgByPrec(const OutFragment& child, int contextPrec,
                            bool scriptFollows = false) const;
  std::string bracket(std::string_view latex) const;
  bool isTall(std::string_view latex) const;
  bool isSign(char c) const;
  bool signRunOn(std::string_view latex) const;
  bool implMulRunOn(std::string_view latex) const;

public:
  Options opt;
  Renderer(Options opt);
  std::string render(ast::AST& tree) const; // entry

  Renderer& setOptions(const Options& options);
  Renderer& setParenPolicy(ParenPolicy parens);
  Renderer& setMulStyles(MulStyle regular, MulStyle variable);
  Renderer& setPrimaryMulStyle(MulStyle style);
  Renderer& setVariantMulStyle(MulStyle style);
  Renderer& setDivStyle(DivStyle divStyle);
  Renderer& setDerivStyle(DerivStyle derivStyle);
  Renderer& setGreek(bool enabled);
  Renderer& setMultiChar(MultiChar mode);
  Renderer& setDelimiter(Delimiter delimiter);
  Renderer& setEmbedMode(EmbedMode mode);
  Renderer& setAutoCopy(bool enabled);

};


}
}
