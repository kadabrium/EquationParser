#include <cassert>
#include "parser.h"
namespace EqP {


namespace parse {
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

// strip namespace prefix on exported op names
std::string_view shortType(std::string_view type) {
  size_t pos = type.rfind("::");
  return (pos == std::string_view::npos)? type : type.substr(pos + 2);
}

// generate parse tree from str src with defined ops in grammar.
std::unique_ptr<ParseNode> parseTree(std::string_view src, std::string_view sourceName) {
  // Returns null without error msg when input is empty,
  if (src.find_first_not_of(" \t\r\n\f\v") == std::string_view::npos) {
    return nullptr;
  }

  Input in(std::string(sourceName), src);
  try {
    auto root = pegtl::parse_tree::parse<
      grammar::grammar, ParseNode, grammar::selector,
      pegtl::nothing, pegtl::must_if_n<grammar::errors>::type>(in);
    assert(root);   // empty handled above, grammar is ws must<expr> ws must<eof>
    return root;
  }
  // Every other failure throws with error position data
  catch (const pegtl::parse_error<Position>& e) {
    const Position& p = e.position_object();
    throw ParseError(Diagnostic{std::string(e.message()), Span{p.count, p.count + 1}});
  }
}

void printTree(std::ostream& os, const ParseNode& n, int indent) {
  os << std::string(indent * 2, ' ');
  if (n.is_root()) {
    os << "ROOT";
  }
  else {
    os << shortType(n.type);
    if (!n.data.empty()) {
        os << "  \"" << n.data << '"';
    }
  }
  os << '\n';
  for (const auto& c : n.children) {
    printTree(os, *c, indent + 1);
  }
}

// One line form eg. expr(a op_add b) 
std::string toStringln(const ParseNode& n) {
  std::string out;
  if (n.is_root()) { out = "ROOT"; }
  // ops print type
  else if (n.data.empty()) { out = shortType(n.type); }
  else {
    // leaves print value
    out = std::string(n.data);
  }
  
  if (!n.children.empty()) {
    out += '(';
    for (std::size_t i = 0; i < n.children.size(); ++i) {
      if (i != 0) { out += ' '; }
      out += toStringln(*n.children[i]);
    }
    out += ')';
  }
  return out;
}

} // parse
} // eqp
