#include "webview_prelaunch.h"
#include "webview_prelaunch_controller.h"

/* static */
std::shared_ptr<WebViewPreLaunchController> WebViewPreLaunchController::Launch(const std::string& cache_args_path) {
    auto webview_prelaunch = std::make_shared<WebViewPreLaunch>();
    webview_prelaunch->Launch(cache_args_path);
    return webview_prelaunch;
}