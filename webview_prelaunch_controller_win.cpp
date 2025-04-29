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

WebviewPrelaunchControllerWin::WebviewPrelaunchControllerWin() : semaphore_(0) {}

HWND WebviewPrelaunchControllerWin::CreateMessageWindow() {
    // Define the window class
    const char CLASS_NAME[] = "WebviewPrelaunchClass";
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
    THROW_LAST_ERROR_IF_NULL(hwnd);

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

void WebviewPrelaunchControllerWin::Launch(const std::string& cache_args_path) {
    cache_args_path_ = cache_args_path;

    launch_thread_ = std::thread([this, cache_args_path]() {
        this->LaunchBackground(cache_args_path);
    });
}

void WebviewPrelaunchControllerWin::LaunchBackground(const std::string& cache_args_path) noexcept try {
    std::cout << "Background pre-launch is starting" << std::endl;
    AutoReset<decltype(webview_)> auto_reset_webview(webview_);
    AutoReset<decltype(webviewController_)> auto_reset_webview_controller(webviewController_);
    AutoRelease<decltype(semaphore_)> auto_release_semaphore(semaphore_);

    std::ifstream cache_file(cache_args_path);
    cache_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    auto args = ReadCachedWebViewCreationArguments(cache_file);

    THROW_IF_FAILED(RoInitialize(RO_INIT_SINGLETHREADED));
    backgroundHwnd_ = CreateMessageWindow();
    
    // Initialize WebView2
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    THROW_IF_NULL_ALLOC(options);
    THROW_IF_FAILED(options->put_AllowSingleSignOnUsingOSPrimaryAccount(true));

    Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions7> options7;
    options.As(&options7);
    THROW_IF_NULL_ALLOC(options7);
    THROW_IF_FAILED(options7->put_ChannelSearchKind(static_cast<COREWEBVIEW2_CHANNEL_SEARCH_KIND>(args.channelSearchKind)));
    THROW_IF_FAILED(options7->put_ReleaseChannels(static_cast<COREWEBVIEW2_RELEASE_CHANNELS>(args.releaseChannelsMask)));
    
    std::wstring browser_exe_path = boost::nowide::widen(args.browser_exe_path);
    std::wstring user_data_dir = boost::nowide::widen(args.user_data_dir);
    std::wstring additional_browser_arguments = boost::nowide::widen(args.additional_browser_arguments);
    std::wstring language = boost::nowide::widen(args.language);

    THROW_IF_FAILED(options->put_AdditionalBrowserArguments(additional_browser_arguments.c_str()));
    THROW_IF_FAILED(options->put_EnableTrackingPrevention(args.enableTrackingPrevention));
    THROW_IF_FAILED(options->put_Language(language.c_str()));

    THROW_IF_FAILED(CreateCoreWebView2EnvironmentWithOptions(      
        browser_exe_path.c_str(), 
        user_data_dir.c_str(),
        options.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                return EnvironmentCreatedCallback(result, env);
            }).Get()));

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
        THROW_LAST_ERROR_IF_NULL(browser_process_handle);

        webview_.reset();
        webviewController_.reset();
        WaitForSingleObject(browser_process_handle, INFINITE);
        std::cout << "Browser process exited" << std::endl;
    }    
}
catch(...) {
    // prevent exceptional launch thread behavior from terminating process
}

HRESULT WebviewPrelaunchControllerWin::EnvironmentCreatedCallback(HRESULT result, ICoreWebView2Environment* env) noexcept try {
    THROW_IF_FAILED(result);

    // Create the WebView using the default profile
    HRESULT hr = env->CreateCoreWebView2Controller(
        backgroundHwnd_,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                return ControllerCreatedCallback(result, controller);
            }).Get());
    THROW_IF_FAILED(hr);

    return S_OK;
}
catch(...) {
    PostMessage(backgroundHwnd_, WM_CLOSE, 0, 0);
    background_thread_should_exit_ = true;
    RETURN_CAUGHT_EXCEPTION();
}

HRESULT WebviewPrelaunchControllerWin::ControllerCreatedCallback(HRESULT result, ICoreWebView2Controller* controller) noexcept try {
    THROW_IF_FAILED(result);

    webviewController_ = controller;
    THROW_IF_FAILED(webviewController_->get_CoreWebView2(&webview_));
    
    THROW_IF_FAILED(webview_->get_BrowserProcessId(&browser_process_id_));
    
    // Release the semaphore to indicate that the WebView is ready
    semaphore_.release();
    return S_OK;
}
catch(...) {
    PostMessage(backgroundHwnd_, WM_CLOSE, 0, 0);
    background_thread_should_exit_ = true;
    RETURN_CAUGHT_EXCEPTION();
}

void WebviewPrelaunchControllerWin::Close(bool wait_for_browser_process_exit) {
    wait_for_browser_process_exit_ = wait_for_browser_process_exit;
    background_thread_should_exit_ = true;

    if (backgroundHwnd_ == nullptr) {
        std::cout << "Background HWND is null" << std::endl;
        return;
    }

    std::cout << "Posting WM_CLOSE to (" << std::hex << backgroundHwnd_ << ") for backgroundHwnd_" << std::endl;
    PostMessage(backgroundHwnd_, WM_CLOSE, 0, 0);
}

void WebviewPrelaunchControllerWin::WaitForClose() {
    if (launch_thread_.joinable()) {
        launch_thread_.join();
    }
}

void WebviewPrelaunchControllerWin::WaitForLaunch() {
    semaphore_.acquire();
    semaphore_.release();
}

void WebviewPrelaunchControllerWin::CacheWebViewCreationArguments(const std::string& cache_args_path, const WebViewCreationArguments& args) noexcept try {
    std::ofstream cache_file(cache_args_path);
    cache_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    
    CacheWebViewCreationArguments(cache_file, args);
}
catch(...) {
}

std::optional<WebViewCreationArguments> WebviewPrelaunchControllerWin::ReadCachedWebViewCreationArguments(const std::string& cache_args_path) noexcept try {
    std::ifstream cache_file(cache_args_path);
    cache_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    return ReadCachedWebViewCreationArguments(cache_file);
}
catch(...) {
    return std::nullopt;
}

/*static*/
void WebviewPrelaunchControllerWin::CacheWebViewCreationArguments(std::ostream& stream, const WebViewCreationArguments& args) {
    json j(args);
    stream << j;
}

/*static*/
WebViewCreationArguments WebviewPrelaunchControllerWin::ReadCachedWebViewCreationArguments(std::istream& stream) {
    json j;
    stream >> j;
    return j.get<WebViewCreationArguments>();
}

uint32_t WebviewPrelaunchControllerWin::GetBrowserProcessId() const {
    return browser_process_id_;
}
