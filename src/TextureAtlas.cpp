#include "GL.h"
#include "TextureAtlas.h"

#include <Context.h>
#include <OpenGLTexture.h>
#include <Image.h>

#include <cmath>
#include <iostream>
#include <cassert>

using namespace std;

TextureAtlas::TextureAtlas(AtlasType _type, unsigned int _block_size, unsigned int _texture_width, unsigned int _texture_height, unsigned int _texture_levels, canvas::InternalFormat _internal_format)
  : type(_type), block_size(_block_size), texture_width(_texture_width), texture_height(_texture_height), texture_levels(_texture_levels), internal_format(_internal_format) {
}

void
TextureAtlas::initializeTexture() {
  if (!texture.get()) { 
    cerr << "creating texture for atlas, w = " << texture_width << ", h = " << texture_height << ", l = " << texture_levels << endl;
    canvas::FilterMode min_filter2 = min_filter;
    if (min_filter2 == canvas::LINEAR_MIPMAP_LINEAR && texture_levels <= 1) min_filter2 = canvas::LINEAR;
    texture = canvas::OpenGLTexture::createTexture(texture_width, texture_height, texture_width, texture_height, min_filter2, mag_filter, internal_format, texture_levels);
  }
}

void
TextureAtlas::updateTexture() {
  initializeTexture();
  if (!waiting_updates.empty()) {
    assert(texture.get());
    // cerr << "updating textureAtlas " << texture.getTextureId() << ": " << waiting_updates.size() << endl;
    
    // auto & data = waiting_updates.front();
    for (auto & data : waiting_updates) {
      assert(data.image.get());
      // cerr << "updating texture " << texture.getTextureId() << ": " << data.x << ", " << data.y << ", " << data.image->getHeight() << ", " << data.image->getHeight() << endl;
      texture.updateData(*(data.image), data.x, data.y);
    }
    waiting_updates.clear();
    texture.generateMipmaps();
  }
}

canvas::InternalFormat
TextureAtlas::getCanvasImageFormat() const {
  switch (internal_format) {
  case canvas::R8:
  case canvas::RG8:
  case canvas::RGB_ETC1:
  case canvas::RGB_DXT1:
    return canvas::RGB8;
        
  case canvas::RGB565:
    return canvas::RGB565;
    // return canvas::ImageFormat::RGB32;

  case canvas::RGBA4:
  case canvas::RGBA8:
  case canvas::LUMINANCE_ALPHA:
  default:
    return canvas::RGBA8;
  }
}

int
TextureAtlas::addProfileImage(std::shared_ptr<canvas::Image> & img) {
  assert(img.get());
  
  const unsigned int w = getBlockSize(), h = getBlockSize();
  if (img->getWidth() != w || img->getHeight() != h || img->getLevels() != getLevels()) {
    cerr << "wrong dimensions, image: (w = " << img->getWidth() << ", h = " << img->getHeight() << ", l = " << img->getLevels() << "), atlas: (" << getBlockSize() << ", " << getLevels() << ")\n";
    assert(0);
  }
  if (num_textures >= texture_width / w * texture_height / h) {
    cerr << "atlas full (w = " << texture_width << ", h = " << texture_height << ")" << endl;
    // assert(0);
    return 0; // return default texture
  }

  int index = int(num_textures++);
  int textures_per_row = texture_width / w;
  unsigned int xx0 = (index % textures_per_row) * w;
  unsigned int yy0 = index / textures_per_row * h;

  waiting_updates.push_back({xx0, yy0, img});
  
  return index;
}

int
TextureAtlas::addImage(std::shared_ptr<canvas::Image> & img, unsigned int actual_width, unsigned int actual_height) {
  assert(img);
  if (!actual_width) actual_width = img->getWidth();
  if (!actual_height) actual_height = img->getHeight();
  
  int index = int(num_textures++);
  assert(type == FREE);
  if (type == FREE) index++;

  unsigned int aligned_width = img->getWidth(), aligned_height = img->getHeight();
  
  if (row_positions.empty()) {
    row_positions.push_back(0);
    current_row_height = aligned_height;
  } else if (row_positions.back() + aligned_width > texture_width || current_row_height != aligned_height) {
    row_positions.push_back(0);
    current_row_y += current_row_height;
    current_row_height = aligned_height;
  }
  unsigned int xx0 = row_positions.back();
  unsigned int yy0 = current_row_y;
  row_positions.back() += aligned_width;
  assert(!row_positions.empty());

  if (yy0 + aligned_height > getHeight()) {
    cerr << "atlas full\n";
    row_positions.pop_back();
    return -1;
  }

  if (internal_format == canvas::RG_RGTC2) {
    img = img->convert(internal_format);
  } else if (internal_format == canvas::LA44 && 0) {
    img = img->convert(internal_format);
  }
    
  waiting_updates.push_back({ xx0, yy0, img });
  texture_positions.push_back(texture_pos_s(xx0, yy0, actual_width, actual_height));
  
  return index;
}

