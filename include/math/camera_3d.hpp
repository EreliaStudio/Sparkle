#pragma once

#include <cstdint>

#include "container/cached_data.hpp"
#include "math/matrix.hpp"
#include "math/vector3.hpp"

namespace spk
{
	class Camera3D
	{
	public:
		enum class ProjectionMode : std::uint8_t
		{
			Perspective,
			Orthographic
		};

	private:
		Vector3 _position{};
		Quaternion _rotation{};
		ProjectionMode _projectionMode = ProjectionMode::Perspective;
		Matrix4x4 _perspective = Matrix4x4::perspective(1.0471975512f, 1.0f, 0.1f, 1000.0f);
		Matrix4x4 _orthographic = Matrix4x4::ortho(-1, 1, -1, 1, 0.1f, 1000.0f);
		mutable CachedData<Matrix4x4> _view{[this] {
			return Matrix4x4::rotation(_rotation.conjugated()) *
				   Matrix4x4::translation(-_position.x, -_position.y, -_position.z);
		}};
		mutable CachedData<Matrix4x4> _viewProjection{[this] {
			return projectionMatrix() * viewMatrix();
		}};

	public:
		Camera3D() = default;
		Camera3D(const Camera3D &other);
		Camera3D(Camera3D &&other);
		Camera3D &operator=(const Camera3D &other);
		Camera3D &operator=(Camera3D &&other);

		void setPosition(Vector3 position);
		void setRotation(Quaternion rotation);
		void lookAt(Vector3 target, Vector3 up = {0, 1, 0});
		void setProjectionMode(ProjectionMode mode);
		void setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
		void setPerspective(float fieldOfView, float aspectRatio, float nearPlane, float farPlane);

		[[nodiscard]] const Vector3 &position() const noexcept;
		[[nodiscard]] Quaternion rotation() const noexcept;
		[[nodiscard]] ProjectionMode projectionMode() const noexcept;
		[[nodiscard]] const Matrix4x4 &viewMatrix() const;
		[[nodiscard]] const Matrix4x4 &projectionMatrix() const noexcept;
		[[nodiscard]] Matrix4x4 viewProjectionMatrix() const;
	};
}
