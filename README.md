# EquationParser

EquationParser converts a compact, plain-text mathematical expression language into LaTeX. It provides:

- `EqPGUI`: a Qt 6 desktop interface with batch conversion, copy, save, and image-rendering actions.
- `EqPCLI`: an interactive command-line interface.
- `EqPTest`: parser, AST, and end-to-end rendering tests.

## Requirements

- A C++20 compiler
- CMake 3.24 or later
- Qt 6.5 or later with the `Widgets`, `Network`, and `Svg` components
- Network access at configure time so CMake can download PEGTL

Point CMake at the Qt installation when it is not already discoverable. For example:

```text
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<kit>
cmake --build build --config Release
ctest --test-dir build --build-config Release
```

On Windows, a CMake package install deploys the required Qt DLLs and plugins beside `EqPGUI.exe`:

```text
cmake --install build --config Release --prefix dist
```

## Usage

Start `EqPGUI` and enter an expression, such as `sqrt(3, x)` or `sum(i, 1, n, x^2)`, then select **Convert** to copy or render the generated LaTeX.

`EqPCLI` accepts an expression per prompt. Enter `notation` to list supported notation, `options` to inspect render options, or `quit` to exit.

## Releases

Windows release archives contain `EqPGUI.exe`, the dynamically linked Qt runtime, and the notices required for that runtime. Extract the complete archive before running the application. The Qt DLLs may be replaced with compatible builds as permitted by the LGPL.

The GUI's optional **Render** action sends the generated LaTeX to the public CodeCogs endpoint. It therefore requires network access and is subject to that service's terms and availability.

## License

The EquationParser source code is released under the [MIT License](LICENSE). It dynamically links to Qt, which is provided in Windows release archives under the GNU LGPL v3; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [LICENSES/LGPL-3.0.txt](LICENSES/LGPL-3.0.txt).

PEGTL is retrieved by CMake when building from source and is licensed under the MIT License.
