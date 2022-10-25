

#ifndef __NUDF_TIMER_HPP__
#define __NUDF_TIMER_HPP__



#include <thread>

namespace NX {


class timer
{
public:
    timer();
    timer(unsigned long interval);
    virtual ~timer();

    bool start();
    void stop();
    void reset(unsigned long interval);
    bool active() const;
    bool force_trigger();

    static void worker(timer* context);

protected:
    virtual void on_timer() {}
    virtual void on_stop() {}

    inline unsigned long get_interval() const { return _interval; }

private:
    HANDLE _h;
    unsigned long _interval;
    std::thread _t;
    unsigned long _internal_flags;
};

}


#endif