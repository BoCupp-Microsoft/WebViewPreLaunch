#include <gtest/gtest.h>
#include <sstream>
#include "webview_prelaunch.h"

TEST(PreLaunchTest, CacheAndReadWebViewCreationArguments) {
    // Setup test data
    WebViewCreationArguments args;
    args.browser_exe_path = L"C:\\test\\chrome.exe";
    args.user_data_dir = L"C:\\test\\user_data";
    args.additional_browser_arguments = L"--test-arg";
    
    // Cache to stringstream
    std::stringstream stream;
    WebViewPreLaunch::CacheWebViewCreationArguments(stream, args);

    // Read back from same stream
    auto read_args = WebViewPreLaunch::ReadCachedWebViewCreationArguments(stream);
    ASSERT_TRUE(read_args.has_value());

    // Verify data matches
    EXPECT_EQ(read_args->browser_exe_path, args.browser_exe_path);
    EXPECT_EQ(read_args->user_data_dir, args.user_data_dir);
    EXPECT_EQ(read_args->additional_browser_arguments, args.additional_browser_arguments);
}

TEST(PreLaunchTest, ReadInvalidJson) {
    // Test with invalid JSON data
    std::stringstream stream;
    stream << "{ invalid json }";
    
    auto read_args = WebViewPreLaunch::ReadCachedWebViewCreationArguments(stream);
    EXPECT_FALSE(read_args.has_value());
}
