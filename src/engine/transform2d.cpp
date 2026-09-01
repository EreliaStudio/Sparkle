#include "engine/transform2d.hpp"

#include "engine/entity2d.hpp"

#include <stdexcept>
#include <utility>

namespace spk
{
	Transform2D::Transform2D(const std::string &name) :
		System::Participant2D(name),
		_worldPosition(
			[this]() {
				const Transform2D *parentTransform = _parentTransform();

				if (parentTransform == nullptr)
				{
					return _position;
				}

				const Matrix4x4 &parentMatrix = parentTransform->modelMatrix();

				return Vector2{
					parentMatrix[0][0] * _position.x + parentMatrix[1][0] * _position.y + parentMatrix[3][0],
					parentMatrix[0][1] * _position.x + parentMatrix[1][1] * _position.y + parentMatrix[3][1]};
			}),
		_worldScale(
			[this]() {
				const Transform2D *parentTransform = _parentTransform();

				if (parentTransform == nullptr)
				{
					return _scale;
				}

				return parentTransform->scale(ReferenceFrame::World) * _scale;
			}),
		_worldRotation(
			[this]() {
				const Transform2D *parentTransform = _parentTransform();

				if (parentTransform == nullptr)
				{
					return _rotation;
				}

				return parentTransform->rotation(ReferenceFrame::World) + _rotation;
			}),
		_worldDepth(
			[this]() {
				const Transform2D *parentTransform = _parentTransform();

				if (parentTransform == nullptr)
				{
					return _depth;
				}

				return parentTransform->depth(ReferenceFrame::World) + _depth;
			}),
		_localModelMatrix(
			[this]() {
				return Matrix4x4::translation(_position.x, _position.y, _depth) * Matrix4x4::rotation(0.0f, 0.0f, _rotation) * Matrix4x4::scale(_scale.x, _scale.y, 1.0f);
			}),
		_inverseLocalModelMatrix(
			[this]() {
				if (_scale.x == 0.0f || _scale.y == 0.0f)
				{
					throw std::runtime_error("Can't invert a Transform2D containing a zero-scaled axis");
				}

				return Matrix4x4::scale(1.0f / _scale.x, 1.0f / _scale.y, 1.0f) * Matrix4x4::rotation(0.0f, 0.0f, -_rotation) * Matrix4x4::translation(-_position.x, -_position.y, -_depth);
			}),
		_modelMatrix(
			[this]() {
				const Transform2D *parentTransform = _parentTransform();

				if (parentTransform == nullptr)
				{
					return localModelMatrix();
				}

				return parentTransform->modelMatrix() * localModelMatrix();
			}),
		_inverseModelMatrix(
			[this]() {
				const Transform2D *parentTransform = _parentTransform();

				if (parentTransform == nullptr)
				{
					return inverseLocalModelMatrix();
				}

				return inverseLocalModelMatrix() * parentTransform->inverseModelMatrix();
			})
	{
	}

	const Transform2D *Transform2D::_parentTransform() const
	{
		const Entity *currentOwner = System::Participant::owner();

		if (currentOwner == nullptr)
		{
			return nullptr;
		}

		const Entity *parent = currentOwner->parent();

		if (parent == nullptr)
		{
			return nullptr;
		}

		const Entity2D *castedParent = dynamic_cast<const Entity2D *>(parent);

		if (castedParent != nullptr)
		{
			return &castedParent->transform();
		}

		return parent->getParticipant<Transform2D>();
	}

	void Transform2D::_clearWorldCaches()
	{
		_worldPosition.invalidate();
		_worldScale.invalidate();
		_worldRotation.invalidate();
		_worldDepth.invalidate();
		_modelMatrix.invalidate();
		_inverseModelMatrix.invalidate();
	}

	void Transform2D::_clearAllCaches()
	{
		_localModelMatrix.invalidate();
		_inverseLocalModelMatrix.invalidate();
		_clearWorldCaches();
	}

