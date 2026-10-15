#include "network/internal/exception_reporting.hpp"

#include "diagnostics/logger.hpp"
#include "exception.hpp"

#include <exception>
#include <string>

namespace spk::NetworkInternal
{
	void reportCurrentException(std::string_view context, std::source_location location) noexcept
	{
		try
		{
			const Exception error(std::string(context), std::current_exception(), location);
			SPK_LOG_LOCATED(Error, location) << error.what() << std::endl;
		} catch (...)
		{
			// Reporting must not throw during callback dispatch or shutdown.
		}
	}
}
