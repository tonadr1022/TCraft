#pragma once

#include <nlohmann/json_fwd.hpp>

namespace util::json {
extern std::optional<std::string> GetString(nlohmann::json& obj, const std::string& str);
extern std::optional<nlohmann::json> GetObject(nlohmann::json& obj, const std::string& str);
extern void WriteJson(nlohmann::json& obj, const std::string& path);
}  // namespace util::json
