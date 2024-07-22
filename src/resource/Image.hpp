#pragma once

struct Image {
  // unsigned char* pixels;
  void* pixels;
  int width, height, channels;
};
