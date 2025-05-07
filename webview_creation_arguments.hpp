#pragma once

#include <nlohmann/json.hpp>
#include <string>

struct WebViewCreationArguments {
    std::string browser_exe_path;
    std::string user_data_dir;
    std::string additional_browser_arguments;
    std::string language;
    uint8_t release_channels_mask = 0;
    uint8_t channel_search_kind = 0;
    bool enable_tracking_prevention = false;

    bool operator==(const WebViewCreationArguments&) const = default;
};

void to_json(nlohmann::json& j, const WebViewCreationArguments& args);
void from_json(const nlohmann::json& j, WebViewCreationArguments& args);
