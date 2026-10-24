#pragma once

#include "engine/entity2d.hpp"
#include "engine/publishable_entity.hpp"
#include "engine/transform2d.hpp"

#include <string>

namespace spk
{
	class PublishableEntity2D : public Entity2D, public PublishableEntity
	{
	private:
		class TransformPart : public Part
		{
		private:
			Transform2D &_transform;
			Transform2D::OnEditionContract _edited;

			void _encode(ByteStream::Writer &writer) const override;
			void _decode(const ByteStream::Reader &reader) override;

		public:
			explicit TransformPart(Transform2D &transform);
		};

		ActivationPart _activation;
		TransformPart _transformPart;

		void _installParts();

	public:
		PublishableEntity2D(Mode mode, const std::string &name, Entity *parent = nullptr);
		PublishableEntity2D(Mode mode, const UUID &uuid, const std::string &name, Entity *parent = nullptr);
	};
}
