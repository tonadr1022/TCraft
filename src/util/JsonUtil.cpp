#include "JsonUtil.hpp"

#include <nlohmann/json.hpp>

namespace json_util {

std::optional<std::string> GetString(nlohmann::json& obj, const std::string& str) {
  if (!obj.contains(str)) {
    spdlog::error("Data does not contain key {}", str);
    return std::nullopt;
  }
  return obj[str].get<std::string>();
}

}  // namespace json_util
