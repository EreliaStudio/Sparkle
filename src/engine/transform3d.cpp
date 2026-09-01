#include "engine/transform3d.hpp"

#include "engine/entity3d.hpp"

#include <stdexcept>
#include <utility>

namespace spk
{
	Transform3D::Transform3D(const std::string &name) :
		System::Participant3D(name),
		_worldPosition(
			[this]() {
				const Transform3D *parentTransform = _parentTransform();

				if (parentTransform == nullptr)
				{
					return _position;
				}

				const Matrix4x4 &parentMatrix = parentTransform->modelMatrix();

				return Vector3{
					parentMatrix[0][0] * _position.x + parentMatrix[1][0] * _position.y + parentMatrix[2][0] * _position.z + parentMatrix[3][0],
					parentMatrix[0][1] * _position.x + parentMatrix[1][1] * _position.y + parentMatrix[2][1] * _position.z + parentMatrix[3][1],
					parentMatrix[0][2] * _position.x + parentMatrix[1][2] * _position.y + parentMatrix[2][2] * _position.z + parentMatrix[3][2]};
			}),
		_worldScale(
			[this]() {
				const Transform3D *parentTransform = _parentTransform();

				if (parentTransform == nullptr)
				{
					return _scale;
				}

				return parentTransform->scale(ReferenceFrame::World) * _scale;
			}),
		_worldRotation(
			[this]() {
				const Transform3D *parentTransform = _parentTransform();

				if (parentTransform == nullptr)
				{
					return _rotation;
				}

				return (parentTransform->rotation(ReferenceFrame::World) * _rotation).normalized();
			}),
		_localModelMatrix(
			[this]() {
				return Matrix4x4::translation(_position.x, _position.y, _position.z) * Matrix4x4::rotation(_rotation) * Matrix4x4::scale(_scale.x, _scale.y, _scale.z);
			}),
		_inverseLocalModelMatrix(
			[this]() {
				if (_scale.x == 0.0f || _scale.y == 0.0f || _scale.z == 0.0f)
				{
					throw std::runtime_error("Can't invert a Transform3D containing a zero-scaled axis");
				}

				return Matrix4x4::scale(1.0f / _scale.x, 1.0f / _scale.y, 1.0f / _scale.z) * Matrix4x4::rotation(_rotation.inversed()) * Matrix4x4::translation(-_position.x, -_position.y, -_position.z);
			}),
		_modelMatrix(
			[this]() {
				const Transform3D *parentTransform = _parentTransform();

				if (parentTransform == nullptr)
				{
					return localModelMatrix();
				}

				return parentTransform->modelMatrix() * localModelMatrix();
			}),
		_inverseModelMatrix(
			[this]() {
				const Transform3D *parentTransform = _parentTransform();

				if (parentTransform == nullptr)
				{
					return inverseLocalModelMatrix();
				}

				return inverseLocalModelMatrix() * parentTransform->inverseModelMatrix();
			})
	{
	}

	const Transform3D *Transform3D::_parentTransform() const
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

		const Entity3D *castedParent = dynamic_cast<const Entity3D *>(parent);

		if (castedParent != nullptr)
		{
			return &castedParent->transform();
		}

		return parent->getParticipant<Transform3D>();
	}

	void Transform3D::_clearWorldCaches()
	{
		_worldPosition.invalidate();
		_worldScale.invalidate();
		_worldRotation.invalidate();
		_modelMatrix.invalidate();
		_inverseModelMatrix.invalidate();
	}

	void Transform3D::_clearAllCaches()
	{
		_localModelMatrix.invalidate();
		_inverseLocalModelMatrix.invalidate();
		_clearWorldCaches();
	}

	void Transform3D::_clearWorldCachesRecursively(Entity &entity)
	{
		Transform3D *transform = entity.getParticipant<Transform3D>();

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

	void Transform3D::_clearDescendantWorldCaches()
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

	void Transform3D::_notifyEdition()
	{
		_clearAllCaches();
		_clearDescendantWorldCaches();
		_onEditionContractProvider.trigger(*this);
	}

	Transform3D::OnEditionContract Transform3D::subscribeToEdition(OnEditionCallback callback)
	{
		return _onEditionContractProvider.subscribe(std::move(callback));
	}

	const Vector3 &Transform3D::position(ReferenceFrame referenceFrame) const
	{
		switch (referenceFrame)
		{
		case ReferenceFrame::Local:
			return _position;
		case ReferenceFrame::World:
			return _worldPosition.get();
		}

		throw std::runtime_error("Invalid Transform3D reference frame");
	}

	const Vector3 &Transform3D::scale(ReferenceFrame referenceFrame) const
	{
		switch (referenceFrame)
		{
		case ReferenceFrame::Local:
			return _scale;
		case ReferenceFrame::World:
			return _worldScale.get();
		}

		throw std::runtime_error("Invalid Transform3D reference frame");
	}

	const Quaternion &Transform3D::rotation(ReferenceFrame referenceFrame) const
	{
		switch (referenceFrame)
		{
		case ReferenceFrame::Local:
			return _rotation;
		case ReferenceFrame::World:
			return _worldRotation.get();
		}

		throw std::runtime_error("Invalid Transform3D reference frame");
	}

	Vector3 Transform3D::eulerRotation(ReferenceFrame referenceFrame) const
	{
		return rotation(referenceFrame).toEuler();
	}

	void Transform3D::place(const Vector3 &position)
	{
		if (_position.x == position.x && _position.y == position.y && _position.z == position.z)
		{
			return;
		}

		_position = position;
		_notifyEdition();
	}

	void Transform3D::move(const Vector3 &delta)
	{
		place(_position + delta);
	}

	void Transform3D::rescale(const Vector3 &scale)
	{
		if (_scale.x == scale.x && _scale.y == scale.y && _scale.z == scale.z)
		{
			return;
		}

		_scale = scale;
		_notifyEdition();
	}

	void Transform3D::setRotation(const Quaternion &rotation)
	{
		const Quaternion normalizedRotation = rotation.normalized();

		if (_rotation == normalizedRotation)
		{
			return;
		}

		_rotation = normalizedRotation;
		_notifyEdition();
	}

	void Transform3D::setEulerRotation(const Vector3 &rotation)
	{
		setRotation(Quaternion::fromEuler(rotation));
	}

	void Transform3D::rotate(const Quaternion &rotation)
	{
		setRotation(_rotation * rotation);
	}

	void Transform3D::rotate(const Vector3 &rotation)
	{
		rotate(Quaternion::fromEuler(rotation));
	}

	void Transform3D::rotate(const Vector3 &axis, float angle)
	{
		rotate(Quaternion::fromAxisAngle(axis, angle));
	}

	const Matrix4x4 &Transform3D::localModelMatrix() const
	{
		return _localModelMatrix.get();
	}

	const Matrix4x4 &Transform3D::inverseLocalModelMatrix() const
	{
		return _inverseLocalModelMatrix.get();
	}

	const Matrix4x4 &Transform3D::modelMatrix() const
	{
		return _modelMatrix.get();
	}

	const Matrix4x4 &Transform3D::inverseModelMatrix() const
	{
		return _inverseModelMatrix.get();
	}
}
