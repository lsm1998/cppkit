#include "cppkit/time.hpp"
#include <iostream>

int main()
{
    auto now = cppkit::Time();
    std::cout << "Current time in local zone: " << now << std::endl;

    now = cppkit::Time::Now();
    std::cout << "Current time in local zone: " << now << std::endl;

    auto later = now.Add(cppkit::Hour * 5 + cppkit::Minute * 30);
    std::cout << "Time after 5 hours and 30 minutes: " << later << std::endl;
}