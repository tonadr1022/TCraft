#include "StringUtil.hpp"

namespace util {

std::string GetFilenameStem(const std::string& filename) {
  return filename.substr(0, filename.find_last_of('.'));
}

}  // namespace util
