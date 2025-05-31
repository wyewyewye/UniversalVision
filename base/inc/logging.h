#include <boost/log/utility/setup/file.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sources/logger.hpp>

namespace logging = boost::log;

namespace univision 
{
	void init()
	{
		logging::add_file_log("sample.log");

		logging::core::get()->set_filter
		(
			logging::trivial::severity >= logging::trivial::info
		);

#if ENABLE_BOOST_LOG

		BOOST_LOG_TRIVIAL(info) << "Enable boost log";

#endif

		BOOST_LOG_TRIVIAL(info) << "wye test";
	}
} // namespace univision