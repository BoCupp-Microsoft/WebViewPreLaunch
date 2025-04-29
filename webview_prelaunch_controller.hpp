#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "webview_creation_arguments.hpp"

struct WebviewPrelaunchTelemetry {
    std::vector<std::string> exceptions;

    std::chrono::time_point<std::chrono::high_resolution_clock> launch_start;
    std::chrono::milliseconds background_launch_start;
    std::chrono::milliseconds cached_args_read;
    std::chrono::milliseconds window_created;
    std::chrono::milliseconds envirionment_created;
    std::chrono::milliseconds controller_created;

    std::chrono::milliseconds DurationSinceLaunch() const;
};

class WebviewPrelaunchController {
public:
    // Starts webview launch on a background thread.
    static std::shared_ptr<WebviewPrelaunchController> Launch(const std::string& cache_args_path);

    virtual std::optional<WebViewCreationArguments> ReadCachedWebViewCreationArguments(const std::string& cache_args_path) noexcept = 0;
    virtual void CacheWebViewCreationArguments(const std::string& cache_args_path, const WebViewCreationArguments& args) noexcept = 0;

    // Closes the pre-launched webview and exits the background thread.
    virtual void Close(bool wait_for_browser_process_exit) = 0;
    // Blocks while closing activities are completed.
    virtual void WaitForClose() = 0;

    // Blocks until launch is completed.
    virtual void WaitForLaunch() = 0;
  
    // Get telemetry data about the pre-launch process.
    // This should be called after WaitForLaunch() to get accurate data.
    virtual const WebviewPrelaunchTelemetry& GetTelemetry() const = 0;
};