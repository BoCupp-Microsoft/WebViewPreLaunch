cmake_minimum_required(VERSION 3.15...3.31)

project(webview_launch VERSION 1.0
                       DESCRIPTION "Test launching WebView2 in parallel to other app code"
                       LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  wil
  URL  https://www.nuget.org/api/v2/package/Microsoft.Windows.ImplementationLibrary
)
FetchContent_Declare(
  webview2
  URL  https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2
)
FetchContent_MakeAvailable(wil webview2)

include_directories(SYSTEM "${wil_SOURCE_DIR}/include/")
include_directories(SYSTEM "${webview2_SOURCE_DIR}/build/native/include/")

set(PLATFORM "arm64")
link_libraries("${webview2_SOURCE_DIR}/build/native/${PLATFORM}/WebView2LoaderStatic.lib")

add_executable(
  webview_launch
  webview_launch.cpp
  webview_launch.h
  webview_launch_main.cpp
)
#target_link_options(webview_launch PRIVATE "/SUBSYSTEM:WINDOWS")


# # Set vcpkg toolchain file
# set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
# set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)



# include(FetchContent)
# FetchContent_Declare(
#   googletest
#   URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
# )
# # For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)

# enable_testing()

# add_executable(
#   webview_launch_test
#   webview_launch.cpp
#   webview_launch_test.cpp
# )
# target_link_libraries(
#   webview_launch_test
#   GTest::gtest_main
# )

# include(GoogleTest)
# gtest_discover_tests(webview_launch_test)


