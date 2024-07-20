#pragma once

#include <nlohmann/json_fwd.hpp>

namespace json_util {
extern std::optional<std::string> GetString(nlohmann::json& obj, const std::string& str);
extern std::optional<nlohmann::json> GetObject(nlohmann::json& obj, const std::string& str);
}  // namespace json_util
