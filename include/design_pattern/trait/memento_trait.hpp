#pragma once

#include "container/byte_stream.hpp"

#include <concepts>
#include <functional>
#include <utility>

namespace spk
{
	template <typename TObject>
	concept MementoSerializable = requires(
		const TObject &object,
		TObject &mutableObject,
		spk::ByteStream::Writer &writer,
		const spk::ByteStream::Reader &reader) {
		{ object.saveMemento(writer) } -> std::same_as<void>;
		{ mutableObject.loadMemento(reader) } -> std::same_as<void>;
	};

	template <typename TObject>
	class MementoTrait
	{
	public:
		~MementoTrait()
		{
			static_assert(MementoSerializable<TObject>, "MementoTrait requires saveMemento(Writer&) const and loadMemento(const Reader&).");
		}

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

		void loadSecure(const spk::ByteStream &snapshot)
		{
			transaction([&] {
				load(snapshot);
			});
		}

		template <typename TOperation>
			requires MementoSerializable<TObject>
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
