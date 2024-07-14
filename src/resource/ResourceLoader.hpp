#pragma once

using TextureHandle = uint32_t;
class ResourceLoader {
 public:
  TextureHandle LoadTexture2DArray(const std::vector<std::vector<uint8_t>> &pixels, int width,
                                   int height);

 private:
};