int
TextureAtlas::renderLabelToTexture(canvas::ContextFactory & factory, const std::string & text, const std::string & tag, canvas::Image * symbol, LabelStyle style) {
  auto context = factory.createContext(0, 0, getCanvasImageFormat(), true);

  float tag_width = 0;
  if (!tag.empty()) {
    context->font.size = 9;
    tag_width = context->measureText(tag).width;    
  }

  float font_size = style == LABEL_GROUP_TITLE ? 15 : 11;
  float h_margin = style == LABEL_DARK_BOX ? 18 : 1;
  float v_margin = style == LABEL_DARK_BOX ? 9 : 0;
  context->font.size = font_size;
  context->font.weight = style == LABEL_GROUP_TITLE ? canvas::Font::BOLD : canvas::Font::NORMAL;  
  context->textBaseline = "middle";
  canvas::TextMetrics size = context->measureText(text);  
  
  bool has_symbol = symbol != 0;
  double x = 0, y = 0, w = round(size.width + tag_width) + h_margin + (has_symbol ? 19 : 0), h = font_size + v_margin;

  unsigned int aligned_width = (int(w * context->getDisplayScale()) + 3) & ~3;
  unsigned int aligned_height = (int(h * context->getDisplayScale()) + 3) & ~3;

  context->resize(int(aligned_width / context->getDisplayScale()), int(aligned_height / context->getDisplayScale()));
  
  if (style == LABEL_DARK_BOX) {
    float r = 5;
    context->fillStyle = canvas::Color(0.1f, 0.075f, 0.05f, 0.75f);
    context->beginPath();
    context->moveTo(x+r, y);
    context->arcTo(x+w, y,   x+w, y+h, r);
    context->arcTo(x+w, y+h, x,   y+h, r);
    context->arcTo(x,   y+h, x,   y,   r);
    context->arcTo(x,   y,   x+w, y,   r);
    context->fill();
    context->fillStyle = "#fff";
#ifndef __APPLE__
    context->shadowColor = canvas::Color(0.0f, 0.0f, 0.0f, 0.25f);
    context->shadowBlur = 2.0f;
    context->shadowOffsetX = context->shadowOffsetY = 1.0f;
#endif
  } else if (style == LABEL_GROUP_TITLE) {
    context->shadowColor = canvas::Color(1.0f, 1.0f, 1.0f, 1.0f);
    context->shadowBlur = 2.0f;    
    context->fillStyle = "#000";
  } else {
    context->shadowColor = canvas::Color(1.0f, 1.0f, 1.0f, 1.0f);
    context->shadowBlur = 2.0f;
    context->fillStyle = "#000";
  }
  
  int pos = h_margin / 3;
  if (has_symbol) {
    // context->globalAlpha = 0.75f;
    context->drawImage(*symbol, pos, h / 2 - 8, 16, 16);
    pos += 19;
    // context->globalAlpha = 1.0f;
  }
  context->fillText(text, pos, h / 2.0);
  pos += size.width + 5;

  if (!tag.empty()) {
    x += pos;
    float w2 = tag_width + 8;
    float r = 3;
    y += 3;
    float h2 = h - 6;
    
    context->fillStyle = canvas::Color(0.0f, 0.0f, 0.0f, 0.5f);
    context->beginPath();
    context->moveTo(x+r, y);
    context->arcTo(x+w2, y,   x+w2, y+h2, r);
    context->arcTo(x+w2, y+h2, x,   y+h2, r);
    context->arcTo(x,   y+h2, x,   y,   r);
    context->arcTo(x,   y,   x+w2, y,   r);
    context->fill();
    
    context->fillStyle = "#bdf";
    context->fillText(tag, pos + 2, h / 2.0);
  }
  
  // cerr << "created label: " << w << " " << h << " => " << context->getActualWidth() << " " << context->getActualHeight() << endl;

  std::shared_ptr<canvas::Image> img = context->getDefaultSurface().createImage();
  return addImage(img, (unsigned int)(w * context->getDisplayScale()), (unsigned int)(h * context->getDisplayScale()));
}
