#include "webview_prelaunch_controller.hpp"
#include "webview_prelaunch_controller_win.hpp"

std::chrono::milliseconds WebViewPreLaunchTelemetry::DurationSinceLaunch() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - launch_start);
}