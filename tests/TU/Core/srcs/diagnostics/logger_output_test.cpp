#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

#include <sparkle_core>

namespace
{
	[[nodiscard]] std::filesystem::path logPath(const std::string &name)
	{
		const auto root = std::filesystem::path(SPARKLE_TEST_RESULTS_DIR) / "logger";
		std::filesystem::create_directories(root);
		const auto path = root / (name + ".log");
		std::filesystem::remove(path);
		return path;
	}

	[[nodiscard]] std::string readFile(const std::filesystem::path &path)
	{
		std::ifstream stream(path);
		return std::string(
			std::istreambuf_iterator<char>(stream),
			std::istreambuf_iterator<char>());
	}

	class ConsoleMute final
	{
	public:
		ConsoleMute()
		{
			spk::logger.muteConsole();
		}

		~ConsoleMute()
		{
			spk::logger.unmuteConsole();
		}
	};

	class ClogCapture final
	{
	private:
		std::ostringstream _capture;
		std::streambuf *_previous = nullptr;

	public:
		ClogCapture() :
			_previous(std::clog.rdbuf(_capture.rdbuf()))
		{
		}

		~ClogCapture()
		{
			std::clog.rdbuf(_previous);
		}

		[[nodiscard]] std::string content() const
		{
			return _capture.str();
		}
	};

	class FailingBuffer final : public std::streambuf
	{
	protected:
		int_type overflow(int_type) override
		{
			return traits_type::eof();
		}
	};

	class FailingClog final
	{
	private:
		FailingBuffer _buffer;
		std::streambuf *_previous = nullptr;
		std::ios::iostate _exceptions = std::ios::goodbit;

	public:
		FailingClog()
		{
			_exceptions = std::clog.exceptions();
			std::clog.exceptions(std::ios::goodbit);
			_previous = std::clog.rdbuf(&_buffer);
			std::clog.clear();
			std::clog.exceptions(std::ios::badbit | std::ios::failbit);
		}

		~FailingClog()
		{
			try
			{
				std::clog.exceptions(std::ios::goodbit);
				std::clog.clear();
				std::clog.rdbuf(_previous);
				std::clog.clear();
				std::clog.exceptions(_exceptions);
			} catch (...)
			{
			}
		}
	};
}

TEST(LoggerOutputTest, ConsoleAlwaysReceivesEveryLevelUnlessMuted)
{
	ClogCapture capture;

	spk::logger << spk::Logger::setLevel(spk::Logger::Level::Trace) << "trace" << std::endl;
	spk::logger.muteConsole();
	spk::logger << spk::Logger::setLevel(spk::Logger::Level::Error) << "hidden" << std::endl;
	spk::logger.unmuteConsole();
	spk::logger << spk::Logger::setLevel(spk::Logger::Level::Warning) << "warning" << std::endl;

	EXPECT_EQ(capture.content(), "[Trace] trace\n[Warning] warning\n");
}

TEST(LoggerOutputTest, FileOutputAppendsAndFiltersBelowThreshold)
{
	ConsoleMute mute;
	const auto path = logPath("output_append_filter");
	{
		std::ofstream seed(path);
		seed << "existing\n";
	}
	auto output = spk::logger.addOutput(path, spk::Logger::Level::Warning);

	spk::logger << "ignored" << std::endl;
	spk::logger << spk::Logger::setLevel(spk::Logger::Level::Warning) << "warning" << std::endl;
	spk::logger << spk::Logger::setLevel(spk::Logger::Level::Error) << "error" << std::endl;

	EXPECT_EQ(readFile(path), "existing\n[Warning] warning\n[Error] error\n");
}

TEST(LoggerOutputTest, SeveralOutputsAndDynamicLevelAreIndependent)
{
	ConsoleMute mute;
	const auto broadPath = logPath("output_broad");
	const auto strictPath = logPath("output_strict");
	auto broad = spk::logger.addOutput(broadPath, spk::Logger::Level::Trace);
	auto strict = spk::logger.addOutput(strictPath, spk::Logger::Level::Error);

	spk::logger << "info" << std::endl;
	spk::logger << spk::Logger::setLevel(spk::Logger::Level::Warning) << "warning-a" << std::endl;
	strict.setLevel(spk::Logger::Level::Warning);
	broad.setLevel(spk::Logger::Level::Warning);
	spk::logger << "ignored-info" << std::endl;
	spk::logger << spk::Logger::setLevel(spk::Logger::Level::Warning) << "warning-b" << std::endl;

	EXPECT_EQ(readFile(broadPath), "[Info] info\n[Warning] warning-a\n[Warning] warning-b\n");
	EXPECT_EQ(readFile(strictPath), "[Warning] warning-b\n");
}

TEST(LoggerOutputTest, OutputLifetimeUnregistersFile)
{
	ConsoleMute mute;
	const auto path = logPath("output_raii");
	{
		auto output = spk::logger.addOutput(path, spk::Logger::Level::Trace);
		spk::logger << "inside" << std::endl;
	}
	spk::logger << "outside" << std::endl;

	EXPECT_EQ(readFile(path), "[Info] inside\n");
}

TEST(LoggerOutputTest, DuplicatePathRegistrationThrowsSparkleException)
{
	ConsoleMute mute;
	const auto path = logPath("output_duplicate");
	auto output = spk::logger.addOutput(path, spk::Logger::Level::Info);

	EXPECT_THROW(
		(void)spk::logger.addOutput(path, spk::Logger::Level::Warning),
		spk::Exception);
}

TEST(LoggerOutputTest, FileOpenFailureThrowsSparkleException)
{
	ConsoleMute mute;
	const auto root = std::filesystem::path(SPARKLE_TEST_RESULTS_DIR) / "logger" / "missing-parent";
	std::filesystem::remove_all(root);
	const auto path = root / "nested" / "output.log";

	EXPECT_THROW(
		(void)spk::logger.addOutput(path, spk::Logger::Level::Info),
		spk::Exception);
}

TEST(LoggerOutputTest, FailingConsoleNeverEscapesOrBlocksFileOutput)
{
	const auto path = logPath("output_console_failure");
	auto output = spk::logger.addOutput(path, spk::Logger::Level::Trace);
	{
		FailingClog failingConsole;
		EXPECT_NO_THROW(spk::logger << "survives" << std::endl);
	}

	EXPECT_EQ(readFile(path), "[Info] survives\n");
}
