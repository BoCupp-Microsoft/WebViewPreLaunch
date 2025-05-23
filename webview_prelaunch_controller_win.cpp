#include "webview_prelaunch_controller_win.hpp"

#include <boost/nowide/convert.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/* static */
std::shared_ptr<WebViewPreLaunchController> WebViewPreLaunchController::Launch(const std::filesystem::path& cache_args_path) {
    auto webview_prelaunch = std::make_shared<WebViewPreLaunchControllerWin>();
    webview_prelaunch->Launch(cache_args_path);
    return webview_prelaunch;
}

void HandleException(const std::exception_ptr& ex, WebViewPreLaunchTelemetry& telemetry, const std::string& unknown_exception_msg) {
    try {
        if (ex) {
            std::rethrow_exception(ex);
        }
    } catch (const std::exception& e) {
        telemetry.exceptions.push_back(e.what());
    } catch (...) {
        telemetry.exceptions.push_back(unknown_exception_msg);
    }
}

// Minimal Window Procedure function to facilitate WebView2 creation
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(/*nExitCode*/0);
            return 0;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

WebViewPreLaunchControllerWin::WebViewPreLaunchControllerWin() : semaphore_(0) {}

WebViewPreLaunchControllerWin::~WebViewPreLaunchControllerWin() {
    if (launch_thread_.joinable()) {
        Close(/*wait_for_browser_process_exit*/false);
        WaitForClose();
    }
}

HWND WebViewPreLaunchControllerWin::CreateMessageWindow() {
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

void WebViewPreLaunchControllerWin::Launch(const std::filesystem::path& cache_args_path) {
    telemetry_.launch_start = std::chrono::high_resolution_clock::now();

    launch_thread_ = std::thread([this, cache_args_path]() {
        this->LaunchBackground(cache_args_path);
    });
}

void WebViewPreLaunchControllerWin::LaunchBackground(const std::filesystem::path& cache_args_path) noexcept {
    telemetry_.background_launch_started = telemetry_.DurationSinceLaunch();
    AutoRelease<decltype(semaphore_)> auto_release_semaphore(semaphore_);

    try {
        AutoReset<decltype(webview_)> auto_reset_webview(webview_);
        AutoReset<decltype(webviewController_)> auto_reset_webview_controller(webviewController_);

        std::ifstream cache_file(cache_args_path);
        cache_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        auto args = ReadCachedWebViewCreationArguments(cache_file);
        telemetry_.read_cached_args_completed = telemetry_.DurationSinceLaunch();

        THROW_IF_FAILED(RoInitialize(RO_INIT_SINGLETHREADED));
        backgroundHwnd_ = CreateMessageWindow();
        telemetry_.window_created = telemetry_.DurationSinceLaunch();
        
        // Initialize WebView2
        auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
        THROW_IF_NULL_ALLOC(options);
        THROW_IF_FAILED(options->put_AllowSingleSignOnUsingOSPrimaryAccount(true));

        Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions7> options7;
        options.As(&options7);
        THROW_IF_NULL_ALLOC(options7);
        THROW_IF_FAILED(options7->put_ChannelSearchKind(static_cast<COREWEBVIEW2_CHANNEL_SEARCH_KIND>(args.channel_search_kind)));
        THROW_IF_FAILED(options7->put_ReleaseChannels(static_cast<COREWEBVIEW2_RELEASE_CHANNELS>(args.release_channels_mask)));
        
        std::wstring browser_exe_path = boost::nowide::widen(args.browser_exe_path);
        std::wstring user_data_dir = boost::nowide::widen(args.user_data_dir);
        std::wstring additional_browser_arguments = boost::nowide::widen(args.additional_browser_arguments);
        std::wstring language = boost::nowide::widen(args.language);

        THROW_IF_FAILED(options->put_AdditionalBrowserArguments(additional_browser_arguments.c_str()));
        THROW_IF_FAILED(options->put_EnableTrackingPrevention(args.enable_tracking_prevention));
        THROW_IF_FAILED(options->put_Language(language.c_str()));

        HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(      
            browser_exe_path.c_str(), 
            user_data_dir.c_str(),
            options.Get(),
            Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                    return EnvironmentCreatedCallback(result, env);
                }).Get());
        THROW_IF_FAILED(hr);

        // Run the message loop
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (background_thread_should_exit_) {
                break;
            }
        }

        if (wait_for_browser_process_exit_) {
            HANDLE browser_process_handle = ::OpenProcess(SYNCHRONIZE , false, browser_process_id_);
            THROW_LAST_ERROR_IF_NULL(browser_process_handle);

            webview_.reset();
            webviewController_.reset();
            WaitForSingleObject(browser_process_handle, INFINITE);
        }    
    }
    catch(...) {
        auto ce = std::current_exception();
        HandleException(ce, telemetry_, "Unknown exception occurred in LaunchBackground");
    }
}

