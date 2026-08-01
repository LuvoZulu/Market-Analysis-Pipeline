#ifndef MAP_LOGGING_H
#define MAP_LOGGING_H

#include <memory> //std::shared_ptr
#include <string> //std::string

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace map::logging {

    class Logger {
    public:
        static Logger& get_instance() {
            static Logger instance;
            return instance;
        }

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        void init(const std::string& name = "Market_Analysis_Pipeline") {
            if (logger_) return;

            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::debug);

            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                "logs/" + name + ".log",
                daily_file_size,
                max_files,
                false
            );
            file_sink->set_level(spdlog::level::trace);

            std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };
            logger_ = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());

            logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
            logger_->set_level(spdlog::level::info);
            logger_->flush_on(spdlog::level::err);

            spdlog::register_logger(logger_);
            spdlog::set_default_logger(logger_);
        }
       
        std::shared_ptr<spdlog::logger> get() {
            if (!logger_) {
                init();
            }
            return logger_;
        }

        std::shared_ptr<spdlog::logger> get(const std::string& name) {
            auto existing = spdlog::get(name);
            if (existing) return existing;

            if (!logger_) init();

            auto new_logger = logger_->clone(name);
            spdlog::register_logger(new_logger);
            return new_logger;
        }

        void set_level(spdlog::level::level_enum lvl) {
            if (logger_) {
                logger_->set_level(lvl);
            }
        }

    private:
        Logger() = default;
        ~Logger() = default;

        std::shared_ptr<spdlog::logger> logger_;

        static constexpr int normal_file_size = 1024 * 1024;
        static constexpr int max_files = 8;
        static constexpr int daily_file_size = normal_file_size * max_files; 
    };

} // namespace map::logging

#define LOG_TRACE(...)    SPDLOG_LOGGER_TRACE(map::logging::Logger::get_instance().get(), __VA_ARGS__)
#define LOG_DEBUG(...)    SPDLOG_LOGGER_DEBUG(map::logging::Logger::get_instance().get(), __VA_ARGS__)
#define LOG_INFO(...)     SPDLOG_LOGGER_INFO (map::logging::Logger::get_instance().get(), __VA_ARGS__)
#define LOG_WARN(...)     SPDLOG_LOGGER_WARN (map::logging::Logger::get_instance().get(), __VA_ARGS__)
#define LOG_ERROR(...)    SPDLOG_LOGGER_ERROR(map::logging::Logger::get_instance().get(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(map::logging::Logger::get_instance().get(), __VA_ARGS__)

#define LOG_MD(...)       SPDLOG_LOGGER_INFO(map::logging::Logger::get_instance().get("market_data"), __VA_ARGS__)
#define LOG_OMS(...)      SPDLOG_LOGGER_INFO(map::logging::Logger::get_instance().get("oms"), __VA_ARGS__)
#define LOG_RISK(...)     SPDLOG_LOGGER_INFO(map::logging::Logger::get_instance().get("risk"), __VA_ARGS__)

#endif // MAP_LOGGING_H