#include "engine/message_serializable_trait.hpp"

#include "exception.hpp"

#include <cstdint>
#include <exception>
#include <limits>

namespace spk
{
	void MessageSerializableTrait::encode(spk::ByteStream::Writer &writer) const
	{
		const std::size_t lengthOffset = writer.size();
		writer << std::uint32_t{0};
		const std::size_t dataOffset = writer.size();
		_encode(writer);
		const std::size_t length = writer.size() - dataOffset;
		if (length > std::numeric_limits<std::uint32_t>::max())
		{
			throw spk::Exception("Serialized object exceeds maximum framed length.");
		}
		writer.edit(lengthOffset, static_cast<std::uint32_t>(length));
	}

	void MessageSerializableTrait::decode(const spk::ByteStream::Reader &reader)
	{
		const std::size_t initialOffset = reader.readOffset();
		std::uint32_t length = 0;
		try
		{
			reader >> length;
			if (length > reader.remaining())
			{
				throw spk::Exception("Serialized object exceeds remaining input.");
			}
		}
		catch (...)
		{
			reader.seek(initialOffset);
			throw;
		}

		auto section = reader.subreader(reader.readOffset(), reader.readOffset() + length);
		reader.skip(length);
		try
		{
			_beforeDecode();
			_decode(section);
			if (section.remaining() != 0)
			{
				throw spk::Exception("Serialized object contains trailing bytes.");
			}
		}
		catch (...)
		{
			try
			{
				_afterDecode(false);
			}
			catch (...)
			{
				throw spk::Exception("Serialized object recovery failed.", std::current_exception());
			}
			throw;
		}
		_afterDecode(true);
	}
}
