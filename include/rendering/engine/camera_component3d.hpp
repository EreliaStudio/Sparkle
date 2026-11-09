#pragma once

#include "engine/component3d.hpp"
#include "math/camera_3d.hpp"

namespace spk
{
	class CameraComponent3D : public Component3D
	{
	private:
		Camera3D _projection;

	public:
		using Component3D::Component3D;

		void setProjectionMode(Camera3D::ProjectionMode mode);
		void setPerspective(float fieldOfView, float aspectRatio, float nearPlane, float farPlane);
		void setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);

		[[nodiscard]] Camera3D::ProjectionMode projectionMode() const noexcept;
		[[nodiscard]] const Matrix4x4 &projectionMatrix() const noexcept;
		[[nodiscard]] Matrix4x4 viewMatrix() const;
		[[nodiscard]] Matrix4x4 viewProjectionMatrix() const;
	};
}
