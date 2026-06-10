#pragma once

#include <iostream>
#include <pthread.h>

class Mutex
{
public:
    Mutex()
    {
        pthread_mutex_init(&_lock, nullptr);
    }
    ~Mutex()
    {
        pthread_mutex_destroy(&_lock);
    }
    void Lock()
    {
        pthread_mutex_lock(&_lock);
    }
    void UnLock()
    {
        pthread_mutex_unlock(&_lock);
    }
    pthread_mutex_t* Get()
    {
        return &_lock;
    }
private:
    pthread_mutex_t _lock;
};

class MutexGuard
{
public:
    MutexGuard(Mutex& mutex)
        : _mutex(mutex)
    {
        _mutex.Lock();
    }
    ~MutexGuard()
    {
        _mutex.UnLock();
    }
private:
    Mutex& _mutex;
};