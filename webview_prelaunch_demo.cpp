#include <chrono>
#include <fstream>
#include <iostream>
#include <shellscalingapi.h>
#include <thread>
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>
#include <wil/com.h>
#include <windows.h>
#include <wrl.h>
#include <wrl/event.h>
#include "webview_prelaunch_controller.h"

using namespace Microsoft::WRL;

// Minimal Window Procedure function to facilitate WebView2 creation
LRESULT CALLBACK DemoWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int main() {
    // Set the process DPI awareness
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    constexpr char cache_file_path[63] = "C:\\Users\\pcupp\\AppData\\Local\\Temp\\webview_prelaunch_cache.json";
    auto controller = WebViewPreLaunchController::Launch(cache_file_path);

    HRESULT hr = RoInitialize(RO_INIT_SINGLETHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM: " << std::hex << hr << std::endl;
        return -1;
    }
    
    WebViewCreationArguments args = {
        L"",
        L"C:\\Users\\pcupp\\AppData\\Local\\Packages\\MSTeams_8wekyb3d8bbwe\\LocalCache\\Microsoft\\MSTeams",
        L"--autoplay-policy=no-user-gesture-required --disable-background-timer-throttling --edge-webview-foreground-boost-opt-in --edge-webview-interactive-dragging --edge-webview-run-with-package-id --isolate-origins=https://[*.]microsoft.com,https://[*.]sharepoint.com,https://[*.]sharepointonline.com,https://mesh-hearts-teams.azurewebsites.net,https://[*.]meshxp.net,https://res-sdf.cdn.office.net,https://res.cdn.office.net,https://copilot.teams.cloud.microsoft,https://local.copilot.teams.office.com --js-flags=--scavenger_max_new_space_capacity_mb=8 --enable-features=AutofillReplaceCachedWebElementsByRendererIds,DocumentPolicyIncludeJSCallStacksInCrashReports,PartitionedCookies,PreferredAudioOutputDevices,SharedArrayBuffer,ThirdPartyStoragePartitioning,msAbydos,msAbydosGestureSupport,msAbydosHandwritingAttr,msWebView2EnableDraggableRegions,msWebView2SetUserAgentOverrideOnIframes,msWebView2TerminateServiceWorkerWhenIdleIgnoringCdpSessions,msWebView2TextureStream --disable-features=BreakoutBoxPreferCaptureTimestampInVideoFrames,V8Maglev,msWebOOUI",
        L"en-US",
        /*releaseChannelMask*/0xF,
        /*channelSearchKind*/0,
        /*enableTrackingPrevention*/false
    };

    auto cached_args = controller->ReadCachedWebViewCreationArguments(cache_file_path);
    if (!cached_args.has_value() || cached_args.value() != args) {
        std::cout << "Cached WebView creation arguments didn't match." << std::endl;
        
        controller->CacheWebViewCreationArguments(cache_file_path, args);
        std::cout << "New WebView creation arguments cached." << std::endl;
    }

    controller->WaitForLaunch(std::chrono::seconds(30));
    std::cout << "Done waiting for WebView prelaunch." << std::endl;

    ////////////////////////////////////
    // Demo Window Creation
    const char CLASS_NAME[] = "WebViewDemo";
    WNDCLASS wc = { };

    wc.lpfnWndProc = DemoWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window that will display the regular webview
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "WebViewDemo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 800,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (hwnd == NULL) {
        std::cerr << "Failed to create window" << std::endl;
        return -1;
    }

    ShowWindow(hwnd, SW_SHOW);

    ////////////////////////////////////
    // Initialize WebView2
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    options->put_AllowSingleSignOnUsingOSPrimaryAccount(true);

    Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions7> options7;
    options.As(&options7);
    if (options7) {
        options7->put_ChannelSearchKind(static_cast<COREWEBVIEW2_CHANNEL_SEARCH_KIND>(args.channelSearchKind));
        options7->put_ReleaseChannels(static_cast<COREWEBVIEW2_RELEASE_CHANNELS>(args.releaseChannelsMask));
    }
    
    options->put_AdditionalBrowserArguments(args.additional_browser_arguments.c_str());
    options->put_EnableTrackingPrevention(args.enableTrackingPrevention);
    options->put_Language(args.language.c_str());

    wil::com_ptr<ICoreWebView2Environment> webViewEnvironment;
    wil::com_ptr<ICoreWebView2Controller> webViewController;
    wil::com_ptr<ICoreWebView2> webView;
    hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        args.user_data_dir.c_str(),
        options.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hwnd, &webViewEnvironment, &webViewController, &webView](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result)) {
                    std::cerr << "Failed to create WebView2 environment: " << std::hex << result << std::endl;
                    return result;
                }
                webViewEnvironment = env;

                env->CreateCoreWebView2Controller(hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [hwnd, &webViewController, &webView](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                        if (FAILED(result)) {
                            std::cerr << "Failed to create WebView2 controller: " << std::hex << result << std::endl;
                            return result;
                        }

                        if (controller != nullptr) {
                            webViewController = controller;
                            webViewController->get_CoreWebView2(&webView);

                            webView->add_NavigationStarting(
                                Callback<ICoreWebView2NavigationStartingEventHandler>(
                                    [hwnd](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                                        LPWSTR uri;
                                        args->get_Uri(&uri);
                                        std::wcout << L"Navigation starting: " << uri << std::endl;
                                        CoTaskMemFree(uri);

                                        return S_OK;
                                    }).Get(), nullptr);

                            webView->Navigate(L"https://www.bing.com");
                            webView->add_NavigationCompleted(
                                Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                    [hwnd](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                                        BOOL isSuccess;
                                        args->get_IsSuccess(&isSuccess);
                                        std::cout << "Navigation completed: " << (isSuccess ? "Success" : "Failed") << std::endl;
                                        return S_OK;
                                    }).Get(), nullptr);

                            RECT bounds;
                            GetClientRect(hwnd, &bounds);
                            std::cout << "Window bounds: " << bounds.left << ", " << bounds.top << ", " << bounds.right << ", " << bounds.bottom << std::endl;
                            webViewController->put_Bounds(bounds);

                            BOOL visibility = FALSE;
                            webViewController->get_IsVisible(&visibility);
                            std::cout << "WV2 visibility: " << visibility << std::endl;
                        }
                        return S_OK;
                    }).Get());
                return S_OK;
            }).Get());

    if (FAILED(hr)) {
        std::cerr << "Failed to create WebView2 environment: " << std::hex << hr << std::endl;
    }
    
    // Run the message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // We could close right after we have created the demo WebView
    // TODO: determine if we avoid the creation of about blank renderers if we leave our pre-launch alive
    controller->Close();
    controller->WaitForClose();

    return 0;
}
