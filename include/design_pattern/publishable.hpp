#pragma once

#include "container/byte_stream.hpp"
#include "container/flags.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace spk
{
	class Publishable
	{
	public:
		using PartID = std::uint32_t;

		enum class Mode
		{
			Source,
			Replicant
		};

		enum class Publication : std::uint8_t
		{
			Update = 1 << 0
		};

		enum class UpdateSelection
		{
			Modified,
			All
		};

		class Part
		{
			friend class Publishable;

		private:
			Publishable *_owner = nullptr;
			std::uint64_t _generation = 0;
			mutable std::optional<ByteStream> _cachedSnapshot;

			[[nodiscard]] std::pair<std::uint64_t, ByteStream> _snapshot() const;

		protected:
			Part() = default;
			void invalidate();

			virtual void _encode(ByteStream::Writer &writer) const = 0;
			virtual void _decode(const ByteStream::Reader &reader) = 0;

		public:
			virtual ~Part();

			Part(const Part &) = delete;
			Part &operator=(const Part &) = delete;
			Part(Part &&) = delete;
			Part &operator=(Part &&) = delete;
		};

		struct PartRegistration
		{
			PartID id;
			std::reference_wrapper<Part> part;
			Flags<Publication> flags;
		};

		using PartRegistry = std::vector<PartRegistration>;

	private:
		struct Registration;

		Mode _mode;
		std::map<PartID, std::shared_ptr<Registration>> _registrations;

		void _markModified(Part &part) noexcept;
		void _forget(Part &part) noexcept;

	protected:
		virtual bool _decodeLast(PartID id) const;
		void setPartRegistry(PartRegistry registrations);

	public:
		explicit Publishable(Mode mode);
		virtual ~Publishable();

		Publishable(const Publishable &) = delete;
		Publishable &operator=(const Publishable &) = delete;
		Publishable(Publishable &&) = delete;
		Publishable &operator=(Publishable &&) = delete;

		[[nodiscard]] Mode mode() const noexcept;
		[[nodiscard]] ByteStream encodeInitialization() const;
		[[nodiscard]] ByteStream encodeUpdate(UpdateSelection selection = UpdateSelection::Modified) const;
		void decode(const ByteStream::Reader &reader);
	};
}
