#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <ios>
#include <memory>
#include <mutex>
#include <ostream>
#include <source_location>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace spk
{
	class Logger final
	{
	public:
		enum class Level : std::uint8_t
		{
			Trace,
			Info,
			Warning,
			Error
		};

		struct LevelSetter
		{
			Level level;
		};

		class Output final
		{
			friend class Logger;

		private:
			Logger *_logger = nullptr;
			std::size_t _identifier = 0;

			Output(Logger *logger, std::size_t identifier) noexcept;
			void _release() noexcept;

		public:
			Output() noexcept = default;
			Output(const Output &) = delete;
			Output &operator=(const Output &) = delete;
			Output(Output &&other) noexcept;
			Output &operator=(Output &&other) noexcept;
			~Output();

			void setLevel(Level level) noexcept;
		};

	private:
		using OStreamManipulator = std::ostream &(*)(std::ostream &);
		using IOSManipulator = std::ios &(*)(std::ios &);
		using IOSBaseManipulator = std::ios_base &(*)(std::ios_base &);

		struct FileOutput;
		struct ThreadState
		{
			Level level = Level::Info;
			std::ostringstream stream;
		};

		std::mutex _mutex;
		std::vector<std::unique_ptr<FileOutput>> _outputs;
		bool _consoleMuted = false;
		std::size_t _nextOutputIdentifier = 1;

		static thread_local ThreadState _threadState;

		Logger();

		void _ensureOutputPathAvailable(const std::filesystem::path &path) const;
		[[nodiscard]] std::unique_ptr<FileOutput> _makeOutput(const std::filesystem::path &path, Level level);
		[[nodiscard]] bool _extractRecord(Level &level, std::string &message) noexcept;
		void _publishRecord(Level level, const std::string &message) noexcept;
		void _dispatch() noexcept;
		void _removeOutput(std::size_t identifier) noexcept;
		void _setOutputLevel(std::size_t identifier, Level level) noexcept;

	public:
		Logger(const Logger &) = delete;
		Logger &operator=(const Logger &) = delete;
		Logger(Logger &&) = delete;
		Logger &operator=(Logger &&) = delete;
		~Logger();

		[[nodiscard]] static Logger &instance() noexcept;
		[[nodiscard]] static LevelSetter setLevel(Level level) noexcept;

		[[nodiscard]] Output addOutput(const std::filesystem::path &path, Level lowerAcceptedLevel);
		void muteConsole() noexcept;
		void unmuteConsole() noexcept;

		Logger &operator<<(LevelSetter setter) noexcept;
		Logger &operator<<(std::source_location location) noexcept;
		Logger &operator<<(OStreamManipulator manipulator) noexcept;
		Logger &operator<<(IOSManipulator manipulator) noexcept;
		Logger &operator<<(IOSBaseManipulator manipulator) noexcept;

		template <typename TType>
		Logger &operator<<(TType &&value) noexcept
		{
			try
			{
				_threadState.stream << std::forward<TType>(value);
			} catch (...)
			{
			}
			return *this;
		}
	};

	inline Logger &logger = Logger::instance();
}

#define SPK_LOG(level) \
	(::spk::logger << ::spk::Logger::setLevel(::spk::Logger::Level::level) << std::source_location::current())
