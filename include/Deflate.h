#ifndef _DEFLATE_H_
#define _DEFLATE_H_

extern "C" {
#include <zlib.h>
};

class Deflate {
 public:
  Deflate(int _compression_level = Z_DEFAULT_COMPRESSION);
  Deflate(const Deflate & other);
  Deflate & operator=(const Deflate & other);
  ~Deflate();

  bool compress(const ustring & input, ustring & output);
  bool decompress(const ustring & input, ustring & output);
  bool decompressStream(const ustring & input, ustring & output);
  void reset();

 private:
  bool initInflate(bool reset = true);
  bool initDeflate();

  int compression_level;
  bool deflate_init = 0, inflate_init = 0;
  z_stream def_stream, inf_stream;
  unsigned char * in_buffer, * out_buffer;
};

#endif
