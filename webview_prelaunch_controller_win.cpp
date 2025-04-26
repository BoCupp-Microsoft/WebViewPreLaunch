#include "webview_prelaunch_controller_win.hpp"

#include <boost/nowide/convert.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace Microsoft::WRL;

// Minimal Window Procedure function to facilitate WebView2 creation
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            std::cout << "Pre-launch background window destroyed" << std::endl;
            PostQuitMessage(/*nExitCode*/0);
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


namespace {
template <class T>
class AutoReset {
public:
    explicit AutoReset(T& t_) : t(t_) {}
    ~AutoReset() {
        t.reset();
    }
private:
    T& t;
};

template <class T>
class AutoRelease {
public:
    explicit AutoRelease(T& t_) : t(t_) {}
    ~AutoRelease() {
        t.release();
    }
private:
    T& t;
};
}  // namespace

void WebViewPreLaunch::Launch(const std::string& cache_args_path) {
    launch_thread_ = std::thread([this, cache_args_path]() {
        std::cout << "Background pre-launch is starting" << std::endl;
        AutoReset<decltype(webView_)> auto_reset_webview(webView_);
        AutoReset<decltype(webViewController_)> auto_reset_webview_controller(webViewController_);
        AutoRelease<decltype(semaphore_)> auto_release_semaphore(semaphore_);

        if (!LaunchBackground(cache_args_path)) {
            return;
        }

        // Run the message loop
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (background_thread_should_exit_) {
                std::cout << "Pre-launch message loop is exiting" << std::endl;
                break;
            }
        }

        std::cout << "Background pre-launch is exiting" << std::endl;
        if (wait_for_browser_process_exit_) {
            std::cout << "Waiting for browser process " << browser_process_id_ << " exit" << std::endl;
            HANDLE browser_process_handle = ::OpenProcess(SYNCHRONIZE , false, browser_process_id_);
            if (browser_process_handle == nullptr) {
                std::cerr << "Failed to open browser process handle: " << GetLastError() << std::endl;
                return;
            }

            webView_.reset();
            webViewController_.reset();    
            WaitForSingleObject(browser_process_handle, INFINITE);
            std::cout << "Browser process exited" << std::endl;
        }
    });
}

bool WebViewPreLaunch::LaunchBackground(const std::string& cache_args_path) {
    std::ifstream cache_file(cache_args_path);
    if (!cache_file) {
        std::cerr << "Could not read cache file path" << std::endl;
        return false;
    }

    auto args = ReadCachedWebViewCreationArguments(cache_file);
    if (!args.has_value()) {
        std::cerr << "Failed to read cached args" << std::endl;
        return false;
    }

    HRESULT hr = RoInitialize(RO_INIT_SINGLETHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM: " << std::hex << hr << std::endl;
        return false;
    }

    backgroundHwnd_ = CreateMessageWindow();
    if (backgroundHwnd_ == nullptr) {
        std::cerr << "Failed to create window" << std::endl;
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
    
    std::wstring browser_exe_path = boost::nowide::widen(args->browser_exe_path);
    std::wstring user_data_dir = boost::nowide::widen(args->user_data_dir);
    std::wstring additional_browser_arguments = boost::nowide::widen(args->additional_browser_arguments);
    std::wstring language = boost::nowide::widen(args->language);

    options->put_AdditionalBrowserArguments(additional_browser_arguments.c_str());
    options->put_EnableTrackingPrevention(args->enableTrackingPrevention);
    options->put_Language(language.c_str());

    hr = CreateCoreWebView2EnvironmentWithOptions(      
        browser_exe_path.c_str(), 
        user_data_dir.c_str(),
        options.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                return EnvironmentCreatedCallback(result, env);
            }).Get());

    if (FAILED(hr)) {
        std::cerr << "Failed to create WebView2 environment: " << std::hex << hr << std::endl;
        return false;
    }

    return true;
}

HRESULT WebViewPreLaunch::EnvironmentCreatedCallback(HRESULT result, ICoreWebView2Environment* env) {
    if (FAILED(result)) {
        std::cerr << "Failed to create WebView2 environment: " << std::hex << result << std::endl;
        PostMessage(backgroundHwnd_, WM_CLOSE, 0, 0);
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
        PostMessage(backgroundHwnd_, WM_CLOSE, 0, 0);
        return hr;
    }

    return S_OK;
}

HRESULT WebViewPreLaunch::ControllerCreatedCallback(HRESULT result, ICoreWebView2Controller* controller) {
    if (FAILED(result)) {
        std::cerr << "Failed to create WebView2 controller: " << std::hex << result << std::endl;
        PostMessage(backgroundHwnd_, WM_CLOSE, 0, 0);
        return result;
    }

    webViewController_ = controller;
    webViewController_->get_CoreWebView2(&webView_);
    if (webView_ == nullptr) {
        std::cerr << "Failed to get WebView2 instance" << std::endl;
        PostMessage(backgroundHwnd_, WM_CLOSE, 0, 0);
        return E_FAIL;
    }

    webView_->get_BrowserProcessId(&browser_process_id_);
    if (browser_process_id_ == 0) {
        std::cerr << "Failed to get browser process ID" << std::endl;
        PostMessage(backgroundHwnd_, WM_CLOSE, 0, 0);
        return E_FAIL;
    }

    // Release the semaphore to indicate that the WebView is ready
    std::cerr << "Pre-launched WebView is ready" << std::endl;
    semaphore_.release();
    return S_OK;
}

void WebViewPreLaunch::Close(bool wait_for_browser_process_exit) {
    wait_for_browser_process_exit_ = wait_for_browser_process_exit;
    background_thread_should_exit_ = true;

    std::cout << "Posting WM_CLOSE to (" << std::hex << backgroundHwnd_ << ") for backgroundHwnd_" << std::endl;
    PostMessage(backgroundHwnd_, WM_CLOSE, 0, 0);
}

void WebViewPreLaunch::WaitForClose() {
    if (launch_thread_.joinable()) {
        launch_thread_.join();
    }
}

void WebViewPreLaunch::WaitForLaunch() {
    semaphore_.acquire();
    semaphore_.release();
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
