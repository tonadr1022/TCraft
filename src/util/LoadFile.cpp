#include "LoadFile.hpp"

#include <fstream>

namespace util {

std::optional<std::string> LoadFromFile(const std::string& path) {
  std::ifstream file_stream(path);
  std::string line;
  std::stringstream s_stream;
  if (!file_stream.is_open()) return std::nullopt;
  while (std::getline(file_stream, line)) {
    s_stream << line << '\n';
  }
  return s_stream.str();
}

}  // namespace util
