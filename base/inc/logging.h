/*
 * Copyright (c) 2025  The UniversalVision project authors. MIT License. All Rights Reserved.
 *
 * This file is part of UniversalVision(https://github.com/wyewyewye/UniversalVision).
 *
 * See LICENSE file for full terms.
 */

#ifdef ENABLE_BOOST_LOG
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/filesystem.hpp>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace src = boost::log::sources;
namespace attrs = boost::log::attributes;

typedef sinks::asynchronous_sink<sinks::text_ostream_backend> async_ostream_sink_t;
typedef sinks::asynchronous_sink<sinks::text_file_backend> async_file_sink_t;

src::severity_logger<logging::trivial::severity_level> globalLogger;

#define LOG_TRACE(TAG) BOOST_LOG_SEV(globalLogger, logging::trivial::severity_level::trace) \
	<< boost::log::add_value("File", __FILE__) \
	<< boost::log::add_value("Line", __LINE__) \
	<< "[" << TAG << "]"
#define LOG_INFO(TAG) BOOST_LOG_SEV(globalLogger, logging::trivial::severity_level::info) \
	<< boost::log::add_value("File", __FILE__) \
	<< boost::log::add_value("Line", __LINE__) \
	<< "[" << TAG << "]"
#define LOG_WARN(TAG) BOOST_LOG_SEV(globalLogger, logging::trivial::severity_level::warning) \
	<< boost::log::add_value("File", __FILE__) \
	<< boost::log::add_value("Line", __LINE__) \
	<< "[" << TAG << "]"
#define LOG_ERROR(TAG) BOOST_LOG_SEV(globalLogger, logging::trivial::severity_level::error) \
	<< boost::log::add_value("File", __FILE__) \
	<< boost::log::add_value("Line", __LINE__) \
	<< "[" << TAG << "]"
#define LOG_FATAL(TAG) BOOST_LOG_SEV(globalLogger, logging::trivial::severity_level::fatal) \
	<< boost::log::add_value("File", __FILE__) \
	<< boost::log::add_value("Line", __LINE__) \
	<< "[" << TAG << "]"

// Use named_scope attribute to get the file and line number
// #define LOG_TRACE(TAG) BOOST_LOG_FUNCTION();BOOST_LOG_SEV(globalLogger, logging::trivial::severity_level::trace) \
// 	<< "[" << TAG << "]"
// #define LOG_INFO(TAG) BOOST_LOG_FUNCTION();BOOST_LOG_SEV(globalLogger, logging::trivial::severity_level::info) \
// 	<< "[" << TAG << "]"
// #define LOG_WARN(TAG) BOOST_LOG_FUNCTION();BOOST_LOG_SEV(globalLogger, logging::trivial::severity_level::warning) \
// 	<< "[" << TAG << "]"
// #define LOG_ERROR(TAG) BOOST_LOG_FUNCTION();BOOST_LOG_SEV(globalLogger, logging::trivial::severity_level::error) \
// 	<< "[" << TAG << "]"
// #define LOG_FATAL(TAG) BOOST_LOG_FUNCTION();BOOST_LOG_SEV(globalLogger, logging::trivial::severity_level::fatal) \
// 	<< "[" << TAG << "]"

namespace univision 
{
	/*
	Initialize the logging system.
		AppMessage -> Logger -> LoggingCore -> FileSink/ConsoleSink(FrontEnd) -> BackEnd(ostream/file)
	
	*/
	void initLog()
	{
        auto core = logging::core::get();

		// Create a rotating text file backend(BackEnd)
		auto fileBackend = boost::make_shared<sinks::text_file_backend>
		(
			keywords::file_name = "UniversalVision.log",
			keywords::target_file_name = "%Y%m%d_%H%M%S_%5N.log",
			keywords::rotation_size = 100 * 1024 * 1024
			// keywords::time_based_rotation = sinks::file::rotation_at_time_point(12, 0 ,0)
		);

		// Create a rotating text file sink(FrontEnd)
		auto fileSink = boost::make_shared<async_file_sink_t>(fileBackend);

        // Set up where the rotated files will be stored
        fileSink->locked_backend()->set_file_collector(
			sinks::file::make_collector(
				keywords::target = "UniversalVisionLogs",                // where to store rotated files
				keywords::max_size = 10 * 1024 * 1024 * 1024,            // maximum total size of the stored files, in bytes
				keywords::min_free_space = 3 * 1024 * 1024 * 1024,       // minimum free space on the drive, in bytes
				keywords::max_files = 1024                               // maximum number of stored files
			)
		);
		fileSink->locked_backend()->scan_for_files();
		fileSink->set_formatter
		(
			expr::format("[%1%][%2%][%3%] %4% (%5%:%6%)")
				% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
				% expr::attr<attrs::current_thread_id::value_type>("ThreadID")
				% expr::attr<logging::trivial::severity_level>("Severity")
				% expr::smessage
				// % expr::format_named_scope("Scope", keywords::format = "<%n>(%f:%l)")
				% expr::attr<std::string>("File")
				% expr::attr<int>("Line")
		);
		fileSink->locked_backend()->auto_flush(true);

		// Add the file sink to the logging core
		core->add_sink(fileSink);

		auto consoleBackend = boost::make_shared<sinks::text_ostream_backend>();
		consoleBackend->add_stream(boost::shared_ptr< std::ostream >(&std::clog, boost::null_deleter()));

		auto consoleSink = boost::make_shared<async_ostream_sink_t>(consoleBackend);
		consoleSink->set_formatter
		(
			expr::format("[%1%][tid=%2%][%3%] %4% (%5%:%6%)")
				% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
				% expr::attr<attrs::current_thread_id::value_type>("ThreadID")
				% expr::attr<logging::trivial::severity_level>("Severity")
				% expr::smessage
				// % expr::format_named_scope("Scope", keywords::format = "<%n>(%f:%l)")
				% expr::attr<std::string>("File")
				% expr::attr<int>("Line")
		);
		core->add_sink(consoleSink);

		core->set_filter
		(
#ifdef DEBUG
			logging::trivial::severity >= logging::trivial::trace
#else
			logging::trivial::severity >= logging::trivial::info
#endif
		);

		logging::add_common_attributes();
		// logging::core::get()->add_thread_attribute("Scope", attrs::named_scope());

		boost::filesystem::path p(boost::filesystem::current_path());
		LOG_INFO("Logging") << "LogDir=" << p;
	}
} // namespace univision

#endif