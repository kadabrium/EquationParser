// EquationParser -- source locations and diagnostics.  See DESIGN.md section 6.6.
#pragma once
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
namespace EqP {

// half-open char idx range into the source string
struct Span { std::size_t begin = 0, end = 0; };

// 3 major error types:
// unbalanced brackets   parse    parse_error::position_object().count
// unclosed expression   parse    same
// arity                 render   the Call node's span
struct Diagnostic {
  std::string message;
  Span where;
};

// Base for every frontend-visible failure.  Each stage already carries a
// Diagnostic, so sharing a base is what lets a frontend catch once -- and what
// keeps a fourth error type from needing an edit in every UI.
struct DiagError : std::runtime_error {
  Diagnostic diag;
  explicit DiagError(Diagnostic d):
    std::runtime_error(d.message), diag(std::move(d)) {}
};

// render error msg, separately called at both parse and render stages
// all three stages report the same shape, so one caret renderer covers them
std::string renderCaret(std::string_view source, const Diagnostic& d);

} 
