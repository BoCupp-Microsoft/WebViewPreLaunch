#include <gtest/gtest.h>
#include <sstream>
#include <filesystem>
#include <fstream>
#include "webview_prelaunch_controller.hpp"
#include "webview_prelaunch_controller_win.hpp"

namespace {
    // Helper method to get a temp path for the prelaunch config
    std::string CreateTempPrelaunchConfigPath() {
        auto temp_path = std::filesystem::temp_directory_path() / "webviewprelaunch_test" / 
            std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::filesystem::create_directories(temp_path);

        auto json_path = temp_path / "test_config.json";
        return json_path.string();
    }

    // Helper method to get a temp path for user data dir
    std::string CreateTempUserDataPath() {
        auto temp_path = std::filesystem::temp_directory_path() / "webviewprelaunch_test" / "user_data" /
            std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::filesystem::create_directories(temp_path);

        return temp_path.string();
    }
}

TEST(PreLaunchTest, CacheAndReadWebViewCreationArguments) {
    WebViewCreationArguments args;
    args.browser_exe_path = "C:\\test\\chrome.exe";
    args.user_data_dir = "C:\\test\\user_data";
    args.additional_browser_arguments = "--test-arg";

    WebviewPrelaunchControllerWin controller;

    // Cache prelaunch args
    auto prelaunch_config_path = CreateTempPrelaunchConfigPath();
    controller.CacheWebViewCreationArguments(prelaunch_config_path, args);

    // Read back from same file
    auto read_args = controller.ReadCachedWebViewCreationArguments(prelaunch_config_path);

    // Verify data matches
    EXPECT_EQ(read_args->browser_exe_path, args.browser_exe_path);
    EXPECT_EQ(read_args->user_data_dir, args.user_data_dir);
    EXPECT_EQ(read_args->additional_browser_arguments, args.additional_browser_arguments);
}

TEST(PreLaunchTest, ReadInvalidJson) {
    auto prelaunch_config_path = CreateTempPrelaunchConfigPath();
    std::ofstream prelaunch_config(prelaunch_config_path);
    prelaunch_config.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    prelaunch_config << "{ invalid json data }";

    WebviewPrelaunchControllerWin controller;
    auto read_args = controller.ReadCachedWebViewCreationArguments(prelaunch_config_path);
    ASSERT_TRUE(!read_args.has_value());
}

TEST(PreLaunchTest, Launch) {
    auto prelaunch_config_path = CreateTempPrelaunchConfigPath();
    std::ofstream prelaunch_config(prelaunch_config_path);
    prelaunch_config.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    
    auto user_data_dir = CreateTempUserDataPath();
    WebViewCreationArguments args;
    args.browser_exe_path = "";
    args.user_data_dir = user_data_dir;
    args.additional_browser_arguments = "";
    args.language = "en-US";
    args.releaseChannelsMask = 0xf;
    args.channelSearchKind = 0x0;
    args.enableTrackingPrevention = false;

    WebviewPrelaunchControllerWin::CacheWebViewCreationArguments(prelaunch_config, args);
    prelaunch_config.close();

    auto controller = WebviewPrelaunchController::Launch(prelaunch_config_path);
    controller->WaitForLaunch();
    EXPECT_NE(static_cast<WebviewPrelaunchControllerWin*>(controller.get())->GetBrowserProcessId(), 0);

    controller->Close(true);
    controller->WaitForClose();
}

