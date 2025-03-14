#include <chrono>
#include <fstream>
#include <iostream>
#include <shellscalingapi.h>
#include <thread>
#include "webview_prelaunch_controller.h"

int main() {
    std::cout << "Hello, World!" << std::endl;

    // Set the process DPI awareness
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    constexpr char cache_file_path[63] = "C:\\Users\\pcupp\\AppData\\Local\\Temp\\webview_prelaunch_cache.json";
    auto controller = WebViewPreLaunchController::Launch(cache_file_path);
    
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
    
    // Create the other WV here
    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
