#pragma once
#include <chrono>
#include <string>
#include <iostream>

class LogDuration {
public:
    LogDuration();

    LogDuration(std::string&& log_name, std::ostream& out);

    ~LogDuration();

private:
    // В переменной будет время конструирования объекта LogDuration
    std::string log_name_;
    std::ostream& out_ = std::cerr;
    const std::chrono::steady_clock::time_point start_time_ = std::chrono::steady_clock::now();
};

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION_STREAM(name, stream) LogDuration UNIQUE_VAR_NAME_PROFILE((name), (stream))