TEST(PreLaunchTest, Launch2) {
    auto prelaunch_config_path = CreateTempPrelaunchConfigPath();
    std::ofstream prelaunch_config(prelaunch_config_path);
    prelaunch_config.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    
    auto user_data_dir = CreateTempUserDataPath();
    WebViewCreationArguments args;
    args.browser_exe_path = "";
    args.user_data_dir = user_data_dir;
    args.additional_browser_arguments = "";
    args.language = "en-US";
    args.releaseChannelsMask = 0xf;
    args.channelSearchKind = 0x0;
    args.enableTrackingPrevention = false;

    WebviewPrelaunchControllerWin::CacheWebViewCreationArguments(prelaunch_config, args);
    prelaunch_config.close();
    
    auto controller = WebviewPrelaunchController::Launch(prelaunch_config_path);
    auto controller2 = WebviewPrelaunchController::Launch(prelaunch_config_path);
    controller->WaitForLaunch();
    EXPECT_NE(static_cast<WebviewPrelaunchControllerWin*>(controller.get())->GetBrowserProcessId(), 0);
    controller2->WaitForLaunch();
    EXPECT_NE(static_cast<WebviewPrelaunchControllerWin*>(controller2.get())->GetBrowserProcessId(), 0);
    EXPECT_EQ(static_cast<WebviewPrelaunchControllerWin*>(controller.get())->GetBrowserProcessId(), static_cast<WebviewPrelaunchControllerWin*>(controller2.get())->GetBrowserProcessId());

    controller->Close(false);
    controller2->Close(false);

    controller->WaitForClose();
    controller2->WaitForClose();
}

TEST(PreLaunchTest, LaunchWithInvalidJson) {
    auto prelaunch_config_path = CreateTempPrelaunchConfigPath();
    std::ofstream prelaunch_config(prelaunch_config_path);
    prelaunch_config.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    prelaunch_config << "{ invalid json data }";
    prelaunch_config.close();
    
    auto controller = WebviewPrelaunchController::Launch(prelaunch_config_path);
    controller->WaitForLaunch();
    EXPECT_EQ(static_cast<WebviewPrelaunchControllerWin*>(controller.get())->GetBrowserProcessId(), 0);
    
    controller->Close(false);
    controller->WaitForClose();
}

TEST(PreLaunchTest, LaunchWithInitiallyDifferentArgs) {
    auto prelaunch_config_path = CreateTempPrelaunchConfigPath();
    std::ofstream prelaunch_config(prelaunch_config_path);
    prelaunch_config.exceptions(std::ofstream::failbit | std::ofstream::badbit);

    auto prelaunch_config_path2 = CreateTempPrelaunchConfigPath();
    std::ofstream prelaunch_config2(prelaunch_config_path2);
    prelaunch_config2.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    
    auto user_data_dir = CreateTempUserDataPath();
    WebViewCreationArguments args;
    args.browser_exe_path = "";
    args.user_data_dir = user_data_dir;
    args.additional_browser_arguments = "";
    args.language = "en-US";
    args.releaseChannelsMask = 0xf;
    args.channelSearchKind = 0x0;
    args.enableTrackingPrevention = false;

    WebviewPrelaunchControllerWin::CacheWebViewCreationArguments(prelaunch_config, args);
    prelaunch_config.close();

    args.additional_browser_arguments = "--disable-features=V8Maglev";
    WebviewPrelaunchControllerWin::CacheWebViewCreationArguments(prelaunch_config2, args);
    prelaunch_config2.close();
    
    auto controller = WebviewPrelaunchController::Launch(prelaunch_config_path);
    controller->WaitForLaunch();
    EXPECT_NE(static_cast<WebviewPrelaunchControllerWin*>(controller.get())->GetBrowserProcessId(), 0);
    // If args are not equal the host should do the following
    controller->CacheWebViewCreationArguments(prelaunch_config_path, args);
    controller->Close(true);
    controller->WaitForClose();

    // Now we can successfully launch the second instance with different args
    auto controller2 = WebviewPrelaunchController::Launch(prelaunch_config_path2);
    controller2->WaitForLaunch();
    EXPECT_NE(static_cast<WebviewPrelaunchControllerWin*>(controller2.get())->GetBrowserProcessId(), 0);

    // Clean everything up.  Notes that the Close and WaitForClose on the first controller
    // are not strictly necessary, but are likely to be called by the host so we don't need to track
    // whether they were closed earlier.  Calling it again is harmless.
    controller->Close(false);
    controller2->Close(false);
    controller->WaitForClose();
    controller2->WaitForClose();
}