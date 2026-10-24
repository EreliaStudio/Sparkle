#include "engine/publishable_entity3d.hpp"

namespace spk
{
	PublishableEntity3D::TransformPart::TransformPart(Transform3D &transform) :
		_transform(transform)
	{
		_edited = _transform.subscribeToEdition([this](const Transform3D &) {
			invalidate();
		});
	}

	void PublishableEntity3D::TransformPart::_encode(ByteStream::Writer &writer) const
	{
		const auto &position = _transform.position();
		const auto &scale = _transform.scale();
		const auto &rotation = _transform.rotation();
		writer << position.x << position.y << position.z << scale.x << scale.y << scale.z;
		writer << rotation.x << rotation.y << rotation.z << rotation.w;
	}

	void PublishableEntity3D::TransformPart::_decode(const ByteStream::Reader &reader)
	{
		float px, py, pz, sx, sy, sz, rx, ry, rz, rw;
		reader >> px >> py >> pz >> sx >> sy >> sz >> rx >> ry >> rz >> rw;
		_transform.place(Vector3{px, py, pz});
		_transform.rescale(Vector3{sx, sy, sz});
		_transform.setRotation(Quaternion{rx, ry, rz, rw});
	}

	PublishableEntity3D::PublishableEntity3D(Mode mode, const std::string &name, Entity *parent) :
		Entity3D(name, parent),
		PublishableEntity(mode),
		_activation(*this),
		_transformPart(transform())
	{
		_installParts();
	}

	PublishableEntity3D::PublishableEntity3D(Mode mode, const UUID &uuid, const std::string &name, Entity *parent) :
		Entity3D(name, parent),
		PublishableEntity(mode, uuid),
		_activation(*this),
		_transformPart(transform())
	{
		_installParts();
	}

	void PublishableEntity3D::_installParts()
	{
		registerInternalPart(0, _activation);
		registerInternalPart(1, _transformPart);
	}
}
