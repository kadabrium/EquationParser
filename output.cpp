//#include <iostream>
#include <cctype>
#include <format>
//#include <ranges>
#include "output.h"
namespace EqP {


namespace out {



void Renderer::updateOptions() {
  this->ops[static_cast<std::size_t>(ast::ASTOp::Mul)] = mulType(opt.mult[0]);
  this->ops[static_cast<std::size_t>(ast::ASTOp::MulVar)] = mulType(opt.mult[1]);
  this->ops[static_cast<std::size_t>(ast::ASTOp::Div)] =
    (opt.div == DivStyle::Slash) ? "SlashDiv" : "FracDiv";
}

Renderer::Renderer(Options opt):
  opt{opt} {
  updateOptions();
}

std::string Renderer::render(ast::AST& tree) const {
  return renderNode(*(tree.root)).latex;
}

const OutFunc& Renderer::funcLookup(std::string_view name, int arity, Span where) const {
  if (name == "int") {
    if (arity != 2 && arity != 4) {
      throw ArityError(std::format(
        "int() takes 2 or 4 arguments, got {}", arity), where);
    }
    return arity == 2? outOpInfoMap.at("IndefInt") : outOpInfoMap.at("DefInt");
  }
  else if (name == "sum") {
    if (arity != 1 && arity != 4) {
      throw ArityError(std::format(
        "sum() takes 1 or 4 arguments, got {}", arity), where);
    }
    return arity == 1? outOpInfoMap.at("IndefSum") : outOpInfoMap.at("DefSum");
  }
  else if (name == "sqrt") {
    if (arity != 2 && arity != 1) {
      throw ArityError(std::format(
        "sqrt() takes 1 or 2 arguments, got {}", arity), where);
    }
    return arity == 2? outOpInfoMap.at("NSqrt") : outOpInfoMap.at("2Sqrt");
  }
  else if (name == "deriv") {
    const OutFunc& fn = this->opt.der == DerivStyle::Frac?
      outOpInfoMap.at("InlineDeriv") : outOpInfoMap.at("OffsetDeriv");
    if (arity != fn.arity) {
      throw ArityError(std::format(
        "{}() takes {} arguments, got {}", name, fn.arity, arity), where);
    }

    return fn;
  }
  else {
    const OutFunc& fn = outOpInfoMap.at(name); // catch not found in renderNode
    if (arity != fn.arity) {
      throw ArityError(std::format(
        "{}() takes {} arguments, got {}", name, fn.arity, arity), where);
    }
    return fn;
  }
}

// Bracket args in an arg list by looking ahead to the next arg/suffix.
// no parent when curr frag binds at least as tightly as the slot it is going into
// extra check if next op affix start with ^ (superscript/pow) or _ (subscript)
// in which case bracketing is only not needed if curr is atomic
std::string Renderer::wrapArgByPrec(const OutFragment& currArg, int contextPrec,
                                    bool sscript) const {
  if (currArg.prec >= contextPrec && (!sscript || currArg.atomic)) {
    return currArg.latex;
  }
  // incl. >= contextPrec && sscript && !atomic 
  else return bracket(currArg.latex);
}

bool Renderer::isTall(std::string_view latex) const {
  std::string tallOps[] = {"\\frac", "\\int", "\\sum", /*"\\prod", "\\sqrt", */"\\binom"};
  for (auto& op: tallOps) {
    if (latex.find(op) < latex.length()) {
      return true;}
  }
  return false;
}

// Bracket type choice (resizable vs raw) when bracketing is needed
std::string Renderer::bracket(std::string_view latex) const {
  const bool lr = (opt.sizing == Delimiter::LR)
                  || (opt.sizing == Delimiter::Auto && isTall(latex));
  return lr ? "\\left(" + std::string(latex) + "\\right)" : 
    "(" + std::string(latex) + ")";
}

bool Renderer::implMulRunOn(std::string_view latex) const {
  if (latex.empty()) { return false; } 
  const unsigned char c = static_cast<unsigned char>(latex.front());
  return std::isdigit(c) || isSign((char)(c));
}

bool Renderer::isSign(char c) const { 
  return c == '+' || c == '-'; 
}

bool Renderer::signRunOn(std::string_view latex) const {
  return !latex.empty() && isSign(latex.front());
}

OutFragment Renderer::renderOp(const OutFunc& f, std::span<const ast::ASTNode*> args) const {
  //assert(args.size() == f.argPrecs.size()); assert(args.size() + 1 == f.affixes.size());
  std::string res = f.affixes[0];

  if (f.name == "ODeriv") { // Offset deriv's arg order is opposite to in-numerator deriv
    std::swap(args[0], args[1]); 
  }

  for (size_t i = 0; i < args.size(); ++i) {
    const std::string& next = f.affixes[i+1];
    bool sscriptNext = !next.empty() && (next[0] == '^' || next[0] == '_');
    std::string text = wrapArgByPrec(renderNode(*args[i]), f.argPrecs[i], sscriptNext);
    // Juxtaposition seam: 
    // check if end of curr prefix causes a run-on with start of next part.
    const std::string& prev = f.affixes[i];
    // case 1: suffix leads with digit (and in an implicit mul)
    if (i > 0 && prev.empty() && implMulRunOn(text)) {
      res += outOpInfoMap.at("DotMul").affixes[1];
    }
    // case 2: prefix ends with operator while suffix starts with pos/neg
    if (!prev.empty() && isSign(prev.back()) && signRunOn(text)) {
      text = bracket(text);
    }
    res += text;
    res += next;
  }
  return OutFragment{std::move(res), f.prec, f.atomic};
}

OutFragment Renderer::renderNode(const ast::ASTNode& node) const {
  return std::visit(
    Renderer::RenderVariant{
      [this](const ast::Number& n) -> OutFragment {
        return OutFragment{n.value};
      },
      [this](const ast::Identifier& id) -> OutFragment {
        size_t subscrPos = id.name.find("_");
        if (subscrPos >= id.name.length()) {
          return OutFragment{
          (this->opt.greek && greekSymbolSet.contains(id.name))?
            "\\" + id.name + " " : id.name };
        }
        else {
          std::string prefix = id.name.substr(0, subscrPos); 
          std::string suffix = id.name.substr(subscrPos + 1);
          if (this->opt.greek) {
            return OutFragment{
              (greekSymbolSet.contains(prefix)? "\\" + prefix : prefix)
              + "_{"
              + (greekSymbolSet.contains(suffix)? "\\" + suffix : suffix) + "}"
            };
          }
          else {
            return OutFragment{
              prefix + "_{" + suffix + "}"
            };
          }  
        }
        
      },
      [this](const ast::Unary& u) -> OutFragment {
        const OutFunc& optor = outOpInfoMap.at(this->ops[(size_t)u.op]);
        const ast::ASTNode* argNodes[] = {u.operand.get()};
        return renderOp(optor, argNodes);
      },
      [this](const ast::Binary& b) -> OutFragment {
        const OutFunc& optor = outOpInfoMap.at(this->ops[(size_t)b.op]);
        const ast::ASTNode* argNodes[] = {b.left.get(), b.right.get()};
        return renderOp(optor, argNodes);
      },
      [this, &node](const ast::Call& call) -> OutFragment {
        OutFunc callee;
        try {
          // look back to node.span for arity error to locate the call
          callee = funcLookup(call.callee, (int)(call.args.size()), node.span);
          // inv not available for funcs with special representations
          if (call.inv) throw ArityError(
            "inverse superscript not available for special notation functions (use asin, etc. for trig)", node.span);
        }
        catch (std::out_of_range&) {
          callee = newVerbatim(call.callee, (int)(call.args.size()), call.inv);
        }
        std::vector<const ast::ASTNode*> argNodes;
        for (const auto& a : call.args) { 
          argNodes.push_back(a.get()); 
        }
        return renderOp(callee, argNodes);
      },
      [this](const ast::List& list) -> OutFragment {
        std::string res;
        for (std::size_t i = 0; i < list.nodes.size(); ++i) {
          if (i != 0) { res += ", "; }
          res += renderNode(*list.nodes[i]).latex;
        }
        return OutFragment{std::move(res), ast::kMinPrec, false};
      }
    }, node.kind);
  }


// ---- option setters ----

Renderer& Renderer::setOptions(const Options& options) {
  opt = options;
  updateOptions();
  return *this;
}

Renderer& Renderer::setParenPolicy(ParenPolicy parens) {
  opt.parens = parens;
  return *this;
}

Renderer& Renderer::setMulStyles(MulStyle regular, MulStyle variant) {
  opt.mult[0] = regular;
  opt.mult[1] = variant;
  updateOptions();
  return *this;
}

Renderer& Renderer::setPrimaryMulStyle(MulStyle style) {
  opt.mult[0] = style;
  this->ops[static_cast<std::size_t>(ast::ASTOp::Mul)] = mulType(style);
  return *this;
}

Renderer& Renderer::setVariantMulStyle(MulStyle style) {
  opt.mult[1] = style;
  this->ops[static_cast<std::size_t>(ast::ASTOp::MulVar)] = mulType(style);
  return *this;
}

Renderer& Renderer::setDivStyle(DivStyle divStyle) {
  opt.div = divStyle;
  this->ops[static_cast<std::size_t>(ast::ASTOp::Div)] =
    (divStyle == DivStyle::Slash) ? "SlashDiv" : "FracDiv";
  return *this;
}

Renderer& Renderer::setDerivStyle(DerivStyle derivStyle) {
  opt.der = derivStyle;
  return *this;
}

Renderer& Renderer::setGreek(bool enabled) {
  opt.greek = enabled;
  return *this;
}

Renderer& Renderer::setMultiChar(MultiChar mode) {
  opt.multiChar = mode;
  return *this;
}

Renderer& Renderer::setDelimiter(Delimiter delimiter) {
  opt.sizing = delimiter;
  return *this;
}

Renderer& Renderer::setEmbedMode(EmbedMode mode) {
  opt.output = mode;
  return *this;
}

Renderer& Renderer::setAutoCopy(bool enabled) {
  opt.autoCopy = enabled;
  return *this;
}

}
}
