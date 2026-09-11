#include <iostream>
#include <string>
#include <string_view>
#include "ctrl.h"
#include "lower.h"
#include "output.h"
namespace EqP{



struct Case {
  std::string_view src;
  std::string_view expected; 
};

constexpr Case cases[] = {
    // atoms, and fold_one collapsing the pass-through layers
    {"x", "ROOT(x)"},
    {"42", "ROOT(42)"},
    {"1.5e-3", "ROOT(1.5e-3)"},
    {"f''", "ROOT(f'')"},

    // infix, flat
    {"a+b", "ROOT(expr(a op_add b))"},
    {"1 + 2 * 3", "ROOT(expr(1 op_add 2 op_mul 3))"},
    {"a - b / c ^ d", "ROOT(expr(a op_sub b op_div c op_pow d))"},

    // prefix(handled together with expr) / postfix
    {"-x", "ROOT(expr(pre_neg x))"},
    {"--x", "ROOT(expr(pre_neg pre_neg x))"},
    {"n!", "ROOT(postfixed(n post_fact))"},
    {"-n!", "ROOT(expr(pre_neg postfixed(n post_fact)))"},
    {"2 - -3", "ROOT(expr(2 op_sub pre_neg 3))"},
    {"pi * -3", "ROOT(expr(pi op_mul pre_neg 3))"},

    // user inserted parens is kept as a node for --parens=preserve mode
    {"(a+b)*c", "ROOT(expr(paren(expr(a op_add b)) op_mul c))"},
    {"(a+b)**c", "ROOT(expr(paren(expr(a op_add b)) op_mulv c))"},
    {"a*b", "ROOT(expr(a op_mul b))"},
    {"a*b_ij", "ROOT(expr(a op_mul b_ij))"},
    {"5*beta_alpha", "ROOT(expr(5 op_mul beta_alpha))"},

    // calls (Arguments are supplied in original LaTex order)
    {"f(x)", "ROOT(call(f arglist(x)))"},
    {"g()", "ROOT(call(g))"},
    {"int(1, 2, x+y, x)",
      "ROOT(call(int arglist(1 2 expr(x op_add y) x)))"},
    {"int(x+y, x)",
      "ROOT(call(int arglist(expr(x op_add y) x)))"},
    {"sum(i, 1, n, x^2)",
      "ROOT(call(sum arglist(i 1 n expr(x op_pow 2))))"},
    {"lim(x, 0, f)", "ROOT(call(lim arglist(x 0 f)))"},
    {"sqrt(3, x)", "ROOT(call(sqrt arglist(3 x)))"},
    {"sqrt(x)", "ROOT(call(sqrt arglist(x)))"},
    {"deriv(x+y, x)",
      "ROOT(call(deriv arglist(expr(x op_add y) x)))"},
    // bracket height
    {"f(softmax(alpha), -1)", "ROOT(call(f arglist(call(softmax arglist(alpha)) expr(pre_neg 1))))"},
    // inverse func
    {"f(sigmoid^-1(alpha), -1)", "ROOT(call(f arglist(call(sigmoid inv_sup arglist(alpha)) expr(pre_neg 1))))"},
    {"sqrt^-1(alpha, beta)", "ROOT(call(sqrt inv_sup arglist(alpha beta)))"},

    {"sin(x)^2", "ROOT(expr(call(sin arglist(x)) op_pow 2))"},
    {"cos(a+b)+c", "ROOT(expr(call(cos arglist(expr(a op_add b))) op_add c))"},
    {"x/y+asin(pi)", "ROOT(expr(x op_div y op_add call(asin arglist(pi))))"},
    {"acos(20*pi/y)+sin(pi)", "ROOT(expr(call(acos arglist(expr(20 op_mul pi op_div y))) op_add call(sin arglist(pi))))"},
    {"cos(20/pi-y)+asin(pi)", "ROOT(expr(call(cos arglist(expr(20 op_div pi op_sub y))) op_add call(asin arglist(pi))))"},

    {"-a**b/(c+d)^2", "ROOT(expr(pre_neg a op_mulv b op_div paren(expr(c op_add d)) op_pow 2))"},
    {"-(a*b/(c+d)^2)", "ROOT(expr(pre_neg paren(expr(a op_mul b op_div paren(expr(c op_add d)) op_pow 2))))"},

    // empty input: continue, no error msg
    {"", ""},
    {"   ", ""},

    // invalid cases, error msg with location data
    // parse error
    {"a +", ""},          // must<prefixed> after an infix operator
    {"sqrt(x,)", ""},     // must<expr> after a comma
    {"f(x", ""},          // must<one<')'>> 
    {"sin(z+(x+y)))", ""}, // must<no_stray_rparen>
    {"sin(z+(x+y)))+w", ""},
    {"(a+b", ""},
    {"2x", ""},           // must<eof>; implicit multiplication defaults to off
    // arity error
    {"sqrt(1,2,3)", "ROOT(call(sqrt arglist(1 2 3)))"},

    
};

int testAST() {
  std::cout << "AST Test \n";
  int failures = 0;
  for (const Case& c : cases) {
    std::string got;
    try {
      auto pt = parse::parseTree(c.src);
      if (!pt) { continue; }   // empty input
      got = lower::makeAST(*pt, std::string(c.src)).toStringln();
    }
    catch (parse::ParseError& pe) {
      got = "Expected invalid input: " + pe.diag.message;
    }
    catch (lower::LowerError&) {
      got = "LowerError";
    }

    bool ok = (got != "LowerError");
    failures += !ok;
    std::cout << c.src << " -> ";
    std::cout << got  << '\n';
  }
  std::cout << (std::size(cases) - failures) << '/' << std::size(cases) << " passed\n";
  return failures == 0 ? 0 : 1;
}

int testParseTree() {
  std::cout << "PT Test \n";
  int failures = 0;
  for (const Case& c : cases) {
    std::string got;
    try {
      auto root = parse::parseTree(c.src);
      got = root ? parse::toStringln(*root) : "";
    }
    catch (const parse::ParseError&) {
      got = "";
    }

    const bool ok = (got == c.expected);
    failures += !ok;
    std::cout << (ok ? " ok " : " FAIL ") << c.src << " -> ";
    std:: cout << ((got == "")? "None" : got)  << '\n';
    if (!ok) {
      std::cout << " expected: " << (c.expected.empty() ? "<error>" : c.expected) << '\n'
                << " got: " << (got.empty() ? "<error>" : got) << '\n';
    }
  }
  std::cout << (std::size(cases) - failures) << '/' << std::size(cases) << " passed\n";
  return failures == 0 ? 0 : 1;
}

void testRender() {
  std::cout << "Render Test \n";
  out::Renderer r{Options{}};
  for (const Case& c : cases) {
    std::string got;
    try {
      auto pt = parse::parseTree(c.src);
      // continue on blank input
      if (!pt) { continue; }
      auto tree = lower::makeAST(*pt, std::string(c.src));
      got = r.render(tree);
    }
    catch (parse::ParseError& pe) {
      got = renderCaret(c.src, pe.diag);
    }
    catch (lower::LowerError& le) {
      got = renderCaret(c.src, le.diag);
    }
    catch (out::ArityError& ae) {
      got = renderCaret(c.src, ae.diag);
    }
    catch (std::runtime_error& e) {
      got = std::string("Unexpected RenderError: ") + e.what();
    }
    std::cout << c.src << " -> " << got << "\n";
  }
}

// -- end to end -------------------------------------------------------
// Everything above hands makeAST a fresh std::string built from a string_view
// that stays alive independently, which is the safe lifetime shape.  ctrl::render
// owns its source and passes it on, which is not, and nothing tested that path
// even though it is the only one both frontends call.
// Hence the input lengths below.  MSVC keeps strings of 15 characters or fewer
// inside the string object, so moving one relocates the bytes instead of
// stealing a heap pointer, and anything holding a view into the source then
// reads a buffer the move has edited: byte 0 becomes '\0'.  Longer inputs
// survive by luck, so a table of only realistic-looking expressions would have
// missed it.  The 15/16 pair straddles the boundary deliberately.
struct RenderCase {
  std::string_view src;
  std::string_view expected;  // rendered LaTeX; ignored when mustFail
  bool mustFail = false;
};

constexpr RenderCase renderCases[] = {
    // short: inline storage, the length range the SSO bug corrupted
    {"x", "x"},                                              // 1
    {"a+b", "a+b"},                                          // 3
    {"f(x)", "\\mathit{f}(x)"},                              // 4
    {"sqrt(x)", "\\sqrt{x}"},                                // 7
    {"1 + 2 * 3", "1+2\\cdot 3"},                            // 9
    {"sqrt(3, x)", "\\sqrt[3]{x}"},                          // 10
    {"5*beta_alpha", "5\\beta_{\\alpha}"},                   // 12
    {"-a**b/(c+d)^2", "\\frac{-a\\cdot b}{(c+d)^{2}}"},      // 13

    // the boundary itself: same expression either side of 15 characters
    {"sum(i, 1, n, x)", "\\sum_{i=1}^{n}x"},                 // 15, last inline
    {"sum(i, 1, n, xy)", "\\sum_{i=1}^{n}xy"},               // 16, first heap

    // long: heap storage, which a move transfers intact
    {"sum(i, 1, n, x^2)", "\\sum_{i=1}^{n}x^{2}"},           // 17
    {"alphabeta+gammadelta", "alphabeta+gammadelta"},        // 20
    {"acos(20*pi/y)+sin(pi)", "\\cos^{-1} \\frac{20\\pi }{y}+\\sin \\pi"},

    // empty input succeeds with no output rather than erroring
    {"", ""},
    {"   ", ""},

    // failures must arrive as Outcome, not as an escaped exception
    {"a +", "", true},              // parse
    {"f(x", "", true},              // parse
    {"sin(z+(x+y)))", "", true},    // parse
    {"sqrt(1,2,3)", "", true},      // arity, thrown from the render stage
};

int testEndToEndRender() {
  std::cout << "End-to-end Render Test \n";
  out::Renderer r{Options{}};
  int failures = 0;
  for (const RenderCase& c : renderCases) {
    const ctrl::Outcome res = ctrl::render(std::string(c.src), r);

    const bool ok = c.mustFail ? (!res.ok() && !res.message.empty())
                               : (res.ok() && res.latex == c.expected);
    failures += !ok;
    std::cout << (ok ? " ok " : " FAIL ") << c.src << " -> "
              << (res.ok() ? res.latex : res.message) << '\n';
    if (!ok) {
      std::cout << " expected: " << (c.mustFail ? "<error>" : std::string(c.expected)) << '\n'
                << " got: " << (res.ok() ? res.latex : "<error> " + res.message) << '\n';
    }
  }
  std::cout << (std::size(renderCases) - failures) << '/' << std::size(renderCases) << " passed\n";
  return failures == 0 ? 0 : 1;
}


}


int main(int argc, char** argv) {
  using namespace EqP;
  const int parseResult = testParseTree();
  const int astResult = testAST();
  testRender();
  const int renderResult = testEndToEndRender();
  return parseResult || astResult || renderResult;

}