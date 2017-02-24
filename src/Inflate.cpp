#include "Inflate.h"

extern "C" {
#include <zlib.h>
};

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>

#define CHUNK 65536

using namespace std;

Inflate::Inflate(const std::basic_string<unsigned char> * _input_buffer)
  : input_buffer(_input_buffer)
{

}

Inflate::~Inflate() {   
  if (inf_stream) {
    inflateEnd(inf_stream);
    free(inf_stream);
  }
}

bool
Inflate::init() {
  if (!inf_stream) {
    // allocate inflate state
    inf_stream = (z_stream*)malloc(sizeof(z_stream));
    inf_stream->zalloc = 0;
    inf_stream->zfree = 0;
    inf_stream->opaque = 0;
    inf_stream->avail_in = 0;
    inf_stream->next_in = 0;

    int ret = inflateInit(inf_stream);    
    if (ret != Z_OK) {
      free(inf_stream);
      inf_stream = 0;
    }    
  }
  return inf_stream != 0;
}

string
Inflate::decompressString(unsigned int data_offset, unsigned short data_length) {
  assert(input_buffer);
  
  if (!init()) {
    return "";
  }

  unsigned char * in_buffer = new unsigned char[CHUNK];
  unsigned char * out_buffer = new unsigned char[CHUNK];

  int ret;
  unsigned int pos = 0, output_pos = 0;
  string output_string;

  // decompress until inflate stream ends or end of file
  do {
    assert(pos <= input_buffer->size());
    
    auto len = input_buffer->size() - pos;
    int flush = Z_SYNC_FLUSH;
    if (len > CHUNK) {
      len = CHUNK;
      flush = Z_NO_FLUSH;
    }
    
    memcpy(in_buffer, input_buffer->data() + pos, len);
    pos += len;
    
    inf_stream->avail_in = len;
    inf_stream->next_in = in_buffer;

    // run inflate() on input until output buffer not full
    string tmp;
    do {
      inf_stream->avail_out = CHUNK;
      inf_stream->next_out = out_buffer;
      
      ret = inflate(inf_stream, flush);
      
      if (ret < 0 && ret != Z_BUF_ERROR) {
	cerr << "error: ret\n" << endl; // (before: avail_in = %d, avail_out = %d, after: avail_in = %d, avail_out = %d)\n", ret, len, CHUNK, inf_stream.avail_in, inf_stream.avail_out);
	return "";
      }
      
      unsigned int have = CHUNK - inf_stream->avail_out;
      tmp.append((char*)out_buffer, have);
    } while (inf_stream->avail_out == 0); // && ret != Z_STREAM_END);

    assert(output_pos < data_offset + data_length);
    if (output_pos + tmp.size() > data_offset) {

      unsigned int o1 = data_offset > output_pos ? data_offset - output_pos : 0;
      unsigned int o2 = tmp.size() - (output_pos + tmp.size() > data_offset + data_length ? output_pos + tmp.size() - data_offset - data_length : 0);

      // cerr << "tmp = " << tmp << ", o1 = " << o1 << ", o2 = " << o2 << ", output_pos = " << output_pos << ", tmp_size = " << tmp.size() << ", data_offset = " << data_offset << ", data_length = " << data_length << endl;

      assert(o1 <= tmp.size());
      assert(o2 >= o1);
      assert(o2 - o1 <= tmp.size());
      output_string += tmp.substr(o1, o2 - o1);				  
    }
    output_pos += tmp.size();
        
    // done when inflate() says it's done
  } while (ret != Z_STREAM_END && output_pos < data_offset + data_length);

  delete[] in_buffer;
  delete[] out_buffer;

  return output_string;
}

#if 0
bool
Inflate::decompressStream(const ustring & input, ustring & output) {
  if (!initInflate(false)) {
    fprintf(stderr, "inflate init failed\n");
    return false;
  }

  int ret;
  unsigned int pos = 0;

  // decompress until inflate stream ends or end of file
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
    
    inf_stream->avail_in = len;
    inf_stream->next_in = in_buffer;

    // run inflate() on input until output buffer not full

    do {
      inf_stream->avail_out = CHUNK;
      inf_stream->next_out = out_buffer;
      
      ret = inflate(inf_stream, Z_NO_FLUSH);
      
      if (ret == Z_NEED_DICT || ret < 0) {
	fprintf(stderr, "error: %d\n", ret);
	return false;
      }
      
      unsigned int have = CHUNK - inf_stream->avail_out;
      
      output += ustring(out_buffer, have);
    } while (inf_stream->avail_out == 0);

    // done when inflate() says it's done
  } while (ret != Z_STREAM_END);
  
  return true;
}
#endif