HRESULT WebViewPreLaunchControllerWin::EnvironmentCreatedCallback(HRESULT result, ICoreWebView2Environment* env) noexcept try {
    telemetry_.environment_created = telemetry_.DurationSinceLaunch();
    THROW_IF_FAILED(result);

    // Create the WebView using the default profile
    HRESULT hr = env->CreateCoreWebView2Controller(
        backgroundHwnd_,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                return ControllerCreatedCallback(result, controller);
            }).Get());
    THROW_IF_FAILED(hr);

    return S_OK;
}
catch(...) {
    auto ce = std::current_exception();
    HandleException(ce, telemetry_, "Unknown exception occurred in EnvironmentCreatedCallback");

    PostMessage(backgroundHwnd_, WM_CLOSE, 0, 0);
    background_thread_should_exit_ = true;
    RETURN_CAUGHT_EXCEPTION();
}

HRESULT WebViewPreLaunchControllerWin::ControllerCreatedCallback(HRESULT result, ICoreWebView2Controller* controller) noexcept try {
    telemetry_.controller_created = telemetry_.DurationSinceLaunch();
    THROW_IF_FAILED(result);

    webviewController_ = controller;
    THROW_IF_FAILED(webviewController_->get_CoreWebView2(&webview_));
    
    THROW_IF_FAILED(webview_->get_BrowserProcessId(&browser_process_id_));
    
    // Release the semaphore to indicate that the WebView is ready
    semaphore_.release();
    return S_OK;
}
catch(...) {
    auto ce = std::current_exception();
    HandleException(ce, telemetry_, "Unknown exception occurred in ControllerCreatedCallback");

    PostMessage(backgroundHwnd_, WM_CLOSE, 0, 0);
    background_thread_should_exit_ = true;
    RETURN_CAUGHT_EXCEPTION();
}

void WebViewPreLaunchControllerWin::Close(bool wait_for_browser_process_exit) {
    telemetry_.close_started = telemetry_.DurationSinceLaunch();

    wait_for_browser_process_exit_ = wait_for_browser_process_exit;
    background_thread_should_exit_ = true;

    if (backgroundHwnd_ == nullptr) {
        return;
    }

    PostMessage(backgroundHwnd_, WM_CLOSE, 0, 0);
}

void WebViewPreLaunchControllerWin::WaitForClose() {
    if (launch_thread_.joinable()) {
        launch_thread_.join();
    }
    telemetry_.waitforclose_completed = telemetry_.DurationSinceLaunch();
}

void WebViewPreLaunchControllerWin::WaitForLaunch() {
    telemetry_.waitforlaunch_started = telemetry_.DurationSinceLaunch();
    semaphore_.acquire();
    semaphore_.release();
    telemetry_.waitforlaunch_completed = telemetry_.DurationSinceLaunch();
}

void WebViewPreLaunchControllerWin::CacheWebViewCreationArguments(const std::filesystem::path& cache_args_path, const WebViewCreationArguments& args) noexcept try {
    std::ofstream cache_file(cache_args_path);
    cache_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    
    CacheWebViewCreationArguments(cache_file, args);
    telemetry_.cache_arguments_completed = telemetry_.DurationSinceLaunch();
}
catch(...) {
    auto ce = std::current_exception();
    HandleException(ce, telemetry_, "Unknown exception occurred in CacheWebViewCreationArguments");
}

const std::optional<WebViewCreationArguments>& WebViewPreLaunchControllerWin::ReadCachedWebViewCreationArguments(const std::filesystem::path& cache_args_path) noexcept try {
    std::ifstream cache_file(cache_args_path);
    cache_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    if (!cached_args_.has_value()) {
        cached_args_ = ReadCachedWebViewCreationArguments(cache_file);  
    }
    telemetry_.foreground_read_cached_args_completed = telemetry_.DurationSinceLaunch();
    return cached_args_;
}
catch(...) {
    auto ce = std::current_exception();
    HandleException(ce, telemetry_, "Unknown exception occurred in ReadCachedWebViewCreationArguments");
    cached_args_ = std::nullopt;
    return cached_args_;
}

const WebViewPreLaunchTelemetry& WebViewPreLaunchControllerWin::GetTelemetry() const {
    return telemetry_;
}

/*static*/
void WebViewPreLaunchControllerWin::CacheWebViewCreationArguments(std::ostream& stream, const WebViewCreationArguments& args) {
    json j(args);
    stream << j;
}

/*static*/
WebViewCreationArguments WebViewPreLaunchControllerWin::ReadCachedWebViewCreationArguments(std::istream& stream) {
    json j;
    stream >> j;
    return j.get<WebViewCreationArguments>();
}

uint32_t WebViewPreLaunchControllerWin::GetBrowserProcessId() const {
    return browser_process_id_;
}
