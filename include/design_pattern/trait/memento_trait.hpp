#pragma once

#include "container/byte_stream.hpp"

#include <functional>
#include <utility>

namespace spk
{
	template <typename TObject>
	class MementoTrait
	{
	public:
		[[nodiscard]] spk::ByteStream save() const
		{
			spk::ByteStream::Writer writer;
			static_cast<const TObject &>(*this).saveMemento(writer);
			return std::move(writer).build();
		}

		void load(const spk::ByteStream &snapshot)
		{
			auto reader = snapshot.reader();
			static_cast<TObject &>(*this).loadMemento(reader);
			if (reader.remaining() != 0)
			{
				throw spk::Exception("Memento snapshot contains trailing bytes.");
			}
		}

		template <typename TOperation>
		void transaction(TOperation &&operation)
		{
			const auto snapshot = save();
			try
			{
				std::invoke(std::forward<TOperation>(operation));
			} catch (...)
			{
				load(snapshot);
				throw;
			}
		}
	};
}
