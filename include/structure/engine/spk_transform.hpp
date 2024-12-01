#pragma once

#include "structure/engine/spk_component.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/math/spk_matrix.hpp"
#include <cmath>

namespace spk
{
	class Transform : public Component
	{
	private:
		spk::Matrix4x4 _model;

		spk::Vector3 _position;
		spk::Vector3 _rotation; // Express as degree
		spk::Vector3 _scale;

		spk::Vector3 _velocity; // Unit : unit / milliseconds

		spk::Vector3 _forward;
		spk::Vector3 _right;
		spk::Vector3 _up;

	public:
		Transform(const spk::SafePointer<Entity>& p_owner) : Component(L"Transform", p_owner),
			_model(),
			_position(0.0f, 0.0f, 0.0f),
			_rotation(0.0f, 0.0f, 0.0f),
			_scale(1.0f, 1.0f, 1.0f),
			_velocity(0.0f, 0.0f, 0.0f),
			_forward(0.0f, 0.0f, 1.0f),
			_right(1.0f, 0.0f, 0.0f),
			_up(0.0f, 1.0f, 0.0f)
		{
			_updateModel();
		}

		const spk::Matrix4x4& model() const
		{
			return (_model);
		}

		const spk::Vector3& position() const
		{
			return (_position);
		}

		const spk::Vector3& rotation() const
		{
			return (_rotation);
		}

		const spk::Vector3& scale() const
		{
			return (_scale);
		}

		const spk::Vector3& forward() const
		{
			return (_forward);
		}

		const spk::Vector3& right() const
		{
			return (_right);
		}

		const spk::Vector3& up() const
		{
			return (_up);
		}

		void lookAt(const spk::Vector3& target)
		{
			spk::Vector3 direction = (target - _position).normalize();

			float yaw = std::atan2(direction.x, direction.z);
			float pitch = std::asin(-direction.y);

			_rotation = spk::Vector3(pitch, yaw, 0.0f);

			_updateModel();
		}

		void setVelocity(const spk::Vector3& p_velocity) // Express as Unit/second
		{
			_velocity = p_velocity / 1000.0f;
		}

		void addVelocity(const spk::Vector3& p_delta) // Express as Unit/second
		{
			_velocity += p_delta / 1000.0f;
		}

		void move(const spk::Vector3& p_delta)
		{
			_position += p_delta;
			_updateModel();
		}

		void place(const spk::Vector3& p_newPosition)
		{
			_position = p_newPosition;
			_updateModel();
		}

		void rotate(const spk::Vector3& p_delta)
		{
			_rotation += p_delta;
			_updateModel();
		}

		void setRotation(const spk::Vector3& p_rotation)
		{
			_rotation = p_rotation;
			_updateModel();
		}

		void setScale(const spk::Vector3 p_scale)
		{
			_scale = p_scale;
			_updateModel();
		}

		void setScale(const spk::Vector3& p_scale)
		{
			_scale = p_scale;
			_updateModel();
		}

		void update(const long long& p_duration)
		{
			if (_velocity != spk::Vector3())
			{
				_position += _velocity * p_duration;
				_updateModel();
			}
		}

	private:
		void _updateModel()
		{
			spk::Matrix4x4 translationMatrix = spk::Matrix4x4::translationMatrix(_position);
			spk::Matrix4x4 rotationMatrix = spk::Matrix4x4::rotationMatrix(_rotation);
			spk::Matrix4x4 scaleMatrix = spk::Matrix4x4::scaleMatrix(_scale);

			_model = translationMatrix * rotationMatrix * scaleMatrix;
			_forward = rotationMatrix * (spk::Vector3(0.0f, 0.0f, 1.0f));
			_right = rotationMatrix * (spk::Vector3(1.0f, 0.0f, 0.0f));
			_up = rotationMatrix * (spk::Vector3(0.0f, 1.0f, 0.0f));
		}
	};
}
