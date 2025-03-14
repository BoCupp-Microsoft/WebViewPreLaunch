#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "webview_prelaunch.h"

using json = nlohmann::json;
using namespace Microsoft::WRL;

// Minimal Window Procedure function to facilitate WebView2 creation
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

WebViewPreLaunch::WebViewPreLaunch() : semaphore_(0) {}

HWND WebViewPreLaunch::CreateMessageWindow() {
    // Define the window class
    const char CLASS_NAME[] = "WebViewPreLaunchClass";
    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "WebView PreLaunch",
        0, 0, 0, 0, 0,
        HWND_MESSAGE,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (hwnd == NULL) {
        std::cerr << "Failed to create window" << std::endl;
        return nullptr;
    }

    return hwnd;
}


void WebViewPreLaunch::Launch(const std::string& cache_args_path) {
    launch_thread_ = std::thread([this, cache_args_path]() {
        if (!LaunchBackground(cache_args_path)) {
            semaphore_.release();
            return;
        }

        // Run the message loop
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (background_thread_should_exit_) {
                semaphore_.release();
                break;
            }
        }
    });
}

bool WebViewPreLaunch::LaunchBackground(const std::string& cache_args_path) {
    std::ifstream cache_file(cache_args_path);
    if (!cache_file) {
        std::cerr << "Could not read cache file path" << std::endl;
        semaphore_.release();
        return false;
    }

    auto args = ReadCachedWebViewCreationArguments(cache_file);
    if (!args.has_value()) {
        std::cerr << "Failed to read cached args" << std::endl;
        semaphore_.release();
        return false;
    }

    HRESULT hr = RoInitialize(RO_INIT_SINGLETHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM: " << std::hex << hr << std::endl;
        semaphore_.release();
        return false;
    }

    backgroundHwnd_ = CreateMessageWindow();
    if (backgroundHwnd_ == NULL) {
        std::cerr << "Failed to create window" << std::endl;
        semaphore_.release();
        return false;
    }

    // Initialize WebView2
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    options->put_AllowSingleSignOnUsingOSPrimaryAccount(true);

    Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions7> options7;
    options.As(&options7);
    if (options7) {
        options7->put_ChannelSearchKind(static_cast<COREWEBVIEW2_CHANNEL_SEARCH_KIND>(args->channelSearchKind));
        options7->put_ReleaseChannels(static_cast<COREWEBVIEW2_RELEASE_CHANNELS>(args->releaseChannelsMask));
    }
    
    options->put_AdditionalBrowserArguments(args->additional_browser_arguments.c_str());
    options->put_EnableTrackingPrevention(args->enableTrackingPrevention);
    options->put_Language(args->language.c_str());

    hr = CreateCoreWebView2EnvironmentWithOptions(      
        nullptr, 
        args->user_data_dir.c_str(),
        options.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                return EnvironmentCreatedCallback(result, env);
            }).Get());

    if (FAILED(hr)) {
        std::cerr << "Failed to create WebView2 environment: " << std::hex << hr << std::endl;
        semaphore_.release();
        return false;
    }

    return true;
}

HRESULT WebViewPreLaunch::EnvironmentCreatedCallback(HRESULT result, ICoreWebView2Environment* env) {
    if (FAILED(result)) {
        std::cerr << "Failed to create WebView2 environment: " << std::hex << result << std::endl;
        background_thread_should_exit_ = true;
        return result;
    }

    // Create the WebView using the default profile
    HRESULT hr = env->CreateCoreWebView2Controller(
        backgroundHwnd_,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                return ControllerCreatedCallback(result, controller);
            }).Get());

    if (FAILED(hr)) {
        std::cerr << "Failed to create WebView2 controller: " << std::hex << hr << std::endl;
        background_thread_should_exit_ = true;
        return hr;
    }

    return S_OK;
}

HRESULT WebViewPreLaunch::ControllerCreatedCallback(HRESULT result, ICoreWebView2Controller* controller) {
    if (FAILED(result)) {
        std::cerr << "Failed to create WebView2 controller: " << std::hex << result << std::endl;
        background_thread_should_exit_ = true;
        return result;
    }

    webViewController_ = controller;
    webViewController_->get_CoreWebView2(&webView_);
    if (webView_ == nullptr) {
        std::cerr << "Failed to get WebView2 instance" << std::endl;
        background_thread_should_exit_ = true;
        return E_FAIL;
    }

    // Release the semaphore to indicate that the WebView is ready
    semaphore_.release();
    return S_OK;
}

void WebViewPreLaunch::Close() {
    
}

bool WebViewPreLaunch::WaitForLaunch(std::chrono::seconds timeout) {
    return semaphore_.try_acquire_for(timeout);
}

void WebViewPreLaunch::CacheWebViewCreationArguments(const std::string& cache_args_path, const WebViewCreationArguments& args) {
    std::ofstream cache_file(cache_args_path);
    if (!cache_file) {
        std::cerr << "Could not open cache file for writing" << std::endl;
        return;
    }
    
    CacheWebViewCreationArguments(cache_file, args);
}

std::optional<WebViewCreationArguments> WebViewPreLaunch::ReadCachedWebViewCreationArguments(const std::string& cache_args_path) {
    std::ifstream cache_file(cache_args_path);
    if (!cache_file) {
        std::cerr << "Could not read cache file" << std::endl;
        return std::nullopt;
    }

    return ReadCachedWebViewCreationArguments(cache_file);
}

/*static*/
void WebViewPreLaunch::CacheWebViewCreationArguments(std::ostream& stream, const WebViewCreationArguments& args) {
    json j(args);
    stream << j;
}

/*static*/
std::optional<WebViewCreationArguments> WebViewPreLaunch::ReadCachedWebViewCreationArguments(std::istream& stream) {
    try {
        json j;
        stream >> j;
        return j.get<WebViewCreationArguments>();
    } catch (const json::exception&) {
        return std::nullopt;
    }
}
