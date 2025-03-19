#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include "webview_creation_arguments.h"

class WebViewPreLaunchController {
public:
    // Starts webview launch on a background thread.
    static std::shared_ptr<WebViewPreLaunchController> Launch(const std::string& cache_args_path);

    virtual std::optional<WebViewCreationArguments> ReadCachedWebViewCreationArguments(const std::string& cache_args_path) = 0;
    virtual void CacheWebViewCreationArguments(const std::string& cache_args_path, const WebViewCreationArguments& args) = 0;

    // Closes the pre-launched webview and exits the background thread.
    virtual void Close() = 0;
    // Blocks while closing activities are completed.
    virtual void WaitForClose() = 0;

    // Blocks until launch is completed (or until it errors out)
    virtual bool WaitForLaunch(std::chrono::seconds timeout) = 0;
};