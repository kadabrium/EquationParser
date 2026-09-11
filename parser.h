// EquationParser PEGTL front end produces a PEGTL parse tree, 
// and it does not yet run the precedence-climbing fold. 
// `expr` / `prefixed` / `postfixed` nodes therefore arrive flat
#pragma once
#include <memory>
#include <ostream>
//#include <stdexcept>
#include <string>
#include <string_view>
#include <tao/pegtl.hpp>
#include <tao/pegtl/control/must_if.hpp>
#include <tao/pegtl/extra/parse_tree.hpp>
#include "diagnostic.h"   
namespace pegtl = TAO_PEGTL_NAMESPACE;
namespace EqP {


namespace grammar {
// whitespace
struct ws : pegtl::star<pegtl::space> {};
// numbers, with augments (+-E.): digit+ ("." digit+)? ([eE] [+-]? digit+)?
struct number : pegtl::seq<
  pegtl::plus<pegtl::digit>,
  pegtl::opt<pegtl::one<'.'>, 
  pegtl::plus<pegtl::digit>>,
  pegtl::opt<pegtl::one<'e', 'E'>,
  pegtl::opt<pegtl::one<'+', '-'>>,
  pegtl::plus<pegtl::digit>>> {};
// identifiers: alpha (alnum | "_")* "'"*  
// prime's can be part of the name: f'' is one id
struct identifier : pegtl::seq<
  pegtl::identifier, 
  pegtl::star<pegtl::one<'\''>>> {};

// infix, prefix, postfix operators
struct op_add : pegtl::one<'+'> {};
struct op_sub : pegtl::one<'-'> {};
// `*` and `**` overlap on their first byte, so a PEG needs maximal munch here.
// Two independent guards, because either alone is a trap:
//   - `two<'*'>` is tried *before* `one<'*'>` in the sor.  sor is ordered, so
//     without this `a**b` matches op_mul, leaves a stray '*' where an operand
//     must start, and the whole expr backtracks to a parse failure.
//   - `not_at<one<'*'>>` makes op_mul refuse to match the first half of a `**`
//     regardless of sor order, so reordering the sor later cannot silently
//     resurrect that bug.  not_at consumes nothing, so the node still spans
//     exactly one byte.
struct op_mulv : pegtl::two<'*'> {};
struct op_mul : pegtl::seq<pegtl::one<'*'>, pegtl::not_at<pegtl::one<'*'>>> {};
struct op_div : pegtl::one<'/'> {};
struct op_pow : pegtl::one<'^'> {};
//
struct op_eq: pegtl::one<'='>{};
//
struct infix_op : pegtl::sor<op_add, op_sub, op_mulv, op_mul, op_div, op_pow, 
 op_eq> {};

struct pre_neg : pegtl::one<'-'> {};
struct pre_pos : pegtl::one<'+'> {};
struct prefix_op : pegtl::sor<pre_neg, pre_pos> {};

struct post_fact : pegtl::one<'!'> {};
struct postfix_op : pegtl::sor<post_fact> {};

struct expr; // final expression forward declared

// function calls
// arglist (list of expressions)
// Not pegtl::list: a bare list lets the trailing separator backtrack away, so
// `f(x,)` silently reduces to the one-argument `f(x)` and the failure surfaces
// later as the wrong complaint ("expected ')'") at the wrong place (the comma).
// Once a ',' is consumed an argument is mandatory, and must<> is what turns
// that commitment into a located diagnostic.
struct arglist : pegtl::seq<
  expr,
  pegtl::star<pegtl::pad<pegtl::one<','>, pegtl::space>, 
  pegtl::must<expr>>> {};

struct inv_sup: pegtl::seq<
  pegtl::one<'^'>, 
  pegtl::one<'-'>, pegtl::one<'1'>,
  pegtl::at<ws, pegtl::one<'('>>> {};

// parenthesis matching for function calls
// 'must' only after '(' has been consumed: parser has committed to a call,
// so a failure past this point is a real error rather than a backtrack.
struct call : pegtl::seq<
  identifier, ws,
  pegtl::opt<inv_sup>, ws,
  pegtl::one<'('>, ws,
  pegtl::opt<arglist>, ws,
  pegtl::must<pegtl::one<')'>>> {};
// parens matching for non function calls
struct paren : pegtl::seq<
  pegtl::one<'('>, ws,
  pegtl::must<expr>, ws,
  pegtl::must<pegtl::one<')'>>> {};


// call/expression shapes summary
// 'call' before 'identifier': both start with an identifier, 
// and sor backtracks cleanly when the '(' is absent.
struct primary : pegtl::sor<number, call, identifier, paren> {};
struct postfixed : pegtl::seq<primary, pegtl::star<ws, postfix_op>> {};
struct prefixed : pegtl::seq<pegtl::star<prefix_op, ws>, postfixed> {};
// sub-expressions (which may contain functions) in turn connected by infix ops:
// operand op operand op operand ... flat to be folded
// Same reason as arglist for not using pegtl::list: on 'a+ the star's
// iteration fails, backtracks, and expr succeeds having consumed only 'a'
// leaving the '+' to be reported much later by eof, with no position of its
// own. After an infix operator an operand is mandatory.
struct expr : pegtl::seq<
  prefixed,
  pegtl::star<pegtl::pad<infix_op, pegtl::space>, pegtl::must<prefixed>>> {};

// check for open rparen at the end of expr
struct no_stray_rparen : pegtl::not_at<pegtl::one<')'>> {};

// grammar export object
// must<expr> gives "" a position instead of a silent null; must<eof> catches
// trailing input and reports it *at* the offending character, which is where a
// note about implicit multiplication being off belongs
struct grammar : pegtl::seq<ws, pegtl::must<expr>, ws, 
pegtl::must<no_stray_rparen>,
pegtl::must<pegtl::eof>> {};

// PEGTL's exception is templated on triggering position type
// #include must_if for custom message
// call as pegtl::must_if_n<grammar::errors>
struct errors {
  template <typename Rule> 
  static constexpr const char* message = nullptr;
  // built-in raise_on_failure is reset to false.  It defaults to
  // message<Rule> != nullptr which would make any failure of a
  // message-carrying type throw, including `primary` which is a sor 
  // whose alternatives are supposed to fail and backtrack.  
  // must<> should stay the single place commitment is declared.
  template <typename Rule> 
  static constexpr bool raise_on_failure = false;
};
template <> 
inline constexpr const char* errors::message<expr> =
  "Error: expected an operand or expression after ','";
template <> 
inline constexpr const char* errors::message<prefixed> =
  "Error: expected an operand or expression after operator";
template <> 
inline constexpr const char* errors::message<pegtl::one<')'>> =
  "Error: unclosed '(', expected ')'";
template <>
inline constexpr const char* errors::message<no_stray_rparen> =
  "Error: unclosed ')', no '(' is open here";
template <> 
inline constexpr const char* errors::message<pegtl::eof> =
  "Error: unexpected trailing input (implicit multiplication is not supported in input)";
//template <typename T> int sizetest = sizeof(T);
//std::cout << sizetest<Case>;

// selector
template<typename Rule>
using selector = pegtl::parse_tree::selector<Rule,
  // text-bearing leaves.
  pegtl::parse_tree::store_content::on<
    number, identifier>,
  // Structure / type only
  pegtl::parse_tree::remove_content::on<
    op_add, op_sub, op_mul, op_mulv,
    op_div, op_pow, 
    op_eq,
    pre_neg, pre_pos, post_fact,
    call, inv_sup, arglist, paren>,
  // Collapse the layer when it added nothing. 
  pegtl::parse_tree::fold_one::on<
    expr, /*prefixed,*/ postfixed>>; 
    // process prefix last for single var case
    // 'a' should not come back as
  // expr(prefixed(postfixed(identifier))). With operators present the node
  // survives and holds the flat sequence.

    
}  // grammar


namespace parse {

using Input = pegtl::text_view_input<pegtl::default_eol, char, std::string>;
using ParseNode = pegtl::parse_tree::node_t<Input>;

/*
Exported ParseTree node:
ParseNode {
  string_view data; // value (for leaves; empty for ops)
  string_view type; // full type name as str, "EqP::grammar::expr". truncate with shortType()
  vector<unique_ptr<ParseNode>> children;
  text_position_with_source begin, end;
  is_root();
  //...
}
*/
std::string_view shortType(std::string_view type);

std::unique_ptr<ParseNode> parseTree(std::string_view src, std::string_view sourceName = "<expr>");

void printTree(std::ostream& os, const ParseNode& n, int indent = 0);

std::string toStringln(const ParseNode& n);


// what pegtl::parse_error is templated on for this input; carries .count
// (byte offset), .line, .column, .source
using Position = Input::error_position_t;

// PEGTL's exception is templated on the position type and its accessors return
// pre-rendered text; converting once here keeps that out of every caller and
// means the parse stage reports in the same shape as lower and render.
struct ParseError : DiagError {
  using DiagError::DiagError;
};

} // parse

} // EqP
