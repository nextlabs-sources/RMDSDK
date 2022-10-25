

#include <Windows.h>
#include <assert.h>

#include <nudf/timer.hpp>


using namespace NX;

#define TIMER_FLAG_ACTIVE           0x0000001
#define TIMER_FLAG_FORCE_TRIGGER    0x0000002

timer::timer() : _interval(0), _h(NULL), _internal_flags(0)
{
    _h = ::CreateSemaphoreW(NULL, 0, 1, NULL);
}

timer::timer(unsigned long interval) : _interval(interval), _h(NULL), _internal_flags(0)
{
    _h = ::CreateSemaphoreW(NULL, 0, 1, NULL);
}

timer::~timer()
{
    stop();
    if (NULL != _h) {
        ::CloseHandle(_h);
        _h = NULL;
    }
}

bool timer::start()
{
    if (NULL == _h) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }
    if (0 == _interval) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }
    if (active()) {
        return true;
    }

    bool result = false;

    try {
        _internal_flags |= TIMER_FLAG_ACTIVE;
        _t = std::thread(timer::worker, this);
        result = true;
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        _internal_flags &= ~TIMER_FLAG_ACTIVE;
        result = false;
    }

    return result;
}

void timer::stop()
{
    if (active()) {
        _internal_flags &= ~TIMER_FLAG_ACTIVE;
        ::ReleaseSemaphore(_h, 1, NULL);
    }
    if (_t.joinable()) {
        _t.join();
    }
}

bool  timer::active() const
{
    return (TIMER_FLAG_ACTIVE == (_internal_flags & TIMER_FLAG_ACTIVE));
}

void timer::reset(unsigned long interval)
{
    _interval = interval;
    if (active()) {
        ::ReleaseSemaphore(_h, 1, NULL);
    }
}

bool timer::force_trigger()
{
    if (!active()) {
        return false;
    }
    _internal_flags |= TIMER_FLAG_FORCE_TRIGGER;
    ::ReleaseSemaphore(_h, 1, NULL);
    return true;
}

void timer::worker(timer* context)
{
    assert(nullptr != context);
    while (context->active()) {

        DWORD wait_result = ::WaitForSingleObject(context->_h, context->_interval);
        if (WAIT_OBJECT_0 == wait_result) {

            if (TIMER_FLAG_FORCE_TRIGGER == (context->_internal_flags & TIMER_FLAG_FORCE_TRIGGER)) {
                // trigger event
                context->_internal_flags &= ~TIMER_FLAG_FORCE_TRIGGER;
                context->on_timer();
            }
        }
        else if (WAIT_TIMEOUT == wait_result) {
            context->on_timer();
        }
        else {
            ;
        }
    }
}