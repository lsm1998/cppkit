#pragma once

#include <mutex>
#include <sstream>

namespace cppkit
{
    inline std::mutex gPrintMutex;

    class sync_stream : public std::ostringstream
    {
    public:
        explicit sync_stream(std::ostream& os) : os_(os)
        {
        }

        ~sync_stream() override
        {
            std::lock_guard lock(gPrintMutex);
            os_ << this->str();
            os_.flush();
        }

    private:
        std::ostream& os_;
    };
}
