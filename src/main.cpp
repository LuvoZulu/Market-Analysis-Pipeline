#include "logging/Logging.h"

int main() {
    
    map::logging::Logger::get_instance().init("Market_Analysis_Pipeline");
    
    LOG_INFO("Application started");
    LOG_DEBUG("Debug value = {}", 42);
    LOG_WARN("Something looks off");
    LOG_ERROR("Failed with code {}", 123);

    LOG_MD("Tick received: price={}", 150.25);
    LOG_OMS("Order sent: id={}", 98765);
    LOG_RISK("Risk limit approaching");

    return 0;
}