#include "DBase3File.h"

#include "utf8.h"

#ifndef _WIN32
#include "../system/IConv.h"
#endif

#include <cstdio>
#include <cstring>
#include <cassert>

using namespace std;
using namespace table;

DBase3File::DBase3File(const string & filename)
 {
  dbf = 0;
  record_count = 0;

  openDBF(filename);
}

DBase3File::~DBase3File() {
  if (dbf) DBFClose(dbf);
}

bool
DBase3File::openDBF(const string & filename) { 
  dbf = DBFOpen(filename.c_str(), "rb");
  if (!dbf) {
    fprintf(stderr, "failed to open dbf file\n");
    return false;
  }
  unsigned int field_count = DBFGetFieldCount(dbf);
  record_count = DBFGetRecordCount(dbf);
  
  char fieldname[255];
  for (unsigned int i = 0; i < field_count; i++) {
    DBFFieldType type = DBFGetFieldInfo(dbf, i, fieldname, 0, 0);
    ColumnDBase3 * col = new ColumnDBase3(dbf, i, record_count, fieldname);
    columns.push_back(std::shared_ptr<Column>(col));
    // addField(fieldname);
  }
  
  return true;
}

ColumnDBase3::ColumnDBase3(DBFHandle _dbf,
			   int _column_index,
			   int _num_rows,
			   const std::string & _name)
: Column(_name),
  dbf(_dbf),
  column_index(_column_index),
  num_rows(_num_rows)
{

}

std::string
ColumnDBase3::getText(int i) const {
  assert(i >= 0 && i < num_rows);
  const char * tmp = DBFReadStringAttribute(dbf, i, column_index);
  if (tmp) {
#ifdef _WIN32
    string output;
    while (!tmp) {
      utf8::append((unsigned char)*tmp, back_inserter(output));
      tmp++;
    }
    return output;
#else
    IConv iconv("ISO8859-1", "UTF-8");
    string tmp2;
    if (iconv.convert(tmp, tmp2)) {
      return tmp2;
    }
#endif
  }
  return "";
}
