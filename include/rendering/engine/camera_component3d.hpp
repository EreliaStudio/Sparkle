#pragma once

#include <optional>

#include "design_pattern/contract_provider.hpp"
#include <string>

#include "container/cached_data.hpp"
#include "engine/component3d.hpp"
#include "math/camera_3d.hpp"

namespace spk
{
	class EngineView;

	class CameraComponent3D : public Component3D
	{
	private:
		struct WorldPose
		{
			Vector3 position;
			Quaternion rotation;

			[[nodiscard]] bool operator==(const WorldPose &) const = default;
		};

		Camera3D _projection;
		ContractProvider<> _onProjectionEdition;
		mutable std::optional<WorldPose> _cachedWorldPose;
		mutable CachedData<Matrix4x4> _view{[this] {
			const WorldPose &pose = *_cachedWorldPose;
			return Matrix4x4::rotation(pose.rotation.conjugated()) * Matrix4x4::translation(-pose.position);
		}};
		mutable CachedData<Matrix4x4> _inverseView{[this] {
			const WorldPose &pose = *_cachedWorldPose;
			return Matrix4x4::translation(pose.position) * Matrix4x4::rotation(pose.rotation);
		}};

		void _refreshViewCache() const;

	public:
		using OnProjectionEditionContract = ContractProvider<>::Contract;
		using Component3D::Component3D;
		[[nodiscard]] OnProjectionEditionContract subscribeToProjectionEdition(ContractProvider<>::callback_type callback);

		void setProjectionMode(Camera3D::ProjectionMode mode);
		void setPerspective(float fieldOfView, float aspectRatio, float nearPlane, float farPlane);
		void setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);

		[[nodiscard]] Camera3D::ProjectionMode projectionMode() const noexcept;
		[[nodiscard]] const Matrix4x4 &projectionMatrix() const noexcept;
		[[nodiscard]] Camera3D captureCamera() const;
		[[nodiscard]] const Matrix4x4 &viewMatrix() const;
		[[nodiscard]] const Matrix4x4 &inverseViewMatrix() const;
		[[nodiscard]] Matrix4x4 viewProjectionMatrix() const;

		[[nodiscard]] EngineView &view(const std::string &name);
		[[nodiscard]] const EngineView &view(const std::string &name) const;
	};
}
