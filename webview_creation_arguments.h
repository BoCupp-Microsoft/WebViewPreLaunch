#include <string>
#include <nlohmann/json.hpp>

struct WebViewCreationArguments {
    std::wstring browser_exe_path;
    std::wstring user_data_dir;
    std::wstring additional_browser_arguments;
};

void to_json(nlohmann::json& j, const WebViewCreationArguments& args);
void from_json(const nlohmann::json& j, WebViewCreationArguments& args);
