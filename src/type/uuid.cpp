#include "type/uuid.hpp"

#include <array>
#include <cctype>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace spk
{
	namespace
	{
		[[nodiscard]] int hexadecimalValue(char character)
		{
			const unsigned char value = static_cast<unsigned char>(character);

			if (std::isdigit(value) != 0)
			{
				return character - '0';
			}
			if (character >= 'a' && character <= 'f')
			{
				return 10 + character - 'a';
			}
			if (character >= 'A' && character <= 'F')
			{
				return 10 + character - 'A';
			}

			return -1;
		}

		[[nodiscard]] bool isHyphenPosition(std::size_t index)
		{
			return index == 8 || index == 13 || index == 18 || index == 23;
		}
	}

	UUID UUID::generate()
	{
		static thread_local std::mt19937_64 generator(std::random_device{}());
		std::uniform_int_distribution<std::uint16_t> distribution(0, 255);

		Storage storage{};
		for (std::uint8_t &byte : storage)
		{
			byte = static_cast<std::uint8_t>(distribution(generator));
		}

		storage[6] = static_cast<std::uint8_t>((storage[6] & 0x0Fu) | 0x40u);
		storage[8] = static_cast<std::uint8_t>((storage[8] & 0x3Fu) | 0x80u);

		return UUID(storage);
	}

	UUID UUID::fromString(std::string_view text)
	{
		std::optional<UUID> parsed = tryParse(text);

		if (!parsed.has_value())
		{
			throw std::invalid_argument("Invalid UUID string");
		}

		return *parsed;
	}

	std::optional<UUID> UUID::tryParse(std::string_view text) noexcept
	{
		if (text.size() != 36u)
		{
			return std::nullopt;
		}

		Storage storage{};
		std::size_t byteIndex = 0;
		int highNibble = -1;

		for (std::size_t index = 0; index < text.size(); ++index)
		{
			if (isHyphenPosition(index))
			{
				if (text[index] != '-')
				{
					return std::nullopt;
				}
				continue;
			}

			const int value = hexadecimalValue(text[index]);
			if (value < 0)
			{
				return std::nullopt;
			}

			if (highNibble < 0)
			{
				highNibble = value;
			}
			else
			{
				if (byteIndex >= storage.size())
				{
					return std::nullopt;
				}

				storage[byteIndex++] = static_cast<std::uint8_t>((highNibble << 4) | value);
				highNibble = -1;
			}
		}

		if (byteIndex != storage.size() || highNibble >= 0)
		{
			return std::nullopt;
		}

		return UUID(storage);
	}

	std::string UUID::toString() const
	{
		std::ostringstream stream;
		stream << std::hex << std::setfill('0') << std::nouppercase;

		for (std::size_t index = 0; index < _bytes.size(); ++index)
		{
			if (index == 4 || index == 6 || index == 8 || index == 10)
			{
				stream << '-';
			}

			stream << std::setw(2) << static_cast<unsigned int>(_bytes[index]);
		}

		return stream.str();
	}
}
