// EquationParser -- rendering options.  See DESIGN.md section 6.7.
#pragma once
namespace EqP {

enum class MulStyle {
  Implicit,
  Cdot,
  Times
}; // pick 2 to map * and **

enum class DivStyle {
  Frac,
  Slash
};

enum class ParenPolicy {
  Minimal,
  Verbatim
};

enum class EmbedMode {
  Raw,
  Inline, // $
  Display // []
};

enum class MultiChar {
  Italic,
  Raw
};

// scoped: unscoped `Auto`/`Never` at namespace scope collide too easily
enum class Delimiter {
  Auto,
  LR,
  Never
};

enum class DerivStyle {
  Frac,
  Offset
};

struct Options {
  MulStyle mult[2] = {MulStyle::Implicit, MulStyle::Cdot};
  DivStyle div = DivStyle::Frac;    
  DerivStyle der = DerivStyle::Frac;  
  bool greek = true;
  EmbedMode output = EmbedMode::Raw;
  Delimiter sizing = Delimiter::Auto;
  ParenPolicy parens = ParenPolicy::Minimal;
  MultiChar multiChar = MultiChar::Italic;
  bool autoCopy = true;
};

} // EqP