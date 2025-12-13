#include "cppkit/time.hpp"
#include "cppkit/timer.hpp"
#include <iostream>
#include <unistd.h>

int main()
{
    auto now = cppkit::Time();
    std::cout << "Current time in local zone: " << now << std::endl;

    now = cppkit::Time::Now();
    std::cout << "Current time in local zone: " << now << std::endl;

    const auto later = now.Add(cppkit::Hour * 5 + cppkit::Minute * 30);
    std::cout << "Time after 5 hours and 30 minutes: " << later << std::endl;


    sleep(2);

    std::cout << "diff:" << cppkit::Time::Since(now) << std::endl;

    cppkit::Timer timer;

    timer.setTimeout(cppkit::Second * 1, []
    {
        std::cout << "setTimeout执行 " << cppkit::Time::Now().Format() << std::endl;
    });

    auto id = timer.setInterval(cppkit::Second * 1, []
    {
        std::cout << "setInterval执行 " << cppkit::Time::Now().Format() << std::endl;
    });

    timer.setTimeout(cppkit::Second * 3, [&timer,id]
    {
        timer.cancel(id);
    });


    sleep(10);
}
