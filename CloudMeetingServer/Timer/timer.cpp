#include "timer.h"
#include <utility>
using namespace tiny_muduo;
tiny_muduo::Timer::Timer(Timestamp timestamp, BasicFunc &&cb, double interval=0.0):
        expiration_(timestamp),
        callback_(std::move(cb)),
        interval_(interval),
        repeat_(interval>0.0)
{
}