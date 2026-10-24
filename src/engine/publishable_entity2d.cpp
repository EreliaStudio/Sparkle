#include "engine/publishable_entity2d.hpp"

namespace spk
{
	PublishableEntity2D::TransformPart::TransformPart(Transform2D &transform) :
		_transform(transform)
	{
		_edited = _transform.subscribeToEdition([this](const Transform2D &) {
			invalidate();
		});
	}

	void PublishableEntity2D::TransformPart::_encode(ByteStream::Writer &writer) const
	{
		const auto &position = _transform.position();
		const auto &scale = _transform.scale();
		writer << position.x << position.y << scale.x << scale.y << _transform.rotation() << _transform.depth();
	}

	void PublishableEntity2D::TransformPart::_decode(const ByteStream::Reader &reader)
	{
		float px, py, sx, sy, rotation, depth;
		reader >> px >> py >> sx >> sy >> rotation >> depth;
		_transform.place(Vector2{px, py});
		_transform.rescale(Vector2{sx, sy});
		_transform.setRotation(rotation);
		_transform.setDepth(depth);
	}

	PublishableEntity2D::PublishableEntity2D(Mode mode, const std::string &name, Entity *parent) :
		Entity2D(name, parent),
		PublishableEntity(mode),
		_activation(*this),
		_transformPart(transform())
	{
		_installParts();
	}

	PublishableEntity2D::PublishableEntity2D(Mode mode, const UUID &uuid, const std::string &name, Entity *parent) :
		Entity2D(name, parent),
		PublishableEntity(mode, uuid),
		_activation(*this),
		_transformPart(transform())
	{
		_installParts();
	}

	void PublishableEntity2D::_installParts()
	{
		registerInternalPart(0, _activation);
		registerInternalPart(1, _transformPart);
	}
}
