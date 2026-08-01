#ifndef MAP_LOGGING_H
#define MAP_LOGGING_H

#include <memory>   // std::shared_ptr
#include <string>   // std::string
#include <string_view>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace map::logging {

    inline constexpr int normal_file_size = 1024 * 1024;
    inline constexpr int large_file_size = normal_file_size * 3;
    inline constexpr int max_files_daily = 8;
    inline constexpr int daily_file_size = normal_file_size * max_files_daily;
    inline constexpr const char* log_name = "logs/logs.txt";

    class Logger {
    public:
        static Logger& get_instance() {
            static Logger instance;
            return instance;
        }

        Logger(const Logger&) = delete;
        Logger(Logger&&) = delete;

        void init(std::string name) {
            // TODO: create sinks + logger
            // auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            // ...
        }

        Logger& operator=(const Logger&) = delete;
        Logger& operator=(Logger&&) = delete;

    private:
        std::shared_ptr<spdlog::logger> logger_;

        Logger() = default;
        ~Logger() = default;
    };

} // namespace map::logging

#endif // MAP_LOGGING_H