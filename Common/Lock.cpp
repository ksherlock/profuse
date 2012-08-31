#include "Lock.h"


Lock::Lock()
{
    pthread_mutex_init(&_mutex, NULL);
}

Lock::~Lock()
{
    pthread_mutex_destroy(&_mutex);
}

void Lock::lock()
{
    pthread_mutex_lock(&_mutex);
}

void Lock::unlock()
{
    pthread_mutex_unlock(&_mutex);
}

bool Lock::tryLock()
{
    return pthread_mutex_trylock(&_mutex) == 0;
}
