#include "webview_creation_arguments.hpp"

void to_json(nlohmann::json& j, const WebViewCreationArguments& args) {
    j = nlohmann::json{
        {"browser_exe_path", args.browser_exe_path},
        {"user_data_dir", args.user_data_dir},
        {"additional_browser_arguments", args.additional_browser_arguments},
        {"language", args.language},
        {"release_channels_mask", args.release_channels_mask},
        {"channel_search_kind", args.channel_search_kind},
        {"enable_tracking_prevention", args.enable_tracking_prevention}
    };
}

void from_json(const nlohmann::json& j, WebViewCreationArguments& args) {
    j.at("browser_exe_path").get_to(args.browser_exe_path);
    j.at("user_data_dir").get_to(args.user_data_dir);
    j.at("additional_browser_arguments").get_to(args.additional_browser_arguments);
    j.at("language").get_to(args.language);
    j.at("release_channels_mask").get_to(args.release_channels_mask);
    j.at("channel_search_kind").get_to(args.channel_search_kind);
    j.at("enable_tracking_prevention").get_to(args.enable_tracking_prevention);
}