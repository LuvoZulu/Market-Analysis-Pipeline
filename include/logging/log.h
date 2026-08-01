#ifndef MAP_LOGGING_H
#define MAPE_LOGGING_H

#include <memory> // std::shared_ptr


namespace map:logging {
	class Logger {
	public:
		static Logger& get_instance() {
			static Logger instance;
			return instance;
		}

		Logger(const Logger&) = delete;
		Logger(Logger&&) = delete;

		Logger& operator=(const Logger&) = delete;
		Logger& operator=(const Logger&&) = delete;
	private:

		Logging(){}
		~Logging(){}
	};
}




#endif // !MAP_LOGGING_H