	void Transform2D::_clearWorldCachesRecursively(Entity &entity)
	{
		Transform2D *transform = entity.getParticipant<Transform2D>();

		if (transform != nullptr)
		{
			transform->_clearWorldCaches();
		}

		for (Entity *child : entity.children())
		{
			if (child != nullptr)
			{
				_clearWorldCachesRecursively(*child);
			}
		}
	}

	void Transform2D::_clearDescendantWorldCaches()
	{
		Entity *currentOwner = System::Participant::owner();

		if (currentOwner == nullptr)
		{
			return;
		}

		for (Entity *child : currentOwner->children())
		{
			if (child != nullptr)
			{
				_clearWorldCachesRecursively(*child);
			}
		}
	}

	void Transform2D::_notifyEdition()
	{
		_clearAllCaches();
		_clearDescendantWorldCaches();
		_onEditionContractProvider.trigger(*this);
	}

	Transform2D::OnEditionContract Transform2D::subscribeToEdition(OnEditionCallback callback)
	{
		return _onEditionContractProvider.subscribe(std::move(callback));
	}

	const Vector2 &Transform2D::position(ReferenceFrame referenceFrame) const
	{
		switch (referenceFrame)
		{
		case ReferenceFrame::Local:
			return _position;
		case ReferenceFrame::World:
			return _worldPosition.get();
		}

		throw std::runtime_error("Invalid Transform2D reference frame");
	}

	const Vector2 &Transform2D::scale(ReferenceFrame referenceFrame) const
	{
		switch (referenceFrame)
		{
		case ReferenceFrame::Local:
			return _scale;
		case ReferenceFrame::World:
			return _worldScale.get();
		}

		throw std::runtime_error("Invalid Transform2D reference frame");
	}

	float Transform2D::rotation(ReferenceFrame referenceFrame) const
	{
		switch (referenceFrame)
		{
		case ReferenceFrame::Local:
			return _rotation;
		case ReferenceFrame::World:
			return _worldRotation.get();
		}

		throw std::runtime_error("Invalid Transform2D reference frame");
	}

	float Transform2D::depth(ReferenceFrame referenceFrame) const
	{
		switch (referenceFrame)
		{
		case ReferenceFrame::Local:
			return _depth;
		case ReferenceFrame::World:
			return _worldDepth.get();
		}

		throw std::runtime_error("Invalid Transform2D reference frame");
	}

	void Transform2D::place(const Vector2 &position)
	{
		if (_position == position)
		{
			return;
		}

		_position = position;
		_notifyEdition();
	}

	void Transform2D::move(const Vector2 &delta)
	{
		place(_position + delta);
	}

	void Transform2D::rescale(const Vector2 &scale)
	{
		if (_scale == scale)
		{
			return;
		}

		_scale = scale;
		_notifyEdition();
	}

	void Transform2D::setRotation(float rotation)
	{
		if (_rotation == rotation)
		{
			return;
		}

		_rotation = rotation;
		_notifyEdition();
	}

	void Transform2D::rotate(float delta)
	{
		setRotation(_rotation + delta);
	}

	void Transform2D::setDepth(float depth)
	{
		if (_depth == depth)
		{
			return;
		}

		_depth = depth;
		_notifyEdition();
	}

	void Transform2D::elevate(float delta)
	{
		setDepth(_depth + delta);
	}

	const Matrix4x4 &Transform2D::localModelMatrix() const
	{
		return _localModelMatrix.get();
	}

	const Matrix4x4 &Transform2D::inverseLocalModelMatrix() const
	{
		return _inverseLocalModelMatrix.get();
	}

	const Matrix4x4 &Transform2D::modelMatrix() const
	{
		return _modelMatrix.get();
	}

	const Matrix4x4 &Transform2D::inverseModelMatrix() const
	{
		return _inverseModelMatrix.get();
	}
}
