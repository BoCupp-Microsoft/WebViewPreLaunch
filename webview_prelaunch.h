#include <chrono>
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

#include "webview_creation_arguments.h"

class WebViewPreLaunch {
private:
    wil::com_ptr<ICoreWebView2> webView_;
    wil::com_ptr<ICoreWebView2Controller> webViewController_;
    std::binary_semaphore semaphore_;
    std::thread launch_thread_;
    HWND backgroundHwnd_ = nullptr;
    bool background_thread_should_exit_ = false;

    bool LaunchBackground(const std::string& cache_args_path);
    HWND CreateMessageWindow();
    HRESULT EnvironmentCreatedCallback( HRESULT result, ICoreWebView2Environment* env);
    HRESULT ControllerCreatedCallback(HRESULT result, ICoreWebView2Controller* controller);
    
public:
    WebViewPreLaunch();

    void Launch(const std::string& cache_args_path);
    bool WaitForLaunch(std::chrono::seconds timeout);

    // public for testing only
    static std::optional<WebViewCreationArguments> ReadCachedWebViewCreationArguments(std::istream& stream);
    static void CacheWebViewCreationArguments(std::ostream& stream, const WebViewCreationArguments& args);

};
