#include "JsonUtil.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace util::json {

std::optional<std::string> GetString(nlohmann::json& obj, const std::string& str) {
  if (!obj.contains(str)) {
    spdlog::error("Data does not contain key {}", str);
    return std::nullopt;
  }
  return obj[str].get<std::string>();
}

std::optional<nlohmann::json> GetObject(nlohmann::json& obj, const std::string& str) {
  if (!obj.contains(str)) {
    spdlog::error("Data does not contain key {}", str);
    return std::nullopt;
  }
  return obj[str];
}

void WriteJson(nlohmann::json& obj, const std::string& path) {
  std::ofstream f(path);
  f << std::setw(2) << obj << std::endl;
}
}  // namespace util::json
