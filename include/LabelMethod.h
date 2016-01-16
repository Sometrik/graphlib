#ifndef _LABELMETHOD_
#define _LABELMETHOD_

class LabelMethod {
 public:
  enum Method {
    AUTOMATIC_LABEL = 1,
    FIXED_LABEL,
    LABEL_FROM_COLUMN,
  };
 LabelMethod(Method _method = AUTOMATIC_LABEL) : method(_method) { }
 LabelMethod(Method _method, const std::string & _column) : method(_method), column(_column) { }

  Method getValue() const { return method; }
  const std::string & getColumn() const { return column; }
  
 private:
  Method method;
  std::string column;
};

#endif
