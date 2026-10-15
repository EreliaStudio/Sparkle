#pragma once

#include "container/byte_stream.hpp"

namespace spk
{
	class MessageSerializableTrait
	{
	protected:
		virtual void _encode(spk::ByteStream::Writer &writer) const = 0;
		virtual void _decode(const spk::ByteStream::Reader &reader) = 0;
		virtual void _beforeDecode() {}
		virtual void _afterDecode(bool success) {}

	public:
		virtual ~MessageSerializableTrait() = default;

		void encode(spk::ByteStream::Writer &writer) const;
		void decode(const spk::ByteStream::Reader &reader);
	};
}
