cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(webview_launch VERSION 1.0
                       DESCRIPTION "Test launching WebView2 in parallel to other app code"
                       LANGUAGES CXX)

if(POLICY CMP0135)
	cmake_policy(SET CMP0135 NEW)
	set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
  wil
  URL  https://www.nuget.org/api/v2/package/Microsoft.Windows.ImplementationLibrary
)
FetchContent_Declare(
  webview2
  URL  https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2
)
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(wil webview2 json)


set(BOOST_INCLUDE_LIBRARIES nowide)
set(BOOST_ENABLE_CMAKE ON)

include(FetchContent)
FetchContent_Declare(
  Boost
  GIT_REPOSITORY https://github.com/boostorg/boost.git
  GIT_TAG boost-1.80.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Boost)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(SYSTEM "${wil_SOURCE_DIR}/include/")
include_directories(SYSTEM "${webview2_SOURCE_DIR}/build/native/include/")

set(ARCHS_64BIT_INTEL "amd64" "x86_64" "x64")
set(ARCHS_64BIT_ARM "arm64" "aarch64")
set(ARCHS_32BIT_INTEL "x86" "i686")

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _SYSTEM_PROCESSOR_LOWERED)

list(FIND ARCHS_64BIT_INTEL "${_SYSTEM_PROCESSOR_LOWERED}" _list_idx)
if (${_list_idx} GREATER -1)
    set(WEBVIEW2_LOADER_ARCH "x64")
endif (${_list_idx} GREATER -1)

list(FIND ARCHS_64BIT_ARM "${_SYSTEM_PROCESSOR_LOWERED}" _list_idx)
if (${_list_idx} GREATER -1)
    set(WEBVIEW2_LOADER_ARCH "arm64")
endif (${_list_idx} GREATER -1)

list(FIND ARCHS_32BIT_INTEL "${_SYSTEM_PROCESSOR_LOWERED}" _list_idx)
if (${_list_idx} GREATER -1)
    set(WEBVIEW2_LOADER_ARCH "x86")
endif (${_list_idx} GREATER -1)

message(STATUS "WEBVIEW2_LOADER_ARCH: ${WEBVIEW2_LOADER_ARCH}")
link_libraries("${webview2_SOURCE_DIR}/build/native/${WEBVIEW2_LOADER_ARCH}/WebView2LoaderStatic.lib")

add_library(
  webview_prelaunch
  webview_creation_arguments.cpp
  webview_creation_arguments.hpp
  webview_prelaunch_controller_win.cpp
  webview_prelaunch_controller_win.hpp
  webview_prelaunch_controller.cpp
  webview_prelaunch_controller.hpp
)
#target_link_options(webview_launch PRIVATE "/SUBSYSTEM:WINDOWS")
target_link_libraries(webview_prelaunch PUBLIC nlohmann_json::nlohmann_json Boost::nowide)

add_executable(
  webview_prelaunch_demo
  webview_prelaunch_demo.cpp
)
target_link_libraries(webview_prelaunch_demo PRIVATE webview_prelaunch)
target_link_libraries(webview_prelaunch_demo PRIVATE "Shcore.lib" "runtimeobject.lib")

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  webview_prelaunch_test_win
  webview_prelaunch_test_win.cpp
)
target_link_libraries(
  webview_prelaunch_test_win
  GTest::gtest_main
  webview_prelaunch
  Boost::nowide
  "runtimeobject.lib"
)

include(GoogleTest)
gtest_discover_tests(webview_prelaunch_test_win)
