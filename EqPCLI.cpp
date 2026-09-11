#include <iostream>
#include "ctrl.h"
using namespace EqP;

void listNotation() {
  for (auto& f: ctrl::builtinOpNotation) {
    std::cout << f << "\n\n";
  }
}

void listOptions(const out::Renderer& renderer) {
  for (const std::string& row : ctrl::optionRows(renderer)) {
    std::cout << row << "\n";
  }
}

int main(/*int argc, char* argv[]*/) {
  out::Renderer r{Options{}};
  bool quit = false;
  while (!quit) {
    std::cout << "EquationParser: Enter expression, \"options\" for options, \"Codename::Option\" to change one, or \"notation\" for function notation info\n";
    std::string input;
    std::cin >> input;
    if (input == "options") {
      listOptions(r);
    }
    else if (input == "notation") {
      listNotation();
    }
    else if (input == "quit") {
      quit = true;
    }
    // use :: as sign for option commands
    else if (input.find("::") != std::string_view::npos) {
      std::cout << ctrl::setOption(input, r) << "\n";
    }
    else {
      ctrl::Outcome res = ctrl::render(input, r);
      if (!res.ok()) {
        std::cout << ctrl::errorText(input, res) << "\n";
      }
      else {
        std::cout << res.latex << "\n";
        #ifdef _WIN32
          if (r.opt.autoCopy) {
            ctrl::windowsClipboard(res.latex);
          }
        #endif
      }
    }

  }

  
  return 0;
}