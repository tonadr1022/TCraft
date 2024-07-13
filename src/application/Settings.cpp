#include "Settings.hpp"

Settings* Settings::instance_ = nullptr;
Settings& Settings::Get() { return *instance_; }
Settings::Settings() {
  EASSERT_MSG(instance_ == nullptr, "Cannot create two shader managers.");
  instance_ = this;
}
