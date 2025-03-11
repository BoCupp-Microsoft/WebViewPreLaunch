#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>
#include <wil/com.h>
#include <windows.h>
#include <wrl.h>
#include <wrl/event.h>

#include "webview_launch.h"

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

static wil::com_ptr<ICoreWebView2> s_webView;
static wil::com_ptr<ICoreWebView2Controller> s_webViewController;

// Minimal Window Procedure function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_SIZE:
            if (s_webViewController != nullptr) {
                RECT bounds;
                GetClientRect(hwnd, &bounds);
                s_webViewController->put_Bounds(bounds);
            };
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}


void CreateWebView(HWND hwnd) {
    HRESULT hr = RoInitialize(RO_INIT_SINGLETHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM: " << std::hex << hr << std::endl;
        return;
    }

    // Initialize WebView2
    wil::com_ptr<ICoreWebView2EnvironmentOptions> options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    options->put_AdditionalBrowserArguments(L"--enable-features=msSingleSignOnOSForPrimaryAccountIsShared,AutofillReplaceCachedWebElementsByRendererIds,DocumentPolicyIncludeJSCallStacksInCrashReports,PartitionedCookies,PreferredAudioOutputDevices,SharedArrayBuffer,ThirdPartyStoragePartitioning,msAbydos,msAbydosGestureSupport,msAbydosHandwritingAttr,msWebView2EnableDraggableRegions,msWebView2SetUserAgentOverrideOnIframes,msWebView2TerminateServiceWorkerWhenIdleIgnoringCdpSessions,msWebView2TextureStream --isolate-origins=https://[*.]microsoft.com,https://[*.]sharepoint.com,https://[*.]sharepointonline.com,https://mesh-hearts-teams.azurewebsites.net,https://[*.]meshxp.net,https://res-sdf.cdn.office.net,https://res.cdn.office.net,https://copilot.teams.cloud.microsoft,https://local.copilot.teams.office.com --js-flags=--scavenger_max_new_space_capacity_mb=8");
    
    wil::com_ptr<ICoreWebView2Environment> webViewEnvironment;
    hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, 
        //L"C:\\Users\\pcupp\\AppData\\Local\\Packages\\MSTeams_8wekyb3d8bbwe\\LocalCache\\Microsoft\\MSTeams", 
        L"C:\\Users\\pcupp\\AppData\\Local\\Temp\\webview_launch_user_data_dir",
        options.get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hwnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result)) {
                    std::cerr << "Failed to create WebView2 environment: " << std::hex << result << std::endl;
                    return result;
                }

                env->CreateCoreWebView2Controller(hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [hwnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                        if (FAILED(result)) {
                            std::cerr << "Failed to create WebView2 controller: " << std::hex << result << std::endl;
                            return result;
                        }

                        if (controller != nullptr) {
                            s_webViewController = controller;
                            s_webViewController->get_CoreWebView2(&s_webView);
                            std::cout << "Got WV: " << result << std::endl;

                            s_webView->add_NavigationStarting(
                                Callback<ICoreWebView2NavigationStartingEventHandler>(
                                    [hwnd](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                                        LPWSTR uri;
                                        args->get_Uri(&uri);
                                        std::wcout << L"Navigation starting: " << uri << std::endl;
                                        CoTaskMemFree(uri);

                                        // // Cancel navigation if the URL contains "example.com"
                                        // std::wstring url(uri);
                                        // if (url.find(L"example.com") != std::wstring::npos) {
                                        //     args->put_Cancel(true);
                                        //     std::wcout << L"Navigation canceled for URL: " << uri << std::endl;
                                        // }
                                        return S_OK;
                                    }).Get(), nullptr);

                            s_webView->Navigate(L"https://www.bing.com");
                            s_webView->add_NavigationCompleted(
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
                            s_webViewController->put_Bounds(bounds);

                            BOOL visibility = FALSE;
                            s_webViewController->get_IsVisible(&visibility);
                            std::cout << "WV2 visibility: " << visibility << std::endl;

                            // COREWEBVIEW2_COLOR color = {1.0, 0, 0, 1.0};
                            // wil::com_ptr<ICoreWebView2Controller2> controller2 =
                            // s_webViewController.query<ICoreWebView2Controller2>();
                            // controller2->put_DefaultBackgroundColor(color);
                        }
                        return S_OK;
                    }).Get());
                return S_OK;
            }).Get());

    if (FAILED(hr)) {
        std::cerr << "Failed to create WebView2 environment: " << std::hex << hr << std::endl;
    }
}

void webViewLaunch(void* context) {
    RoInitialize(RO_INIT_SINGLETHREADED);
    // Define the window class
    const char CLASS_NAME[] = "SplashScreenClass";
    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Splash Screen",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 800,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (hwnd == NULL) {
        std::cerr << "Failed to create window" << std::endl;
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
    CreateWebView(hwnd);

    // Run the message loop
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);    
    }

}
