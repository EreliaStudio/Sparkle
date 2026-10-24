#pragma once

#include "engine/entity3d.hpp"
#include "engine/publishable_entity.hpp"
#include "engine/transform3d.hpp"

#include <string>

namespace spk
{
	class PublishableEntity3D : public Entity3D, public PublishableEntity
	{
	private:
		class TransformPart : public Part
		{
		private:
			Transform3D &_transform;
			Transform3D::OnEditionContract _edited;

			void _encode(ByteStream::Writer &writer) const override;
			void _decode(const ByteStream::Reader &reader) override;

		public:
			explicit TransformPart(Transform3D &transform);
		};

		ActivationPart _activation;
		TransformPart _transformPart;

		void _installParts();

	public:
		PublishableEntity3D(Mode mode, const std::string &name, Entity *parent = nullptr);
		PublishableEntity3D(Mode mode, const UUID &uuid, const std::string &name, Entity *parent = nullptr);
	};
}
