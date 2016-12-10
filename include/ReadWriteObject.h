#ifndef _READWRITEOBJECT_H_
#define _READWRITEOBJECT_H_

#include <Mutex.h>

class ReadWriteObject {
 public:
  ReadWriteObject() { }
  virtual ~ReadWriteObject() { }
  
  void lockReader() const {
    MutexLocker locker(mutex);
    num_readers++;
    if (num_readers == 1) writer_mutex.lock();
  }
  void unlockReader() const {
    MutexLocker locker(mutex);
    num_readers--;
    if (num_readers == 0) {
      writer_mutex.unlock();
    }
  }
  void lockWriter() {
    writer_mutex.lock();
  }
  void unlockWriter() {
    writer_mutex.unlock();
  }

 private:
  mutable int num_readers = 0;
  mutable Mutex mutex, writer_mutex;
};

#endif
