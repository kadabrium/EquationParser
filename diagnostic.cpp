#include "diagnostic.h"
#include <algorithm>
namespace EqP {

// <the ERROR line the span starts on>
// <the ^~~~
std::string renderCaret(std::string_view source, const Diagnostic& d) {
  // begin of node/error in source string
  // in-string with RenderErrors, eol/eol+1 with ParseErrors (unclosed brackets etc)
  const size_t begin = std::min(d.where.begin, source.size());
  const size_t end = std::clamp(d.where.end, begin, source.size());

  // begin of line containing the error
  // const size_t echoFrom = (std::string_view::npos > end)? 0 : begin;
  const size_t echoFrom = (d.where.end >= end)? 0 : begin;

  const size_t nextLine = std::min(source.find('\n', echoFrom), source.find('\r', echoFrom));
  const size_t echoTo = (nextLine == std::string_view::npos)? /*source.size()*/nextLine-1 : nextLine;

  std::string out = d.message + '\n';
  // echo source up to error location
  out += source.substr(echoFrom, echoTo - echoFrom); out += '\n';
  // whitespaces up to error location next line
  out.append(begin - echoFrom, ' ');
  out += '^';
  if (end > begin + 1) { out.append(end - begin - 1, '~'); } // none with ParseError at eol
  out += '\n';
  return out;
}

} // EqP
