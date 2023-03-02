cmake_minimum_required(VERSION 3.24)
project(infinitetrain VERSION 0.1.0.0 LANGUAGES CXX)

include(cmake/CPM.cmake)

CPMAddPackage(
    NAME              spdlog
    GIT_TAG           v1.10.0
    GITHUB_REPOSITORY gabime/spdlog
)

CPMAddPackage(
    NAME              args
    GIT_TAG           6.4.6
    GITHUB_REPOSITORY Taywee/args
    OPTIONS           "ARGS_BUILD_EXAMPLE OFF" "ARGS_BUILD_UNITTESTS OFF"
)

CPMAddPackage(
    NAME              yaml-cpp
    GIT_TAG           yaml-cpp-0.7.0
    GITHUB_REPOSITORY jbeder/yaml-cpp
    OPTIONS           "YAML_CPP_BUILD_TOOLS OFF" "YAML_CPP_BUILD_CONTRIB OFF"
)

CPMAddPackage(
    NAME              flac
    GIT_TAG           1.4.2
    GITHUB_REPOSITORY xiph/flac
    OPTIONS           "BUILD_PROGRAMS OFF" "BUILD_EXAMPLES OFF" "BUILD_TESTING OFF" "BUILD_DOCS OFF"
                      "INSTALL_MANPAGES OFF" "INSTALL_PKGCONFIG_MODULES OFF" "INSTALL_CMAKE_CONFIG_MODULE OFF"
                      "WITH_OGG OFF" "INSTALL_MANPAGES OFF"
)

add_executable(${PROJECT_NAME} src/main.cc)

target_link_libraries(${PROJECT_NAME} PRIVATE FLAC++ spdlog args yaml-cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PRIVATE VC_EXTRALEAN WIN32_LEAN_AND_MEAN)