#ifndef _TEXTUREATLAS_H_
#define _TEXTUREATLAS_H_

#include "ReadWriteObject.h"

#include <TextureRef.h>
#include <InternalFormat.h>
#include <ImageFormat.h>
#include <LabelStyle.h>

#include <memory>
#include <unordered_map>
#include <vector>
#include <list>

namespace canvas {
  class Context;
  class ContextFactory;
  class Image;
  class Surface;
};

struct update_data_s {
  unsigned int x, y;
  std::shared_ptr<canvas::Image> image;
};

struct texture_pos_s {
  texture_pos_s() : x(0), y(0), width(0), height(0) { }
  texture_pos_s(unsigned int _x, unsigned int _y, unsigned int _width, unsigned int _height) : x(_x), y(_y), width(_width), height(_height) { }

  unsigned int x, y, width, height;
};

class TextureAtlas : public ReadWriteObject {
 public:
  enum AtlasType { GRID = 1, FREE };
  TextureAtlas(AtlasType _type, unsigned int _block_size, unsigned int _texture_width, unsigned int _texture_height, unsigned int _texture_levels = 1, canvas::InternalFormat _internal_format = canvas::RGBA4);

  TextureAtlas(const TextureAtlas & other) = delete;
  TextureAtlas & operator =(const TextureAtlas & other) = delete;
  
  int addImage(std::shared_ptr<canvas::Image> & img, unsigned int actual_width = 0, unsigned int actual_height = 0);
  int renderLabelToTexture(canvas::ContextFactory & factory, const std::string & text, const std::string & tag, canvas::Image * symbol, LabelStyle style);
  int addProfileImage(std::shared_ptr<canvas::Image> & img);
  size_t size() const { return num_textures; }
  bool empty() const { return !num_textures; }
  const canvas::TextureRef & getBackingTexture() const { return texture; }
  void clear() {
    num_textures = 0;
    waiting_updates.clear();
    row_positions.clear();
    current_row_y = current_row_height = 0;
    texture_positions.clear();
    // texture.clear();
  }

  void bindAtlas();
  void unbindAtlas();
  void updateTexture();

  const texture_pos_s & getTexturePos(unsigned int i) const {
    if (i >= 1 && i <= texture_positions.size()) return texture_positions[i - 1];
    else return null_pos;
  }
  
  unsigned int getWidth() const { return texture_width; }
  unsigned int getHeight() const { return texture_height; }
  unsigned int getLevels() const { return texture_levels; }
  unsigned int getBlockSize() const { return block_size; }

  void setMinFilter(canvas::FilterMode mode) { min_filter = mode; }
  void setMagFilter(canvas::FilterMode mode) { mag_filter = mode; }
  
 protected:
  canvas::InternalFormat getCanvasImageFormat() const;

 private:
  AtlasType type;
  size_t num_textures = 0;
  std::list<update_data_s> waiting_updates;
  canvas::TextureRef texture;
  std::vector<int> row_positions;
  std::vector<texture_pos_s> texture_positions;
  unsigned int current_row_y = 0, current_row_height = 0;
  texture_pos_s null_pos;
  unsigned int block_size, texture_width, texture_height, texture_levels;
  canvas::InternalFormat internal_format;
  canvas::FilterMode min_filter = canvas::LINEAR_MIPMAP_LINEAR, mag_filter = canvas::LINEAR;
};

class TextureAtlasRefR {
public:
  TextureAtlasRefR(const TextureAtlas * _atlas) : atlas(_atlas) {
    if (atlas) atlas->lockReader();
  }
  TextureAtlasRefR(const TextureAtlasRefR & other) : atlas(other.atlas) {
    if (atlas) atlas->lockReader();
  }
  ~TextureAtlasRefR() {
    if (atlas) atlas->unlockReader();
  }
  TextureAtlasRefR & operator=(const TextureAtlasRefR & other) {
    if (&other != this) {
      if (atlas) atlas->unlockReader();
      atlas = other.atlas;
      if (atlas) atlas->lockReader();
    }
    return *this;
  }
  const TextureAtlas * operator->() const {
    return atlas;
  }
  const TextureAtlas & operator*() const {
    return *atlas;
  }
  const TextureAtlas * get() const { return atlas; }

  void reset(TextureAtlas * ptr) {
    if (atlas) atlas->unlockReader();
    atlas = ptr;
    if (atlas) atlas->lockReader();
  }
  
private:
  const TextureAtlas * atlas;
};

class TextureAtlasRefW {
public:
  TextureAtlasRefW(TextureAtlas * _atlas) : atlas(_atlas) {
    if (atlas) atlas->lockWriter();
  }
  TextureAtlasRefW(const TextureAtlasRefW & other) = delete;
  TextureAtlasRefW(TextureAtlasRefW && other) {
    atlas = other.atlas;
    other.atlas = nullptr;
  }
  ~TextureAtlasRefW() {
    if (atlas) atlas->unlockWriter();
  }
  TextureAtlasRefW & operator=(const TextureAtlasRefW & other) = delete;
  
  TextureAtlas * operator->() {
    return atlas;
  }
  TextureAtlas & operator*() {
    return *atlas;
  }
  const TextureAtlas * get() const { return atlas; }
  TextureAtlas * get() { return atlas; }

  void reset(TextureAtlas * ptr) {
    if (atlas) atlas->unlockWriter();
    atlas = ptr;
    if (atlas) atlas->lockWriter();
  }
  
private:
  TextureAtlas * atlas;
};

#endif
