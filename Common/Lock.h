#ifndef __LOCK_H__
#define __LOCK_H__

#include <pthread.h>


class Lock {
public:
    Lock();
    ~Lock();
    
    void lock();
    void unlock();
    
    bool tryLock();
    
private:
    pthread_mutex_t _mutex;
};

class Locker {
public:
    Locker(Lock& lock) : _lock(lock) { _lock.lock(); }
    ~Locker() { _lock.unlock(); }
private:
    Lock &_lock;
};


#endif
