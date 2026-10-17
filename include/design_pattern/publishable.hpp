#pragma once

#include "container/byte_stream.hpp"
#include "container/cached_data.hpp"
#include "container/flags.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace spk
{
	class Publishable
	{
	private:
		struct Registration;
		struct State;

	public:
		using PartID = std::uint32_t;

		enum class Publication : std::uint8_t
		{
			Edition = 1 << 0,
			Update = 1 << 1
		};

		[[nodiscard]] friend constexpr Flags<Publication> operator|(Publication lhs, Publication rhs) noexcept
		{
			return Flags<Publication>(lhs) | Flags<Publication>(rhs);
		}

		enum class UnknownPartBehaviour
		{
			Throw,
			Skip
		};

		class Part
		{
			friend class Publishable;

		public:
			static constexpr Flags<Publication> InitializationOnlyFlag{};

		private:
			struct Snapshot
			{
				CachedData<ByteStream> edition;
				CachedData<ByteStream> update;
			};

			struct Subscriber
			{
				std::weak_ptr<State> state;
				std::weak_ptr<Registration> registration;
			};

			mutable std::mutex _cacheMutex;
			mutable Snapshot _snapshot;
			std::vector<Subscriber> _subscribers;

			[[nodiscard]] ByteStream _editionSnapshot() const;
			[[nodiscard]] ByteStream _updateSnapshot() const;
			void _subscribe(const std::shared_ptr<State> &state, const std::shared_ptr<Registration> &registration);
			void _unsubscribe(const std::shared_ptr<Registration> &registration) noexcept;
			[[nodiscard]] ByteStream _buildEdition() const;
			[[nodiscard]] ByteStream _buildUpdate() const;

		protected:
			Part();
			void invalidate();

			virtual void _encode(ByteStream::Writer &writer) const = 0;
			virtual void _decode(const ByteStream::Reader &reader) = 0;
			virtual void _encodeUpdate(ByteStream::Writer &writer) const;
			virtual void _decodeUpdate(const ByteStream::Reader &reader);

		public:
			virtual ~Part();

			Part(const Part &) = delete;
			Part &operator=(const Part &) = delete;
			Part(Part &&) = delete;
			Part &operator=(Part &&) = delete;
		};

	private:
		std::shared_ptr<State> _state;

	protected:
		void add(
			PartID id,
			Part *part,
			Flags<Publication> flags = Part::InitializationOnlyFlag);
		void remove(PartID id);
		[[nodiscard]] bool contains(PartID id) const;

	public:
		Publishable();
		virtual ~Publishable();

		Publishable(const Publishable &) = delete;
		Publishable &operator=(const Publishable &) = delete;
		Publishable(Publishable &&) = delete;
		Publishable &operator=(Publishable &&) = delete;

		[[nodiscard]] ByteStream encodeInitialization() const;
		[[nodiscard]] ByteStream encodeEdited() const;
		[[nodiscard]] ByteStream encodeUpdate() const;

		void decode(
			const ByteStream::Reader &reader,
			UnknownPartBehaviour behaviour = UnknownPartBehaviour::Throw);
	};
}
