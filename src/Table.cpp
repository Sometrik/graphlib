#include "Table.h"

#include <StringUtils.h>

#include "TextColumn.h"
#include "CompressedTextColumn.h"
#include "TimeSeriesColumn.h"

#include <fstream>
#include <iostream>
#include <cassert>

using namespace std;
using namespace table;

void
Table::loadCSV(const char * filename, char delimiter) {
  ifstream in;
  in.open(filename);
  assert(in);
  
  cerr << "reading csv\n";

  unsigned int n = 0;
  while (!in.eof() && !in.fail()) {
    string s;
    getline(in, s);
    StringUtils::trim(s);
    if (s.empty()) continue;
    vector<string> row = StringUtils::split(s, delimiter);

    cerr << "got " << row.size() << " columns\n";

    assert(n < size());
	     
    for (unsigned int i = 0; i < row.size(); i++) {
      assert(i < columns_in_order.size());
      cerr << "setting value to column " << i << ": " << row[i] << endl;
      columns_in_order[9 + i]->setValue(n, row[i]);
    }
    cerr << "done setting values\n";
    n++;
  }
}

ColumnBase &
Table::addTextColumn(const char * name) {
  auto it = columns.find(name);
  if (it != columns.end()) {
    return *(it->second);
  } else {
    return addColumn(name, std::make_shared<TextColumn>());
  }
}

ColumnBase &
Table::addCompressedTextColumn(const char * name) {
  auto it = columns.find(name);
  if (it != columns.end()) {
    return *(it->second);
  } else {
    return addColumn(name, std::make_shared<CompressedTextColumn>());
  }
}

ColumnBase &
Table::addDoubleColumn(const char * name) {
  auto it = columns.find(name);
  if (it != columns.end()) {
    return *(it->second);
  } else {
    return addColumn(name, std::make_shared<Column<double> >());
  }
}

ColumnBase &
Table::addIntColumn(const char * name) {
  auto it = columns.find(name);
  if (it != columns.end()) {
    return *(it->second);
  } else {
    return addColumn(name, std::make_shared<Column<int> >());
  }
}

ColumnBase &
Table::addUShortColumn(const char * name) {
  auto it = columns.find(name);
  if (it != columns.end()) {
    return *(it->second);
  } else {
    return addColumn(name, std::make_shared<Column<unsigned short> >());
  }
}

ColumnBase &
Table::addBigIntColumn(const char * name) {
  auto it = columns.find(name);
  if (it != columns.end()) {
    return *(it->second);
  } else {
    return addColumn(name, std::make_shared<Column<long long> >());
  }
}

ColumnBase &
Table::addTimeSeriesColumn(const char * name) {
  auto it = columns.find(name);
  if (it != columns.end()) {
    return *(it->second);
  } else {
    return addColumn(name, std::make_shared<TimeSeriesColumn>());
  }
}
