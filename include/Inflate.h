#ifndef _GRAPHLIB_INFLATE_H_
#define _GRAPHLIB_INFLATE_H_

#include <string>

struct z_stream_s;

class Inflate {
 public:
  Inflate(const std::basic_string<unsigned char> * _input_buffer);
  Inflate(const Inflate & other) = delete;
  Inflate & operator=(const Inflate & other) = delete;
  ~Inflate();

  std::string decompressString(unsigned int data_offset, unsigned short data_length);  
  
 private:
  bool init();

  struct z_stream_s * inf_stream = 0;
  const std::basic_string<unsigned char> * input_buffer;
};

#endif
