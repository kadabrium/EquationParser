# Third-party notices

## Qt 6.11.1

The Windows release archive dynamically links EquationParser to Qt 6.11.1 libraries and plugins, including Qt Core, GUI, Widgets, Network, and SVG.

Qt is used under the GNU Lesser General Public License version 3.0. A copy of that license is provided in [LICENSES/LGPL-3.0.txt](LICENSES/LGPL-3.0.txt). The corresponding source code is available from the Qt project at https://download.qt.io/official_releases/qt/6.11/6.11.1/submodules/ and https://code.qt.io/cgit/qt/.

The release keeps the Qt runtime as separate dynamic libraries. You may replace those libraries with compatible versions, subject to the GNU LGPL v3 and Qt's licensing terms. Qt is a trademark of The Qt Company Ltd.

## PEGTL

This project obtains PEGTL 3.2.8 at build time from https://github.com/taocpp/PEGTL. PEGTL is licensed under the MIT License.

## Microsoft graphics components

A Windows Qt deployment can include Microsoft Direct3D compiler/runtime files where required by Qt. Those components remain subject to their applicable Microsoft license terms.
