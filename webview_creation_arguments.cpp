#include "webview_creation_arguments.hpp"

void to_json(nlohmann::json& j, const WebViewCreationArguments& args) {
    j = nlohmann::json{
        {"browser_exe_path", args.browser_exe_path},
        {"user_data_dir", args.user_data_dir},
        {"additional_browser_arguments", args.additional_browser_arguments},
        {"language", args.language},
        {"releaseChannelsMask", args.releaseChannelsMask},
        {"channelSearchKind", args.channelSearchKind},
        {"enableTrackingPrevention", args.enableTrackingPrevention}
    };
}

void from_json(const nlohmann::json& j, WebViewCreationArguments& args) {
    j.at("browser_exe_path").get_to(args.browser_exe_path);
    j.at("user_data_dir").get_to(args.user_data_dir);
    j.at("additional_browser_arguments").get_to(args.additional_browser_arguments);
    j.at("language").get_to(args.language);
    j.at("releaseChannelsMask").get_to(args.releaseChannelsMask);
    j.at("channelSearchKind").get_to(args.channelSearchKind);
    j.at("enableTrackingPrevention").get_to(args.enableTrackingPrevention);
}