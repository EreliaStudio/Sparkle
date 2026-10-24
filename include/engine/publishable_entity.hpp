#pragma once

#include "design_pattern/publishable.hpp"
#include "engine/entity.hpp"
#include "type/uuid.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace spk
{
	class PublishableEntity : public Publishable
	{
	public:
		using EntityTypeID = std::uint32_t;
		static constexpr PartID FirstGamePartID = 16;

	protected:
		class ActivationPart : public Part
		{
		private:
			Entity &_entity;
			ActivableTrait::ActivationContract _activated;
			ActivableTrait::DeactivationContract _deactivated;

			void _encode(ByteStream::Writer &writer) const override;
			void _decode(const ByteStream::Reader &reader) override;

		public:
			explicit ActivationPart(Entity &entity);
		};

	private:
		UUID _uuid;
		PartRegistry _internalParts;
		PartRegistry _gameParts;

		void _install();

	protected:
		explicit PublishableEntity(Mode mode);
		PublishableEntity(Mode mode, const UUID &uuid);

		void registerInternalPart(PartID id, Part &part, Flags<Publication> flags = Publication::Update);
		void setPartRegistry(PartRegistry registry);

		[[nodiscard]] bool _decodeLast(PartID id) const override;

	public:
		[[nodiscard]] const UUID &uuid() const noexcept;
	};
}
