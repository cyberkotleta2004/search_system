#include "log_duration.h"

using namespace std::string_literals;


LogDuration::LogDuration() {}

LogDuration::LogDuration(std::string&& log_name, std::ostream& out = std::cerr)
    : log_name_(std::move(log_name))
    , out_(out)
{}

LogDuration::~LogDuration() {
    const auto end_time = std::chrono::steady_clock::now();
    const auto dur = end_time - start_time_;
    out_ << log_name_ << ": "s
        << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count() 
        << " ms"s << std::endl;
}
