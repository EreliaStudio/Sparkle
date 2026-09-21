#include "diagnostics/logger.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#include "exception.hpp"

namespace
{
	[[nodiscard]] const char *levelName(spk::Logger::Level level) noexcept
	{
		switch (level)
		{
		case spk::Logger::Level::Trace: return "Trace";
		case spk::Logger::Level::Info: return "Info";
		case spk::Logger::Level::Warning: return "Warning";
		case spk::Logger::Level::Error: return "Error";
		}
		return "Unknown";
	}

	void writeRecord(std::ostream &stream, spk::Logger::Level level, const std::string &message) noexcept
	{
		try
		{
			stream << '[' << levelName(level) << "] " << message << '\n';
			stream.flush();
		}
		catch (...)
		{
		}
	}
}

namespace spk
{
	struct Logger::FileOutput
	{
		std::size_t identifier = 0;
		std::filesystem::path path;
		Level lowerAcceptedLevel = Level::Trace;
		std::ofstream stream;
	};

	Logger::Output::Output(Logger *logger, std::size_t identifier) noexcept :
		_logger(logger),
		_identifier(identifier)
	{
	}

	Logger::Output::Output(Output &&other) noexcept :
		_logger(std::exchange(other._logger, nullptr)),
		_identifier(std::exchange(other._identifier, 0))
	{
	}

	Logger::Output &Logger::Output::operator=(Output &&other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}
		_release();
		_logger = std::exchange(other._logger, nullptr);
		_identifier = std::exchange(other._identifier, 0);
		return *this;
	}

	Logger::Output::~Output()
	{
		_release();
	}

	void Logger::Output::_release() noexcept
	{
		if (_logger == nullptr)
		{
			return;
		}
		_logger->_removeOutput(_identifier);
		_logger = nullptr;
		_identifier = 0;
	}

	void Logger::Output::setLevel(Level level) noexcept
	{
		if (_logger != nullptr)
		{
			_logger->_setOutputLevel(_identifier, level);
		}
	}

	Logger::Logger() = default;

	Logger::~Logger() = default;

	Logger &Logger::instance() noexcept
	{
		static Logger instance;
		return instance;
	}

	Logger::LevelSetter Logger::setLevel(Level level) noexcept
	{
		return LevelSetter{level};
	}

	Logger::Output Logger::addOutput(const std::filesystem::path &path, Level lowerAcceptedLevel)
	{
		try
		{
			const std::filesystem::path normalizedPath = path.lexically_normal();
			const std::scoped_lock lock(_mutex);

			const auto duplicate = std::find_if(_outputs.begin(), _outputs.end(), [&](const auto &output) {
				return output->path == normalizedPath;
			});
			if (duplicate != _outputs.end())
			{
				throw spk::Exception("Logger output already registered [" + normalizedPath.string() + "]");
			}

			auto output = std::make_unique<FileOutput>();
			output->identifier = _nextOutputIdentifier++;
			output->path = normalizedPath;
			output->lowerAcceptedLevel = lowerAcceptedLevel;
			output->stream.open(normalizedPath, std::ios::out | std::ios::app);
			if (!output->stream.is_open())
			{
				throw spk::Exception("Failed to open logger output [" + normalizedPath.string() + "]");
			}

			const std::size_t identifier = output->identifier;
			_outputs.push_back(std::move(output));
			return Output(this, identifier);
		}
		catch (const spk::Exception &)
		{
			throw;
		}
		catch (...)
		{
			throw spk::Exception("Failed to add logger output", std::current_exception());
		}
	}

	void Logger::muteConsole() noexcept
	{
		try
		{
			const std::scoped_lock lock(_mutex);
			_consoleMuted = true;
		}
		catch (...)
		{
		}
	}

	void Logger::unmuteConsole() noexcept
	{
		try
		{
			const std::scoped_lock lock(_mutex);
			_consoleMuted = false;
		}
		catch (...)
		{
		}
	}

	Logger &Logger::operator<<(LevelSetter setter) noexcept
	{
		_threadState.level = setter.level;
		return *this;
	}

	Logger &Logger::operator<<(const std::source_location &location) noexcept
	{
		try
		{
			_threadState.stream << location.file_name() << ':' << location.line() << ' ';
		}
		catch (...)
		{
		}
		return *this;
	}

	Logger &Logger::operator<<(OStreamManipulator manipulator) noexcept
	{
		const OStreamManipulator endLine =
			static_cast<OStreamManipulator>(std::endl<char, std::char_traits<char>>);
		if (manipulator == endLine)
		{
			_dispatch();
			return *this;
		}
		try
		{
			manipulator(_threadState.stream);
		}
		catch (...)
		{
		}
		return *this;
	}

	Logger &Logger::operator<<(IOSManipulator manipulator) noexcept
	{
		try
		{
			manipulator(_threadState.stream);
		}
		catch (...)
		{
		}
		return *this;
	}

	Logger &Logger::operator<<(IOSBaseManipulator manipulator) noexcept
	{
		try
		{
			manipulator(_threadState.stream);
		}
		catch (...)
		{
		}
		return *this;
	}

	void Logger::_dispatch() noexcept
	{
		const Level level = _threadState.level;
		std::string message;
		try
		{
			message = _threadState.stream.str();
			_threadState.stream.str({});
			_threadState.stream.clear();
			_threadState.level = Level::Info;
		}
		catch (...)
		{
			return;
		}

		try
		{
			const std::scoped_lock lock(_mutex);
			if (!_consoleMuted)
			{
				writeRecord(std::clog, level, message);
			}
			for (auto &output : _outputs)
			{
				if (static_cast<std::uint8_t>(level) < static_cast<std::uint8_t>(output->lowerAcceptedLevel))
				{
					continue;
				}
				writeRecord(output->stream, level, message);
			}
		}
		catch (...)
		{
		}
	}

	void Logger::_removeOutput(std::size_t identifier) noexcept
	{
		try
		{
			const std::scoped_lock lock(_mutex);
			std::erase_if(_outputs, [&](const auto &output) {
				return output->identifier == identifier;
			});
		}
		catch (...)
		{
		}
	}

	void Logger::_setOutputLevel(std::size_t identifier, Level level) noexcept
	{
		try
		{
			const std::scoped_lock lock(_mutex);
			const auto it = std::find_if(_outputs.begin(), _outputs.end(), [&](const auto &output) {
				return output->identifier == identifier;
			});
			if (it != _outputs.end())
			{
				(*it)->lowerAcceptedLevel = level;
			}
		}
		catch (...)
		{
		}
	}
}
