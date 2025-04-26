#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <istream>
#include <optional>
#include <ostream>
#include <semaphore>
#include <string>
#include <thread>

#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>
#include <wil/com.h>
#include <windows.h>
#include <wrl.h>
#include <wrl/event.h>

#include "webview_creation_arguments.hpp"
#include "webview_prelaunch_controller.hpp"

class WebViewPreLaunch : public  WebViewPreLaunchController {
private:
    wil::com_ptr<ICoreWebView2> webView_;
    wil::com_ptr<ICoreWebView2Controller> webViewController_;
    std::binary_semaphore semaphore_;
    std::thread launch_thread_;
    std::atomic<bool> background_thread_should_exit_ = false;
    std::atomic<bool> wait_for_browser_process_exit_ = false;
    HWND backgroundHwnd_ = nullptr;
    uint32_t browser_process_id_ = 0;

    bool LaunchBackground(const std::string& cache_args_path);
    HWND CreateMessageWindow();
    HRESULT EnvironmentCreatedCallback( HRESULT result, ICoreWebView2Environment* env);
    HRESULT ControllerCreatedCallback(HRESULT result, ICoreWebView2Controller* controller);
    
public:
    WebViewPreLaunch();

    void Launch(const std::string& cache_args_path);
    void WaitForLaunch() override;
    void Close(bool wait_for_browser_process_exit) override;
    void WaitForClose() override;

    std::optional<WebViewCreationArguments> ReadCachedWebViewCreationArguments(const std::string& cache_args_path) override;
    void CacheWebViewCreationArguments(const std::string& cache_args_path, const WebViewCreationArguments& args) override;

    // Exposed public for testing
    static std::optional<WebViewCreationArguments> ReadCachedWebViewCreationArguments(std::istream& stream);
    static void CacheWebViewCreationArguments(std::ostream& stream, const WebViewCreationArguments& args);
};
