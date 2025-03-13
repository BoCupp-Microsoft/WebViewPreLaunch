#include <chrono>
#include <fstream>
#include <iostream>
#include <shellscalingapi.h>
#include <thread>
#include "webview_prelaunch.h"

int main() {
    std::cout << "Hello, World!" << std::endl;

    // Set the process DPI awareness
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    WebViewPreLaunch webview_prelaunch;
    constexpr char cache_file_path[63] = "C:\\Users\\pcupp\\AppData\\Local\\Temp\\webview_prelaunch_cache.json";
    std::ofstream cache_file(cache_file_path);
    if (!cache_file) {
        std::cerr << "Could not open cache file for writing" << std::endl;
        return -1;
    }

    WebViewCreationArguments args = {
        L"",
        L"C:\\Users\\pcupp\\AppData\\Local\\Packages\\MSTeams_8wekyb3d8bbwe\\LocalCache\\Microsoft\\MSTeams",
        //L"C:\\Users\\pcupp\\AppData\\Local\\Temp\\webview_prelaunch",
        L"--autoplay-policy=no-user-gesture-required --disable-background-timer-throttling --edge-webview-foreground-boost-opt-in --edge-webview-interactive-dragging --edge-webview-run-with-package-id --isolate-origins=https://[*.]microsoft.com,https://[*.]sharepoint.com,https://[*.]sharepointonline.com,https://mesh-hearts-teams.azurewebsites.net,https://[*.]meshxp.net,https://res-sdf.cdn.office.net,https://res.cdn.office.net,https://copilot.teams.cloud.microsoft,https://local.copilot.teams.office.com --js-flags=--scavenger_max_new_space_capacity_mb=8 --enable-features=AutofillReplaceCachedWebElementsByRendererIds,DocumentPolicyIncludeJSCallStacksInCrashReports,PartitionedCookies,PreferredAudioOutputDevices,SharedArrayBuffer,ThirdPartyStoragePartitioning,msAbydos,msAbydosGestureSupport,msAbydosHandwritingAttr,msWebView2EnableDraggableRegions,msWebView2SetUserAgentOverrideOnIframes,msWebView2TerminateServiceWorkerWhenIdleIgnoringCdpSessions,msWebView2TextureStream --disable-features=BreakoutBoxPreferCaptureTimestampInVideoFrames,V8Maglev,msWebOOUI"
    };
    webview_prelaunch.CacheWebViewCreationArguments(cache_file, args);
    cache_file.close();
    std::cout << "WebView creation arguments cached." << std::endl;

    webview_prelaunch.Launch(cache_file_path);
    std::cout << "WebView prelaunch initiated." << std::endl;
;
    webview_prelaunch.WaitForLaunch(std::chrono::seconds(30));
    std::cout << "WebView prelaunch completed." << std::endl;
    
    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
