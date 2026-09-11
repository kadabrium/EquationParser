cmake_minimum_required(VERSION 3.24)

project(EquationParser VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    pegtl
    GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
    GIT_TAG 3.2.8
    GIT_SHALLOW TRUE
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
