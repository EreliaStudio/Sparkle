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
			_station = ::CreateWindowStationW(
				nullptr,
				CWF_CREATE_ONLY,
				WINSTA_ALL_ACCESS,
				nullptr);
			if (_station == nullptr && ::GetLastError() == ERROR_ALREADY_EXISTS)
			{
				_station = ::CreateWindowStationW(
					nullptr,
					0,
					WINSTA_ALL_ACCESS,
					nullptr);
			}
			if (_station == nullptr)
			{
				throw std::runtime_error("Unable to create private window station");
			}

			if (::SetProcessWindowStation(_station) == FALSE)
			{
				throw std::runtime_error("Unable to activate private window station");
			}

			const std::wstring desktopName =
				L"SparkleClipboardTest_" +
				std::to_wstring(::GetCurrentProcessId());

			_desktop = ::CreateDesktopW(
				desktopName.c_str(),
				nullptr,
				nullptr,
				0,
				GENERIC_ALL,
				nullptr);
			if (_desktop == nullptr)
			{
				throw std::runtime_error("Unable to create private clipboard desktop");
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
