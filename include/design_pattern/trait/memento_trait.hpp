#pragma once

#include "container/byte_stream.hpp"

#include <functional>
#include <utility>

namespace spk
{
	class MementoTrait
	{
	private:
		virtual void _saveMemento(spk::ByteStream::Writer &writer) const = 0;
		virtual void _loadMemento(const spk::ByteStream::Slice &reader) = 0;

	public:
		virtual ~MementoTrait() = default;

		[[nodiscard]] spk::ByteStream save() const
		{
			spk::ByteStream::Writer writer;
			_saveMemento(writer);
			return std::move(writer).build();
		}

		void load(const spk::ByteStream &snapshot)
		{
			auto reader = snapshot.reader();
			_loadMemento(reader);
			if (reader.remaining() != 0)
			{
				throw spk::Exception("Memento snapshot contains trailing bytes.");
			}
		}

		void loadSecure(const spk::ByteStream &snapshot)
		{
			const auto previous = save();
			try
			{
				load(snapshot);
			} catch (...)
			{
				try
				{
					load(previous);
				} catch (...)
				{
					throw spk::Exception("Memento restoration and rollback both failed.");
				}
				throw;
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
