#pragma once

#include "container/byte_stream.hpp"
#include "exception.hpp"

#include <functional>
#include <utility>

namespace spk
{
	class MementoTrait
	{
	protected:
		virtual void saveMemento(spk::ByteStream::Writer &writer) const = 0;
		virtual void loadMemento(const spk::ByteStream::Reader &reader) = 0;

	public:
		virtual ~MementoTrait() = default;

		[[nodiscard]] spk::ByteStream save() const
		{
			spk::ByteStream::Writer writer;
			saveMemento(writer);
			return std::move(writer).build();
		}

		void load(const spk::ByteStream &snapshot)
		{
			auto reader = snapshot.reader();
			loadMemento(reader);
			if (reader.remaining() != 0)
			{
				throw spk::Exception("Memento snapshot contains trailing bytes.");
			}
		}

		void loadSecure(const spk::ByteStream &snapshot)
		{
			transaction([&] {
				load(snapshot);
			});
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
				try
				{
					load(snapshot);
				} catch (...)
				{
					throw spk::Exception("Memento transaction rollback failed.");
				}
				throw;
			}
		}
	};
}
