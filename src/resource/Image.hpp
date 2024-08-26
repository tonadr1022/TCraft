#pragma once

struct Image {
  uint8_t* pixels{};
  int width{}, height{}, channels{};
};
