#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <source_location>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <sparkle_core>

namespace
{
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

	struct ThrowingValue
	{
	};

	std::ostream &operator<<(std::ostream &, const ThrowingValue &)
	{
		throw std::runtime_error("intentional stream failure");
	}
}

TEST(LoggerStreamTest, DefaultsToInfoAndResetsLevelAfterDispatch)
{
	ConsoleMute mute;
	const auto path = logPath("stream_level_reset");
	auto output = spk::logger.addOutput(path, spk::Logger::Level::Trace);

	spk::logger << spk::Logger::setLevel(spk::Logger::Level::Warning) << "first" << std::endl;
	spk::logger << "second" << std::endl;

	EXPECT_EQ(readFile(path), "[Warning] first\n[Info] second\n");
}

TEST(LoggerStreamTest, SupportsStreamableValuesAndStandardManipulators)
{
	ConsoleMute mute;
	const auto path = logPath("stream_values");
	auto output = spk::logger.addOutput(path, spk::Logger::Level::Trace);

	spk::logger << "value=" << std::hex << 255 << std::dec << ", bool=" << true << std::endl;

	EXPECT_EQ(readFile(path), "[Info] value=ff, bool=1\n");
}

TEST(LoggerStreamTest, SupportsSourceLocationAndMacroAtCallerSite)
{
	ConsoleMute mute;
	const auto path = logPath("stream_source_location");
	auto output = spk::logger.addOutput(path, spk::Logger::Level::Trace);

	const auto location = std::source_location::current();
	spk::logger << location << "manual" << std::endl;
	const int macroLine = __LINE__ + 1;
	SPK_LOG(Warning) << "macro" << std::endl;

	const std::string content = readFile(path);
	EXPECT_NE(content.find(std::string(location.file_name()) + ":" + std::to_string(location.line())), std::string::npos);
	EXPECT_NE(content.find("logger_stream_test.cpp:" + std::to_string(macroLine)), std::string::npos);
	EXPECT_NE(content.find("[Warning]"), std::string::npos);
}

TEST(LoggerStreamTest, ConcurrentRecordsNeverInterleave)
{
	ConsoleMute mute;
	const auto path = logPath("stream_concurrent");
	auto output = spk::logger.addOutput(path, spk::Logger::Level::Trace);
	constexpr int NbThread = 6;
	constexpr int NbRecord = 32;

	std::vector<std::jthread> threads;
	for (int thread = 0; thread < NbThread; ++thread)
	{
		threads.emplace_back([thread] {
			for (int record = 0; record < NbRecord; ++record)
			{
				spk::logger << "thread-" << thread << "-record-" << record << std::endl;
			}
		});
	}
	threads.clear();

	std::set<std::string> expected;
	for (int thread = 0; thread < NbThread; ++thread)
	{
		for (int record = 0; record < NbRecord; ++record)
		{
			expected.insert("[Info] thread-" + std::to_string(thread) + "-record-" + std::to_string(record));
		}
	}

	std::ifstream stream(path);
	std::string line;
	std::size_t count = 0;
	while (std::getline(stream, line))
	{
		EXPECT_EQ(expected.erase(line), 1u);
		++count;
	}
	EXPECT_EQ(count, static_cast<std::size_t>(NbThread * NbRecord));
	EXPECT_TRUE(expected.empty());
}

TEST(LoggerStreamTest, StreamInsertionFailureNeverEscapes)
{
	ConsoleMute mute;
	EXPECT_NO_THROW(
		spk::logger << "before" << ThrowingValue{} << "after" << std::endl);
}
