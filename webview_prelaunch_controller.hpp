#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "webview_creation_arguments.hpp"

struct WebViewPreLaunchTelemetry {
  std::vector<std::string> exceptions;

  std::chrono::time_point<std::chrono::high_resolution_clock> launch_start;
  // All the subsequent milliseconds record the distance from launch_start until their recording.
  // They are not step times to be summed.  Zero values were not recorded.
  std::chrono::milliseconds background_launch_started = std::chrono::milliseconds::zero();
  std::chrono::milliseconds read_cached_args_completed = std::chrono::milliseconds::zero();
  std::chrono::milliseconds window_created = std::chrono::milliseconds::zero();
  std::chrono::milliseconds environment_created = std::chrono::milliseconds::zero();
  std::chrono::milliseconds controller_created = std::chrono::milliseconds::zero();
  // The timings from here forward are recorded on the foreground thread to help track any negative
  // impact on its execution from pre-launching.
  std::chrono::milliseconds waitforlaunch_started = std::chrono::milliseconds::zero();
  std::chrono::milliseconds waitforlaunch_completed = std::chrono::milliseconds::zero();
  std::chrono::milliseconds cache_arguments_completed = std::chrono::milliseconds::zero();
  std::chrono::milliseconds foreground_read_cached_args_completed =
      std::chrono::milliseconds::zero();
  std::chrono::milliseconds close_started = std::chrono::milliseconds::zero();
  std::chrono::milliseconds waitforclose_completed = std::chrono::milliseconds::zero();

  std::chrono::milliseconds DurationSinceLaunch() const;
};

class WebViewPreLaunchController {
public:
      // Starts webview launch on a background thread.
  static std::shared_ptr<WebViewPreLaunchController> Launch(
    const std::filesystem::path& cache_args_path);

  virtual std::optional<WebViewCreationArguments> ReadCachedWebViewCreationArguments(
    const std::filesystem::path& cache_args_path) noexcept = 0;
  virtual void CacheWebViewCreationArguments(const std::filesystem::path& cache_args_path,
                                             const WebViewCreationArguments& args) noexcept = 0;

  // Closes the pre-launched webview and exits the background thread.
  virtual void Close(bool close_webview_on_exit) = 0;
  // Blocks while closing activities are completed.
  virtual void WaitForClose() = 0;

  // Blocks until launch is completed.
  virtual void WaitForLaunch() = 0;

  // Get telemetry data about the pre-launch process.
  // This should be called after WaitForLaunch() to get accurate data.
  virtual const WebViewPreLaunchTelemetry& GetTelemetry() const = 0;
};