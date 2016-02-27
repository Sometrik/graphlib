#include "DBase3File.h"

#include "utf8.h"

#ifndef _WIN32
#include "../system/IConv.h"
#endif

#include <cstring>
#include <cassert>
#include <shapefil.h>
#include <iostream>

using namespace std;
using namespace table;

class table::DBase3Handle {
public:
  DBase3Handle() { }
  ~DBase3Handle() {
    if (h) {
      DBFClose(h);
    }
  }

  void open(const std::string & fn) {
    h = DBFOpen(fn.c_str(), "rb");
    if (!h) {
      cerr << "failed to open DBF " << fn << endl;
    }
  }
  
  int readIntegerAttribute(int rec, int field) { return DBFReadIntegerAttribute(h, rec, field); }
  double readDoubleAttribute(int rec, int field) { return DBFReadDoubleAttribute(h, rec, field); }
  std::string readStringAttribute(int rec, int field) {
    const char * tmp = DBFReadStringAttribute(h, rec, field);
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
  bool readBoolAttribute(int rec, int field) { return DBFReadLogicalAttribute(h, rec, field); }
  bool isNull(int rec, int field) { return DBFIsAttributeNULL(h, rec, field); }
  unsigned int getRecordCount() { return h ? DBFGetRecordCount(h) : 0; }
  unsigned int getFieldCount() { return h ? DBFGetFieldCount(h) : 0; }
  string getFieldName(int field) {
    char fieldname[255];
    DBFFieldType type = DBFGetFieldInfo(h, field, fieldname, 0, 0);
    return fieldname;
  }
  
private:
  DBFHandle h = 0;
};

DBase3File::DBase3File(const string & filename) {
  record_count = 0;
  openDBF(filename);
}

bool
DBase3File::openDBF(const string & filename) {
  dbf = std::make_shared<DBase3Handle>();
  dbf->open(filename);
  record_count = dbf->getRecordCount();
  unsigned int field_count = dbf->getFieldCount();
  for (unsigned int i = 0; i < field_count; i++) {
    string name = dbf->getFieldName(i);
    columns.push_back(std::make_shared<ColumnDBase3>(dbf, i, record_count, name));
  }
  return true;
}

ColumnDBase3::ColumnDBase3(const std::shared_ptr<DBase3Handle> _dbf,
			   int _column_index,
			   int _num_rows,
			   const std::string & _name)
: Column(_name),
  dbf(_dbf),
  column_index(_column_index),
  num_rows(_num_rows)
{

}

long long
ColumnDBase3::getInt64(int i) const {
  if (i >= 0 && i < num_rows) {
    return dbf->readIntegerAttribute(i, column_index);
  } else {
    return 0;
  }
}
    
std::string
ColumnDBase3::getText(int i) const {
  if (i >= 0 && i < num_rows) {
    return dbf->readStringAttribute(i, column_index);
  } else {
    return "";
  }
}

double
ColumnDBase3::getDouble(int i) const {
  if (i >= 0 && i < num_rows) {
    return dbf->readIntegerAttribute(i, column_index);
  } else {
    return 0;
  }
}

int
ColumnDBase3::getInt(int i) const {
  if (i >= 0 && i < num_rows) {
    return dbf->readIntegerAttribute(i, column_index);
  } else {
    return 0;
  }
}
