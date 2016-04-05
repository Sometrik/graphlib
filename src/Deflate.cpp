#include "Deflate.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#define CHUNK 65536

using namespace std;

Deflate::Deflate(int _compression_level) : compression_level(_compression_level)
{
  in_buffer = new unsigned char[CHUNK];
  out_buffer = new unsigned char[CHUNK];    
}

Deflate::Deflate(const Deflate & other)
  : compression_level(other.compression_level) {
  
  in_buffer = new unsigned char[CHUNK];
  out_buffer = new unsigned char[CHUNK];
}

Deflate &
Deflate::operator=(const Deflate & other) {
  if (this != &other) {
    compression_level = other.compression_level;
    deflate_init = inflate_init = false;
  }
  return *this;
}

Deflate::~Deflate() {   
  if (deflate_init) {
    deflateEnd(&def_stream);
  }
  if (inflate_init) {
    inflateEnd(&inf_stream);
  }

  delete[] in_buffer;
  delete[] out_buffer;
}

bool
Deflate::initDeflate() {
  int ret;
  if (!deflate_init) {
    deflate_init = true;
    
    // allocate deflate state
    def_stream.zalloc = 0;
    def_stream.zfree = 0;
    def_stream.opaque = 0;
    
    ret = deflateInit(&def_stream, compression_level);
  } else {
    ret = deflateReset(&def_stream);
  }
    
  assert(ret == Z_OK);
    
  return ret == Z_OK;
}

bool
Deflate::initInflate(bool reset) {
  int ret;
  if (!inflate_init) {
    inflate_init = true;

    // allocate inflate state
    inf_stream.zalloc = 0;
    inf_stream.zfree = 0;
    inf_stream.opaque = 0;
    inf_stream.avail_in = 0;
    inf_stream.next_in = 0;

    ret = inflateInit(&inf_stream);    
  } else if (reset) {
    ret = inflateReset(&inf_stream);
  } else {
    return true;
  }

  // assert(ret == Z_OK);
  
  if (ret == Z_OK) {
    return true;
  } else {
    return false;
  }
}

void
Deflate::reset() {
  if (inflate_init) {
    initInflate();
  }
  if (deflate_init) {
    initDeflate();
  }		 
}
		 
bool
Deflate::decompress(const ustring & input, ustring & output) {
  if (!initInflate()) {
    fprintf(stderr, "deflate init failed\n");
    return false;
  }

  int ret;
  unsigned int pos = 0;
  // unsigned int iter_count = 0;

  // decompress until deflate stream ends or end of file
  do {
    assert(pos <= input.size());
    
    unsigned int len = input.size() - pos;
    if (len > CHUNK) {
      len = CHUNK;
    }
    
    memcpy(in_buffer, input.data() + pos, len);
    pos += len;
    
    inf_stream.avail_in = len;
    inf_stream.next_in = in_buffer;

    // run inflate() on input until output buffer not full

    do {
//      if (++iter_count > 10000) {
//	fprintf(stderr, "Deflate: iter count too big, ret = %d\n", ret);
//	return false;
//      }
      inf_stream.avail_out = CHUNK;
      inf_stream.next_out = out_buffer;
      
      // fprintf(stderr, "inflating\n");
      ret = inflate(&inf_stream, Z_NO_FLUSH);
      // fprintf(stderr, "inflating done, ret = %d\n", ret);
      
      if (ret < 0 && ret != Z_BUF_ERROR) {
	fprintf(stderr, "error: %d (before: avail_in = %d, avail_out = %d, after: avail_in = %d, avail_out = %d)\n", ret, len, CHUNK, inf_stream.avail_in, inf_stream.avail_out);
	return false;
      }
      
      unsigned int have = CHUNK - inf_stream.avail_out;
      
      output += ustring(out_buffer, have);
    } while (inf_stream.avail_out == 0); // && ret != Z_STREAM_END);

    // done when inflate() says it's done
    // what if we still have stuff to decompress?
  } while (ret != Z_STREAM_END);

  return true;
}

bool
Deflate::decompressStream(const ustring & input, ustring & output) {
  if (!initInflate(false)) {
    fprintf(stderr, "deflate init failed\n");
    return false;
  }

  int ret;
  unsigned int pos = 0;

  // decompress until deflate stream ends or end of file
  do {
    assert(pos <= input.size());
    
    unsigned int len = input.size() - pos;
    if (len > CHUNK) {
      len = CHUNK;
    }

    if (!len) {
      break;
    }
    
    memcpy(in_buffer, input.data() + pos, len);
    pos += len;
    
    inf_stream.avail_in = len;
    inf_stream.next_in = in_buffer;

    // run inflate() on input until output buffer not full

    do {
      inf_stream.avail_out = CHUNK;
      inf_stream.next_out = out_buffer;
      
      ret = inflate(&inf_stream, Z_NO_FLUSH);
      
      if (ret == Z_NEED_DICT || ret < 0) {
	fprintf(stderr, "error: %d\n", ret);
	return false;
      }
      
      unsigned int have = CHUNK - inf_stream.avail_out;
      
      output += ustring(out_buffer, have);
    } while (inf_stream.avail_out == 0);

    // done when inflate() says it's done
  } while (ret != Z_STREAM_END);
  
  return true;
}

bool
Deflate::compress(const ustring & input, ustring & output) {
  if (!initDeflate()) {
    return false;
  }
  
  int ret, flush;
  unsigned int pos = 0;

  do {
    assert(pos <= input.size());
    
    flush = Z_FINISH;
    unsigned int len = input.size() - pos;
    if (len > CHUNK) {
      flush = Z_NO_FLUSH;
      len = CHUNK;
    }

    memcpy(in_buffer, input.data() + pos, len);
    pos += len;

    def_stream.avail_in = len;
    def_stream.next_in = in_buffer;
       
    // run deflate() on input until output buffer not full, finish
    // compression if all of source has been read in
    
    do {
      def_stream.avail_out = CHUNK;
      def_stream.next_out = out_buffer;

      ret = deflate(&def_stream, flush);    // no bad return value
      assert(ret != Z_STREAM_ERROR);  // state not clobbered

      unsigned int have = CHUNK - def_stream.avail_out;

      output += ustring(out_buffer, have);
      
    } while (def_stream.avail_out == 0);
    
    assert(def_stream.avail_in == 0);       // all input will be used

  } while (flush != Z_FINISH);
  
  assert(ret == Z_STREAM_END);        // stream will be complete

  return true;
}
