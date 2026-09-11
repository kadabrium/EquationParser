# for workspace globals and presets, check parent folder
# default to msvc ninja
# 3.28 is required for EXCLUDE_FROM_ALL on FetchContent_Declare (see below).
cmake_minimum_required(VERSION 3.28)

project(EquationParser VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)

# PEGTL is fetched at configure time rather than read from a path on this
# machine, so a clean checkout configures anywhere with a network connection.
#
# This pin is a commit on main, not a release tag, because the sources need the
# unreleased 4.0.0 layout: parser.h includes <tao/pegtl/control/must_if.hpp> and
# <tao/pegtl/extra/parse_tree.hpp>, neither of which exists in 3.2.8 (3.x has no
# must_if at all and spells the other one contrib/parse_tree.hpp). Do not
# "helpfully" move this to the newest tag -- 3.2.8 is the newest tag and it does
# not compile. Note 4.x is also Boost Software License 1.0 where 3.x was MIT;
# THIRD_PARTY_NOTICES.md and LICENSES/ track that.
#
# GIT_SHALLOW is deliberately absent: it fetches --depth 1 of the branch tips,
# so it only ever resolves a hash that is still the tip of main and silently
# breaks as soon as upstream commits again. The full clone costs ~9 MB.
#
# EXCLUDE_FROM_ALL: 
# PEGTL CMakeLists.txt installs unconditionally (there is no PEGTL_INSTALL option),
# and FetchContent brings it in via add_subdirectory, so without this its rules
# join *our* install and leak include/tao/pegtl/**, share/pegtl/cmake/*,
# share/pkgconfig/pegtl.pc and share/doc/tao/pegtl/LICENSE_1_0.txt into the
# CPack ZIP. CMake >= 3.28 ignores install() rules from an EXCLUDE_FROM_ALL
# subdirectory. SYSTEM keeps PEGTL's headers out of our warning settings.
FetchContent_Declare(
    pegtl
    GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
    GIT_TAG 8b9ddae995d035859ae5b8eada4f17b5f5203c3b # main, 4.0.0, 2026-07-19
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_MakeAvailable(pegtl)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Network Svg)

add_library(equation_parser_core STATIC
    ast.cpp
    astops.cpp
    diagnostic.cpp
    lower.cpp
    output.cpp
    parser.cpp
)
target_include_directories(equation_parser_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(equation_parser_core PUBLIC taocpp::pegtl)
target_compile_features(equation_parser_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(equation_parser_core PUBLIC /bigobj /wd4503 /FS)
endif()

add_executable(EqPCLI EqPCLI.cpp)
target_link_libraries(EqPCLI PRIVATE equation_parser_core)

add_executable(EqPGUI EqPGUI.cpp EqPGUI.h)
target_link_libraries(EqPGUI PRIVATE
    equation_parser_core
    Qt6::Widgets
    Qt6::Network
    Qt6::Svg
)
set_target_properties(EqPGUI PROPERTIES AUTOMOC ON)

if(WIN32)
    # Put the Qt runtime next to EqPGUI.exe *in the build tree*, so F5 in VSCode
    # launches without Qt's bin on PATH. This is separate from the install-time
    # deployment further down; that one only populates the CPack archive.
    #
    # windeployqt rather than copying $<TARGET_RUNTIME_DLLS:EqPGUI>, because the
    # DLLs alone are not enough. Qt 6 is relocatable, so QLibraryInfo derives its
    # prefix from wherever Qt6Core.dll sits; copying that into the build tree
    # stops Qt finding plugins back in the Qt install, and it aborts at startup
    # with "could not find or load the Qt platform plugin windows". windeployqt
    # also lays down the plugin directories (platforms/, styles/, tls/, ...),
    # which Qt then picks up automatically from beside the executable.
    #
    # Per-config by construction: TARGET_FILE_DIR is the config's own output
    # directory and --debug/--release selects the matching Qt variant, so the
    # Debug, Release and RelWithDebInfo trees each get their own correct runtime
    # (Ninja Multi-Config builds all three side by side).
    add_custom_command(TARGET EqPGUI POST_BUILD
        COMMAND Qt6::windeployqt
                --no-translations
                --no-compiler-runtime
                $<IF:$<CONFIG:Debug>,--debug,--release>
                "$<TARGET_FILE:EqPGUI>"
        COMMENT "Deploying Qt runtime beside EqPGUI.exe"
        VERBATIM
    )
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(EqPTest EqPTest.cpp)
    target_link_libraries(EqPTest PRIVATE equation_parser_core)
    add_test(NAME EquationParserTests COMMAND EqPTest)
endif()

include(GNUInstallDirs)
install(TARGETS EqPGUI EqPCLI RUNTIME DESTINATION .)
install(FILES LICENSE THIRD_PARTY_NOTICES.md DESTINATION .)
install(DIRECTORY LICENSES DESTINATION .)

if(WIN32)
    qt_generate_deploy_app_script(
        TARGET EqPGUI
        OUTPUT_SCRIPT deploy_qt
        NO_TRANSLATIONS
        NO_COMPILER_RUNTIME
    )
    install(SCRIPT "${deploy_qt}")
endif()

set(CPACK_PACKAGE_NAME "EquationParser")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_GENERATOR "ZIP")
include(CPack)
