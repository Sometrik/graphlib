#ifndef _LABELMETHOD_
#define _LABELMETHOD_

class LabelMethod {
 public:
  enum Method {
    AUTOMATIC_LABEL = 1,
    LABEL_FROM_COLUMN,
  };
 LabelMethod(Method _method = AUTOMATIC_LABEL) : method(_method) { }
 LabelMethod(Method _method, const std::string & _column) : method(_method), column(_column) { }
 LabelMethod(Method _method, const std::string & _column, const std::string & _priority_column) : method(_method), column(_column), priority_column(_priority_column) { }

  Method getValue() const { return method; }
  const std::string & getColumn() const { return column; }
  const std::string & getPriorityColumn() const { return priority_column; }
  
 private:
  Method method;
  std::string column, priority_column;
};

#endif
