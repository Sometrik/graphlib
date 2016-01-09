#include "Table.h"

#include "../system/StringUtils.h"

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
