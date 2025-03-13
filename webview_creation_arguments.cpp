#include <windows.h>
#include "webview_creation_arguments.h"

std::wstring string_to_wide_string(const std::string& string)
{
    if (string.empty())
    {
        return L"";
    }

    const auto size_needed = MultiByteToWideChar(CP_UTF8, 0, string.data(), (int)string.size(), nullptr, 0);
    if (size_needed <= 0)
    {
        throw std::runtime_error("MultiByteToWideChar() failed: " + std::to_string(size_needed));
    }

    std::wstring result(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, string.data(), (int)string.size(), result.data(), size_needed);
    return result;
}

std::string wide_string_to_string(const std::wstring& wide_string)
{
    if (wide_string.empty())
    {
        return "";
    }

    const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, wide_string.data(), (int)wide_string.size(), nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0)
    {
        throw std::runtime_error("WideCharToMultiByte() failed: " + std::to_string(size_needed));
    }

    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide_string.data(), (int)wide_string.size(), result.data(), size_needed, nullptr, nullptr);
    return result;
}

void to_json(nlohmann::json& j, const WebViewCreationArguments& args) {
    j = nlohmann::json{
        {"browser_exe_path", wide_string_to_string(args.browser_exe_path)},
        {"user_data_dir", wide_string_to_string(args.user_data_dir)},
        {"additional_browser_arguments", wide_string_to_string(args.additional_browser_arguments)}
    };
}

void from_json(const nlohmann::json& j, WebViewCreationArguments& args) {
    std::string browser_exe_path;
    std::string user_data_dir;
    std::string additional_browser_arguments;

    j.at("browser_exe_path").get_to(browser_exe_path);
    j.at("user_data_dir").get_to(user_data_dir);
    j.at("additional_browser_arguments").get_to(additional_browser_arguments);

    args.browser_exe_path = string_to_wide_string(browser_exe_path);
    args.user_data_dir = string_to_wide_string(user_data_dir);
    args.additional_browser_arguments = string_to_wide_string(additional_browser_arguments);
}