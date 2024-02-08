#include "game_engine/spk_transform.hpp"

namespace spk
{
	void Transform::_computeDirections()
	{
		float pitch = spk::degreeToRadian(rotation.get().x);
		float yaw = spk::degreeToRadian(rotation.get().y);
		float roll = spk::degreeToRadian(rotation.get().z);

		spk::Vector3 tmpForward(0, 0, 1); 
		spk::Vector3 tmpRight(1, 0, 0);
		spk::Vector3 tmpUp(0, 1, 0);
		
		spk::Vector3 forwardRotated = tmpForward * cos(yaw) + tmpRight * sin(yaw);
		tmpRight = tmpRight * cos(yaw) - tmpForward * sin(yaw);

		spk::Vector3 upRotated = tmpUp * cos(roll) - tmpRight * sin(roll);

		_right = tmpRight * cos(roll) + tmpUp * sin(roll);
		_forward = forwardRotated * cos(pitch) + tmpUp * sin(pitch);
		_up = upRotated * cos(pitch) - forwardRotated * sin(pitch);
	}

	Transform::Transform() :
		translation(0.0f, 0.0f, 0.0f),
		scale(1.0f, 1.0f, 1.0f),
		rotation(0.0f, 0.0f, 0.0f)
	{
		rotationContract = rotation.subscribe([&](){_computeDirections();});
		_computeDirections();
	}

	Transform::Transform(const Transform& p_other) :
		translation(p_other.translation.get()),
		scale(p_other.scale.get()),
		rotation(p_other.rotation.get())
	{

	}

	Transform& Transform::operator = (const Transform& p_other)
	{
		if (this != &p_other)
		{
			translation = p_other.translation.get();
			scale = p_other.scale.get();
			rotation = p_other.rotation.get();	
		}

		return (*this);
	}

	void Transform::lookAt(const spk::Vector3& p_target)
	{
		Vector3 position(translation.get());
		_forward = (p_target - position).normalize();

		if (_forward.norm() == 0)
			return;

		Vector3 worldUp = Vector3(0, 1, 0);
		float dot = worldUp.dot(_forward);
		if (dot == 1 || dot == -1 )
			worldUp = Vector3(0, 0, 1);

		_right = (worldUp.cross(_forward)).normalize();
		_up = _forward.cross(_right);

		float yaw = atan2(_forward.z, _forward.x);
		float pitch = asin(-_forward.y);
		float roll = atan2(_up.y, _right.y);

		Vector3 eulerRotation = Vector3::radianToDegree(Vector3(pitch, yaw, roll));

		rotation.setWithoutNotification(eulerRotation);
	}

	const spk::Vector3& Transform::forward() const
	{
		return (_forward);
	}

	const spk::Vector3& Transform::right() const
	{
		return (_right);
	}

	const spk::Vector3& Transform::up() const
	{
		return (_up);
	}
}