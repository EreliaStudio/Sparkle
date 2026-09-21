#include <gtest/gtest.h>

#include <iostream>
#include <stdexcept>
#include <string>

#include <Windows.h>

namespace
{
	class PrivateWindowStation
	{
	private:
		HWINSTA _station = nullptr;
		HDESK _desktop = nullptr;

	public:
		PrivateWindowStation()
		{
			const std::wstring stationName =
				L"SparkleClipboardTest_" +
				std::to_wstring(::GetCurrentProcessId()) +
				L"_" +
				std::to_wstring(::GetTickCount64());

			_station = ::CreateWindowStationW(
				stationName.c_str(),
				0,
				WINSTA_ALL_ACCESS,
				nullptr);
			if (_station == nullptr)
			{
				throw std::runtime_error("Unable to create private window station");
			}

			if (::SetProcessWindowStation(_station) == FALSE)
			{
				throw std::runtime_error("Unable to activate private window station");
			}

			_desktop = ::CreateDesktopW(
				L"Default",
				nullptr,
				nullptr,
				0,
				DESKTOP_ALL_ACCESS,
				nullptr);
			if (_desktop == nullptr)
			{
				throw std::runtime_error("Unable to create private clipboard desktop");
			}

			if (::SetThreadDesktop(_desktop) == FALSE)
			{
				throw std::runtime_error("Unable to activate private clipboard desktop");
			}
		}

		PrivateWindowStation(const PrivateWindowStation &) = delete;
		PrivateWindowStation &operator=(const PrivateWindowStation &) = delete;
	};
}

int main(int argc, char **argv)
{
	try
	{
		PrivateWindowStation windowStation;
		::testing::InitGoogleTest(&argc, argv);
		return RUN_ALL_TESTS();
	} catch (const std::exception &exception)
	{
		std::cerr << exception.what() << std::endl;
		return 2;
	}
}
