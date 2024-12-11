#include "structure/engine/spk_transform.hpp"
#include "structure/engine/spk_entity.hpp"
#include "structure/math/spk_quaternion.hpp"

namespace spk
{
	Transform::Transform() : 
		Component(L"Transform"),
		_model(),
		_position(0.0f, 0.0f, 0.0f),
		_rotation(spk::Quaternion::identity()),
		_scale(1.0f, 1.0f, 1.0f),
		_velocity(0.0f, 0.0f, 0.0f),
		_forward(0.0f, 0.0f, 1.0f),
		_right(1.0f, 0.0f, 0.0f),
		_up(0.0f, 1.0f, 0.0f)
	{
		_updateModel();
	}

	ContractProvider::Contract Transform::addOnEditionCallback(const std::function<void()>& p_callback)
	{
		return (std::move(_onEditContractProvider.subscribe(p_callback)));
	}

	const spk::Matrix4x4 &Transform::model() const
	{
		return (_model);
	}

	const spk::Matrix4x4 &Transform::inverseModel() const
	{
		return (_inverseModel);
	}

	const spk::Vector3 &Transform::position() const
	{
		return (_position);
	}

	spk::Vector3 Transform::rotation() const
	{
		return _rotation.toEuler();
	}

	const spk::Quaternion& Transform::rotationQuaternion() const
	{
		return (_rotation);
	}

	const spk::Vector3 &Transform::scale() const
	{
		return (_scale);
	}

	const spk::Vector3 &Transform::forward() const
	{
		return (_forward);
	}

	const spk::Vector3 &Transform::right() const
	{
		return (_right);
	}

	const spk::Vector3 &Transform::up() const
	{
		return (_up);
	}

	void Transform::lookAt(const spk::Vector3 &target)
	{
		_rotation = spk::Quaternion::lookAt(_position, target, spk::Vector3(0, 1, 0));
		_updateModel();
	}

	void Transform::setVelocity(const spk::Vector3 &p_velocity) // unit/second
	{
		_velocity = p_velocity / 1000.0f;
	}

	void Transform::addVelocity(const spk::Vector3 &p_delta) // unit/second
	{
		_velocity += p_delta / 1000.0f;
	}

	void Transform::move(const spk::Vector3 &p_delta)
	{
		_position += p_delta;
		_updateModel();
	}

	void Transform::place(const spk::Vector3 &p_newPosition)
	{
		_position = p_newPosition;
		_updateModel();
	}

	void Transform::rotate(const spk::Vector3 &p_deltaEuler)
	{
		spk::Quaternion deltaQ = spk::Quaternion::fromEuler(p_deltaEuler);

		_rotation = (_rotation * deltaQ).normalize();

		_updateModel();
	}

	void Transform::setRotation(const spk::Vector3 &p_euler)
	{
		_rotation = spk::Quaternion::fromEuler(p_euler).normalize();
		
		_updateModel();
	}
	
	void Transform::rotateAroundPoint(const spk::Vector3& center, const spk::Vector3& axis, float angle)
	{
		spk::Vector3 relativePosition = _position - center;

		spk::Quaternion rotationQuat = spk::Quaternion::fromAxisAngle(axis, angle);

		spk::Vector3 rotatedPosition = rotationQuat.rotate(relativePosition);

		_position = rotatedPosition + center;

		_updateModel();
	}

	void Transform::setScale(const spk::Vector3 &p_scale)
	{
		_scale = p_scale;
		_updateModel();
	}

	void Transform::update(const long long &p_duration)
	{
		if (_velocity != spk::Vector3())
		{
			_position += _velocity * (float)p_duration;
			_updateModel();
		}
	}

	void Transform::_updateModel()
	{
		spk::Matrix4x4 translationMatrix = spk::Matrix4x4::translationMatrix(_position);
		spk::Matrix4x4 scaleMatrix = spk::Matrix4x4::scaleMatrix(_scale);
		spk::Matrix4x4 rotationMatrix = spk::Matrix4x4::rotationMatrix(_rotation);

		spk::Matrix4x4 parentModel = spk::Matrix4x4::identity();
		spk::SafePointer<Entity> entityOwner = owner();
		
		if (entityOwner != nullptr && entityOwner->parent() != nullptr)
		{
			const Entity *parentEntity = static_cast<const Entity *>(entityOwner->parent());
			const Transform &parentTransform = parentEntity->transform();
			parentModel = parentTransform.model();
		}

		_model = parentModel * translationMatrix * rotationMatrix * scaleMatrix;
		_inverseModel = _model.inverse();

		_forward = _rotation * spk::Vector3(0.0f, 0.0f, 1.0f).normalize();
		float forwardToUpDot = _forward.dot(spk::Vector3(0.0f, 1.0f, 0.0f));
		_right = Vector3(1, 0, 0);

		if (forwardToUpDot > -1 && forwardToUpDot < 1)
		{
			_right = _forward.cross(spk::Vector3(0.0f, 1.0f, 0.0f)).normalize();
		}

		_up = _right.cross(_forward);

		_onEditContractProvider.trigger();
	}
}
