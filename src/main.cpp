#include "logging/Logging.h"

int main() {


	auto& logger = map::logging::Logger::get_instance();
	logger.init("MarketAnalysis");



	return 0;
}