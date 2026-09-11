#pragma once
#include <array>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#ifdef _WIN32
  #include <windows.h>
#endif
#include "output.h"
#include "lower.h"
namespace EqP{



namespace ctrl{

inline constexpr size_t numBuiltinOps = 5;
inline constexpr std::array<std::string_view, 5> builtinOpNotation = {
  "Arithmetic:\n "
  "+  -  x  /  \n"
  "Multiply(primary style):  *  \n"
  "Multiply(secondary style): **\n"
  "Exponent: ^     Factorial: !",

  "Other arithmetic: \nsqrt()  lg()  ln()\n"
  "n-th log: log(n, x)\nn-th sqrt: sqrt(n, x)",

  "Trig:\n "
  "sin()  cos()  tan() \nasin()  acos()  atan()",

  "Calculus:\n"
  "limit: lim(x, target, f)\n"
  "definite integral: int(f, x)\n"
  "indefinite integral: int(lo, hi, f, x)\n"
  "derivative: deriv(f, x)",

  "Other: \n"
  "binomial: binom(n, r)\n"
  "definite sum: sum(x, lo, hi, f)\n"
  "indefinite sum: sum(f)\n"
  "inverse function: func^-1 (verbatim function only)"
  
};

inline constexpr size_t numOptions = 10;
// Slot ids for the three parallel option tables below.  
// naming the slots to check for misalignment with static assert
enum OptionSlot : std::size_t {
  MultStyleSlot, MultStyle2Slot, DivStyleSlot, DerivStyleSlot, EmbedModeSlot,
  ParensPolicySlot, DelimiterSlot, GreekSlot, MultiCharSlot, AutoCopySlot
};

inline constexpr std::array<std::string_view, numOptions> optionDesc = {
  "Multiplication style",
  "Secondary multiplication style",
  "Division style",
  "Derivative style",
  "Embedding mode",
  "Redundant parenthesis policy",
  "Delimiter parenthesis style",
  "Auto Greek letters",
  "Multi-char function names",
  "Auto copy to clipboard"
};

inline constexpr std::array<std::string_view, numOptions> optionCodeNames = {
  "MultStyle",
  "MultStyle2",
  "DivStyle",
  "DerivStyle",
  "EmbedMode",
  "ParensPolicy",
  "Delimiter",
  "Greek",
  "MultiChar",
  "AutoCopy"
};

// Choice groups.  MultStyle and MultStyle2 share one, as do Greek and AutoCopy,
// so these are named separately and referenced twice rather than duplicated.

// ORDER IS LOAD-BEARING.  A choice's position in its group is the enum value it
// maps to -- see options.h.  Eight of the ten go straight through a static_cast;
// the two bool-backed ones are `index == 0`, because "On" sits at index 0 while
// the bool it stands for is 1.  Reordering a group silently changes what an
// option means.  Renaming an entry changes the CLI's command vocabulary and
// invalidates saved GUI settings, which store these labels verbatim.
inline constexpr std::string_view mulStyleGroup[] = {"Implicit", "Dot", "Cross"};
inline constexpr std::string_view divStyleGroup[] = {"Fraction", "Slash"};
inline constexpr std::string_view derivGroup[] = {"Numerator", "Offset"};
inline constexpr std::string_view embedGroup[] = {"Raw", "Inline", "Display"};
inline constexpr std::string_view parenGroup[] = {"Discard", "Preserve"};
inline constexpr std::string_view delimGroup[] = {"Auto", "LR", "Never"};
inline constexpr std::string_view multiCharGroup[] = {"Italic", "Roman"};
inline constexpr std::string_view onOffGroup[] = {"On", "Off"};

inline constexpr std::array<std::span<const std::string_view>, numOptions> optionChoices = {{
  mulStyleGroup,   // MultStyle
  mulStyleGroup,  // MultStyle2
  divStyleGroup,   
  derivGroup,      
  embedGroup,   
  parenGroup,     
  delimGroup,     
  onOffGroup,  // Greek
  multiCharGroup,
  onOffGroup,  // AutoCopy
}};

// Check alignment between slot enum and name table
static_assert(optionCodeNames[MultStyleSlot]    == "MultStyle");
static_assert(optionCodeNames[MultStyle2Slot]   == "MultStyle2");
static_assert(optionCodeNames[DivStyleSlot]     == "DivStyle");
static_assert(optionCodeNames[DerivStyleSlot]   == "DerivStyle");
static_assert(optionCodeNames[EmbedModeSlot]    == "EmbedMode");
static_assert(optionCodeNames[ParensPolicySlot] == "ParensPolicy");
static_assert(optionCodeNames[DelimiterSlot]    == "Delimiter");
static_assert(optionCodeNames[GreekSlot]        == "Greek");
static_assert(optionCodeNames[MultiCharSlot]    == "MultiChar");
static_assert(optionCodeNames[AutoCopySlot]     == "AutoCopy");
static_assert(AutoCopySlot + 1 == numOptions);

static inline std::size_t optionSlot(std::string_view codeName) {
  for (size_t i = 0; i < numOptions; ++i) {
    if (optionCodeNames[i] == codeName) { return i; }
  }
  // default when the name is unknown.
  return numOptions;
}

// entry idx of optionChoices[slot] the renderer currently sits on 
// Callers that want the label go through getOptionStatus
static inline std::size_t optionIndex(const out::Renderer& renderer, std::size_t slot) {
  const Options& opt = renderer.opt;
  switch (slot) {
    case MultStyleSlot:    return static_cast<std::size_t>(opt.mult[0]);
    case MultStyle2Slot:   return static_cast<std::size_t>(opt.mult[1]);
    case DivStyleSlot:     return static_cast<std::size_t>(opt.div);
    case DerivStyleSlot:   return static_cast<std::size_t>(opt.der);
    case EmbedModeSlot:    return static_cast<std::size_t>(opt.output);
    case ParensPolicySlot: return static_cast<std::size_t>(opt.parens);
    case DelimiterSlot:    return static_cast<std::size_t>(opt.sizing);
    case GreekSlot:        return opt.greek ? 0u : 1u;
    case MultiCharSlot:    return static_cast<std::size_t>(opt.multiChar);
    case AutoCopySlot:     return opt.autoCopy ? 0u : 1u;
    default:               return 0u;
  }
}

static inline std::string_view getOptionStatus(const out::Renderer& renderer, size_t slot) {
  return optionChoices[slot][optionIndex(renderer, slot)];
}

static inline void applyOption(std::size_t slot, std::size_t idx, out::Renderer& renderer) {
  switch (slot) {
    case MultStyleSlot: renderer.setPrimaryMulStyle(static_cast<MulStyle>(idx));  break;
    case MultStyle2Slot: renderer.setVariantMulStyle(static_cast<MulStyle>(idx));  break;
    case DivStyleSlot: renderer.setDivStyle(static_cast<DivStyle>(idx));         break;
    case DerivStyleSlot: renderer.setDerivStyle(static_cast<DerivStyle>(idx));     break;
    case EmbedModeSlot: renderer.setEmbedMode(static_cast<EmbedMode>(idx));       break;
    case ParensPolicySlot: renderer.setParenPolicy(static_cast<ParenPolicy>(idx));   break;
    case DelimiterSlot: renderer.setDelimiter(static_cast<Delimiter>(idx));       break;
    case GreekSlot: renderer.setGreek(idx == 0);                              break;
    case MultiCharSlot: renderer.setMultiChar(static_cast<MultiChar>(idx));       break;
    case AutoCopySlot: renderer.setAutoCopy(idx == 0);                           break;
    default: break;
  }
}

static inline std::vector<std::string> optionRows(const out::Renderer& renderer) {
  std::vector<std::string> rows;
  rows.reserve(numOptions+1);
  rows.push_back("Description - Codename - Current");
  for (std::size_t i = 0; i < numOptions; ++i) {
    rows.push_back(std::string(optionDesc[i])
      + " - " + std::string(optionCodeNames[i])
      + " - " + std::string(getOptionStatus(renderer, i)));
  }
  return rows;
}

static inline std::string setOption(std::string_view command, out::Renderer& renderer) {
  const size_t bound = command.find("::");
  // Guard npos so substr(bound + 2) on npos wraps to substr(1) 
  if (bound == std::string_view::npos) { return "Option not found."; }
  const std::string_view codeName = command.substr(0, bound);
  const std::string_view value = command.substr(bound + 2);

  const size_t slot = optionSlot(codeName);
  if (codeName.empty() || value.empty()
    || command.find("::", bound + 2) != std::string_view::npos
    || slot == numOptions) {
    return "Option not found.";
  }

  const std::span<const std::string_view> choices = optionChoices[slot];
  std::size_t idx = choices.size();
  for (size_t i = 0; i < choices.size(); ++i) {
    if (value == choices[i]) { idx = i; break; }
  }
  if (idx == choices.size()) {
    std::ostringstream validVals;
    for (size_t i = 0; i < choices.size(); ++i) {
      if (i != 0) {
        validVals << ", ";
      }
      validVals << choices[i];
    }
    return "Invalid value for " + std::string(codeName) + ". Options: " + validVals.str();
  }

  applyOption(slot, idx, renderer);

  return std::string(codeName) + " set to "
          + std::string(getOptionStatus(renderer, slot));

}

// structured return data so that successful out puts and 
// error messages can render differently
struct Outcome {
  std::string latex;   // filled if result received
  std::string message; // filled if error message reveicef
  std::optional<Span> where; // set when the failure is located
  bool ok() const { return message.empty(); }
};

static inline Outcome render(const std::string& input, out::Renderer& r) {
  try {
    auto pt = parse::parseTree(input);
    if (!pt) { return {}; } 
    // nb: parse tree passed to lower still holds refs to input
    // so input cannot be eg. moved before lower returns
    auto tree = lower::makeAST(*pt, input);
    return { r.render(tree), {}, std::nullopt };
  }
  catch (const DiagError& e) {
    return { {}, e.diag.message, e.diag.where };
  }
  catch (const std::exception& e) {
    return { {}, std::string("Unexpected error: ") + e.what(), std::nullopt };
  }
}

// error message with caret for CLI., use raw for GUI
static inline std::string errorText(std::string_view src, const Outcome& o) {
  return o.where ? renderCaret(src, {o.message, *o.where}) : o.message;
}

namespace{
struct closeFile { 
  void operator()(FILE* f) const {
    if (f != nullptr) { fclose(f); }
  }
};}
inline static Outcome batchProcess(const std::string& sourcePath, const std::string& targetPath, out::Renderer& r) {
  std::unique_ptr<FILE, closeFile> source{fopen(sourcePath.c_str(), "r")};
  if (!source) {
    return Outcome{{}, "Error: unable to open input file", std::nullopt};
  }

  // explicitly create path first if not found, but text file itself creates automatically with fopen
  const std::filesystem::path targetParent = std::filesystem::path(targetPath).parent_path();
  if (!targetParent.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(targetParent, ec);
    if (ec) {
      return Outcome{{}, "Error: Unable to create directory: " + targetParent.string(), std::nullopt};
    }
  }
  std::unique_ptr<FILE, closeFile> target{fopen(targetPath.c_str(), "w")};
  if (!target) {
    return Outcome{{}, "Error: Unable to open output file: " + targetPath, std::nullopt};
  }

  char buffer[256];
  while (fgets(buffer, sizeof(buffer), source.get()) != NULL) {
    Outcome resLine = render(std::string(buffer), r);
    if (resLine.ok()) {
      fputs((resLine.latex + "\n").c_str(), target.get());
    }
    else {
      fputs(ctrl::errorText(buffer, resLine).c_str(), target.get());
    }
  }
  // displayed in terminal
  return Outcome{"File saved to" + targetPath, ""};
}

#ifdef _WIN32
inline static void windowsClipboard(const std::string& output) {
  size_t len = output.length() + 1;
  HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len); if (!hMem) return;
  std::memcpy(GlobalLock(hMem), output.c_str(), len);
  GlobalUnlock(hMem);

  if (OpenClipboard(nullptr)) {
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
  }
}
#endif

}

}