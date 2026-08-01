#ifndef MAP_LOGGING_H
#define MAP_LOGGING_H

#include <memory>   // std::shared_ptr
#include <string>   // std::string
#include <string_view>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace map::logging {

    class Logger {
    public:
        static Logger& get_instance() {
            static Logger instance;
            return instance;
        }

        Logger(const Logger&) = delete;
        Logger(Logger&&) = delete;

        void init(std::string name) {
            auto file_sink = std::make_shared < spdlog::sinks::rotating_file_sink_mt(name.c_str(), daily_file_size, max_files_daily, false);
            logger_ = std::make_shared<spdlog::logger>(name.c_str(), file_sink);

            logger_->set_pattern("%Y-%m-%d %H:%M:%S.%e  %v");
            logger_->set_level(spdlog::level::info);

            spdlog::register_logger(logger);
            spdlog::set_default_logger(logger_);
        }

        void set_level(const spdlog::level lvl) { logger_->set_level(lvl); }

        Logger& operator=(const Logger&) = delete;
        Logger& operator=(Logger&&) = delete;

    private:
        std::shared_ptr<spdlog::logger> logger_;

        Logger() = default;
        ~Logger() = default;

        inline constexpr int normal_file_size = 1024 * 1024;
        inline constexpr int large_file_size = normal_file_size * 3;
        inline constexpr int max_files_daily = 8;
        inline constexpr int daily_file_size = normal_file_size * max_files_daily;
        inline constexpr const char* log_name = "logs/logs.txt";
    };

} // namespace map::logging

#endif // MAP_LOGGING_H