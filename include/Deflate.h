#ifndef _DEFLATE_H_
#define _DEFLATE_H_

#include <string>

struct z_stream_s;

class Deflate {
 public:
  Deflate(int _compression_level = 6);
  Deflate(const Deflate & other);
  Deflate & operator=(const Deflate & other) = delete;
  ~Deflate();

  unsigned int compress(const void * input, size_t input_len, bool flush = false);
  void flush();
  void reset();

  size_t size() const { return output_buffer.size(); }
  const std::basic_string<unsigned char> & data() const { return output_buffer; }
  
 private:
  bool init();
  
  int compression_level;
  struct z_stream_s * def_stream = 0;
  unsigned int current_uncompressed_pos = 0;
  std::basic_string<unsigned char> output_buffer;
};

#endif
