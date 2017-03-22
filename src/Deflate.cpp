#include "Deflate.h"

extern "C" {
#include <zlib.h>
};

#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdlib>

#define CHUNK 65536

using namespace std;

Deflate::Deflate(int _compression_level) : compression_level(_compression_level)
{
}

Deflate::Deflate(const Deflate & other)
  : compression_level(other.compression_level),
    current_uncompressed_pos(other.current_uncompressed_pos),
    output_buffer(other.output_buffer)
{
  if (other.def_stream) {
    def_stream = (z_stream *)malloc(sizeof(z_stream));
    z_stream * other_stream = const_cast<z_stream*>(other.def_stream);
    deflateCopy(def_stream, other_stream);    
  }
}

Deflate::~Deflate() {   
  if (def_stream) {
    deflateEnd(def_stream);
    free(def_stream);
  }
}

bool
Deflate::init() {
  if (!def_stream) {
    cerr << "init stream\n";
    def_stream = (z_stream *)malloc(sizeof(z_stream));
    
    // allocate deflate state
    def_stream->zalloc = 0;
    def_stream->zfree = 0;
    def_stream->opaque = 0;
    
    int ret = deflateInit(def_stream, compression_level);
    if (ret != Z_OK) {
      free(def_stream);
      def_stream = 0;
    }
  }
  return def_stream != 0;
}

void
Deflate::reset() {
  if (def_stream) {
    deflateReset(def_stream);
  }
  current_uncompressed_pos = 0;
  output_buffer.clear();
}

void
Deflate::flush() {
  compress(0, 0, true);
}

unsigned int
Deflate::compress(const void * input, size_t input_len, bool do_flush) {
  if (!init()) {
    return 0;
  }

  unsigned char * in_buffer = new unsigned char[CHUNK];
  unsigned char * out_buffer = new unsigned char[CHUNK];

  unsigned int uncompressed_pos = current_uncompressed_pos;
  current_uncompressed_pos += input_len;
  
  int ret;
  unsigned int pos = 0;
  bool finished = false;
  
  do {
    assert(pos <= input_len);
    
    int flush_v;
    unsigned int len = input_len - pos;
    if (len <= CHUNK) {
      flush_v = do_flush ? Z_SYNC_FLUSH : Z_NO_FLUSH;
      finished = true;
    } else {
      flush_v = Z_NO_FLUSH;
      len = CHUNK;
    }

    if (len) {
      assert(input);
      memcpy(in_buffer, (const unsigned char*)input + pos, len);
      pos += len;
    }

    def_stream->avail_in = len;
    def_stream->next_in = in_buffer;
       
    // run deflate() on input until output buffer not full, finish
    // compression if all of source has been read in
    
    do {
      def_stream->avail_out = CHUNK;
      def_stream->next_out = out_buffer;

      ret = deflate(def_stream, flush_v);    // no bad return value
      assert(ret != Z_STREAM_ERROR);  // state not clobbered

      unsigned int have = CHUNK - def_stream->avail_out;

      assert(have >= 0 && have <= CHUNK);
      output_buffer.append(out_buffer, have);      
    } while (def_stream->avail_out == 0);

    // cerr << "avail_in = " << def_stream->avail_in << ", len = " << len << endl;
    
    assert(def_stream->avail_in == 0);       // all input will be used

  } while (!finished);

  // cerr << "ret = " << ret << endl;
  assert(ret == Z_OK || ret == Z_STREAM_END); // stream will be complete (or not)

  delete[] in_buffer;
  delete[] out_buffer;
  
  return uncompressed_pos;
}
