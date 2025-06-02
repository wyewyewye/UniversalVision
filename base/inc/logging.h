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
#include <boost/core/null_deleter.hpp>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

typedef sinks::asynchronous_sink<sinks::text_ostream_backend> async_ostream_sink_t;
typedef sinks::asynchronous_sink<sinks::text_file_backend> async_file_sink_t;

namespace univision 
{
	void initLog()
	{
		//logging::add_file_log("sample.log");

        auto core = logging::core::get();

		auto fileBackend = boost::make_shared<sinks::text_file_backend>
		(
			keywords::file_name = "UniversalVision.log",
			keywords::target_file_name = "UniversalVision_%Y%m%d_%5N.log",
			keywords::rotation_size = 100 * 1024 * 1024,
			keywords::time_based_rotation = sinks::file::rotation_at_time_point(12, 0 ,0)
		);
		auto fileSink = boost::make_shared<async_file_sink_t>(fileBackend);
		fileSink->set_formatter
		(
			expr::format("[%1%][%2%][%3%]%4%[%5%]")
			% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y%m%d %H%M%S.%f")
			% expr::attr<unsigned int>("ThreadID")
			% logging::trivial::severity
			% expr::smessage
			% expr::attr<unsigned int>("LineID")
		);
		fileSink->locked_backend()->auto_flush(true);
		core->add_sink(fileSink);

		auto consoleBackend = boost::make_shared<sinks::text_ostream_backend>();
		consoleBackend->add_stream(boost::shared_ptr< std::ostream >(&std::clog, boost::null_deleter()));

		auto consoleSink = boost::make_shared<async_ostream_sink_t>(consoleBackend);
		consoleSink->set_formatter
		(
			expr::format("[%1%][%2%][%3%]%4%[%5%]")
				% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y%m%d %H%M%S.%f")
				% expr::attr<unsigned int>("ThreadID")
				% logging::trivial::severity
				% expr::smessage
				% expr::attr<unsigned int>("LineID")
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
	}
} // namespace univision

#define LOG_INFO(TAG) BOOST_LOG_TRIVIAL(info) << "[" << TAG << "]"
#define LOG_WARN(TAG) BOOST_LOG_TRIVIAL(warning) << "[" << TAG << "]"
#define LOG_ERROR(TAG) BOOST_LOG_TRIVIAL(error) << "[" << TAG << "]"
#define LOG_FATAL(TAG) BOOST_LOG_TRIVIAL(fatal) << "[" << TAG << "]"

#endif