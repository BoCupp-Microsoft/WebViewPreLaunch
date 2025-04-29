#include "webview_prelaunch_controller.hpp"
#include "webview_prelaunch_controller_win.hpp"

/* static */
std::shared_ptr<WebViewPreLaunchController> WebViewPreLaunchController::Launch(const std::string& cache_args_path) {
    auto webview_prelaunch = std::make_shared<WebViewPreLaunchControllerWin>();
    webview_prelaunch->Launch(cache_args_path);
    return webview_prelaunch;
}