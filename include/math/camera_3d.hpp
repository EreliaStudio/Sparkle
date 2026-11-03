#pragma once

#include "container/cached_data.hpp"
#include "math/matrix.hpp"
#include "math/vector3.hpp"

namespace spk
{
	class Camera3D
	{
	private:
		Vector3 _position{};
		Quaternion _rotation{};
		Matrix4x4 _projection = Matrix4x4::identity();
		mutable CachedData<Matrix4x4> _view{[this] {
			return Matrix4x4::rotation(_rotation.conjugated()) *
				   Matrix4x4::translation(-_position.x, -_position.y, -_position.z);
		}};

	public:
		void setPosition(Vector3 position);
		void setRotation(Quaternion rotation);
		void setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
		void setPerspective(float fieldOfView, float aspectRatio, float nearPlane, float farPlane);

		[[nodiscard]] const Vector3 &position() const noexcept;
		[[nodiscard]] Quaternion rotation() const noexcept;
		[[nodiscard]] const Matrix4x4 &viewMatrix() const;
		[[nodiscard]] const Matrix4x4 &projectionMatrix() const noexcept;
		[[nodiscard]] Matrix4x4 viewProjectionMatrix() const;
	};
}
