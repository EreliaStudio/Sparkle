#include "game_engine/spk_transform.hpp"
#include "game_engine/spk_game_object.hpp"

namespace spk
{
	void Transform::_computeDirections()
	{
		if (_owner != nullptr)
		{
			_forward = _owner->globalRotation().applyRotation(spk::Vector3(0, 0, 1));
			_right = _owner->globalRotation().applyRotation(spk::Vector3(1, 0, 0));
			_up = _owner->globalRotation().applyRotation(spk::Vector3(0, 1, 0));
		}
		else
		{
			_forward = rotation.get().applyRotation(spk::Vector3(0, 0, 1));
			_right = rotation.get().applyRotation(spk::Vector3(1, 0, 0));
			_up = rotation.get().applyRotation(spk::Vector3(0, 1, 0));
		}
	}

	void Transform::_bind(GameObject* p_owner)
	{
		_owner = p_owner;
		_computeDirections();
	}

	Transform::Transform() :
		translation(0.0f, 0.0f, 0.0f),
		scale(1.0f, 1.0f, 1.0f),
		rotation(0.0f, 0.0f, 0.0f, 1.0f)
	{
		rotationContract = std::move(rotation.subscribe([&](){
				_computeDirections();
			}));
		_computeDirections();
	}

	Transform::Transform(const Transform& p_other) :
		translation(p_other.translation.get()),
		scale(p_other.scale.get()),
		rotation(p_other.rotation.get()),
		_owner(p_other._owner),
		_forward(p_other._forward),
		_right(p_other._right),
		_up(p_other._up)
	{
	}

	Transform& Transform::operator = (const Transform& p_other)
	{
		if (this != &p_other)
		{
			translation = p_other.translation.get();
			scale = p_other.scale.get();
			rotation = p_other.rotation.get();	
			_owner = p_other._owner;
			_forward = p_other._forward;
			_right = p_other._right;
			_up = p_other._up;
		}

		return (*this);
	}

	void Transform::lookAt(const spk::Vector3& p_target)
	{
		rotation = spk::Quaternion();
		Vector3 position = _owner->globalPosition();
        Vector3 direction = (p_target - position).normalize();

        if (direction.norm() != 0)
        {
            rotation = Quaternion::lookAt(
					direction,
					Vector3(0.0f, 1.0f, 0.0f)
				); 
		}
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