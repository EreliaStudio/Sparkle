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
		_forward(0.0f, 0.0f, -1.0f),
		_right(1.0f, 0.0f, 0.0f),
		_up(0.0f, 1.0f, 0.0f)
	{
		_updateModel();
	}

	ContractProvider::Contract Transform::addOnEditionCallback(const std::function<void()> &p_callback)
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

	const spk::Vector3 &Transform::localPosition() const
	{
		return (_localPosition);
	}

	spk::Vector3 Transform::rotation() const
	{
		return (_rotation.toEuler());
	}

	const spk::Quaternion &Transform::rotationQuaternion() const
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

	void Transform::lookAt(const spk::Vector3 &p_target)
	{
		spk::Quaternion lookRotation = spk::Quaternion::lookAt(_position, p_target, spk::Vector3(0.0f, 1.0f, 0.0f));
		_rotation = spk::Quaternion(-lookRotation.x, -lookRotation.y, -lookRotation.z, lookRotation.w);
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
		_localPosition += p_delta;
		_updateModel();
	}

	void Transform::place(const spk::Vector3 &p_newPosition)
	{
		_localPosition = p_newPosition;
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

	void Transform::rotateAroundPoint(const spk::Vector3 &p_center, const spk::Vector3 &p_axis, float p_angle)
	{
		spk::Vector3 relativePosition = _position - p_center;

		spk::Quaternion rotationQuat = spk::Quaternion::fromAxisAngle(p_axis, p_angle);

		spk::Vector3 rotatedPosition = rotationQuat.rotate(relativePosition);

		_localPosition = rotatedPosition + p_center;

		_updateModel();
	}

	void Transform::setScale(const spk::Vector3 &p_scale)
	{
		_scale = p_scale;
		_updateModel();
	}

	void Transform::onUpdateEvent(spk::UpdateEvent &p_event)
	{
		if (_velocity != spk::Vector3())
		{
			move(_velocity * (float)p_event.deltaTime.milliseconds);
		}
	}

	void Transform::_updateModel()
	{
		spk::Matrix4x4 translationMatrix = spk::Matrix4x4::translation(_localPosition);
		spk::Matrix4x4 scaleMatrix = spk::Matrix4x4::scale(_scale);
		spk::Matrix4x4 rotationMatrix = spk::Matrix4x4::rotation(_rotation);

		spk::Matrix4x4 parentModel = spk::Matrix4x4::identity();
		spk::SafePointer<Entity> entityOwner = owner();

		if (entityOwner != nullptr && entityOwner->parent() != nullptr)
		{
			const Entity *parentEntity = static_cast<const Entity *>(entityOwner->parent());
			const Transform &parentTransform = parentEntity->transform();
			parentModel = parentTransform.model();
		}

		_model = scaleMatrix * rotationMatrix * translationMatrix * parentModel;
		_inverseModel = _model.inverse();

		spk::Vector3 right(_model[0][0], _model[0][1], _model[0][2]);
		spk::Vector3 up(_model[1][0], _model[1][1], _model[1][2]);
		spk::Vector3 forward(_model[2][0], _model[2][1], _model[2][2]);

		_right = right.normalize();
		_up = up.normalize();
		_forward = (-forward).normalize();

		_position = spk::Vector3(_model[3][0], _model[3][1], _model[3][2]);

		_onEditContractProvider.trigger();	
	}
}
