#pragma once

#include <optional>

#include "container/cached_data.hpp"
#include "engine/component3d.hpp"
#include "math/camera_3d.hpp"

namespace spk
{
	class CameraComponent3D : public Component3D
	{
	private:
		struct WorldPose
		{
			Vector3 position;
			Quaternion rotation;

			[[nodiscard]] bool operator==(const WorldPose &) const = default;
		};

		struct ViewMatrices
		{
			Matrix4x4 view;
			Matrix4x4 inverseView;
		};

		Camera3D _projection;
		mutable std::optional<WorldPose> _cachedWorldPose;
		mutable CachedData<ViewMatrices> _viewMatrices{[this] {
			const WorldPose &pose = *_cachedWorldPose;
			return ViewMatrices{
				Matrix4x4::rotation(pose.rotation.conjugated()) * Matrix4x4::translation(-pose.position),
				Matrix4x4::translation(pose.position) * Matrix4x4::rotation(pose.rotation)};
		}};

		void _refreshViewCache() const;

	public:
		using Component3D::Component3D;

		void setProjectionMode(Camera3D::ProjectionMode mode);
		void setPerspective(float fieldOfView, float aspectRatio, float nearPlane, float farPlane);
		void setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);

		[[nodiscard]] Camera3D::ProjectionMode projectionMode() const noexcept;
		[[nodiscard]] const Matrix4x4 &projectionMatrix() const noexcept;
		[[nodiscard]] const Matrix4x4 &viewMatrix() const;
		[[nodiscard]] const Matrix4x4 &inverseViewMatrix() const;
		[[nodiscard]] Matrix4x4 viewProjectionMatrix() const;
	};
}
