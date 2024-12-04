#pragma once

#include <optional>

#include "structure/engine/2d/spk_entity_2d.hpp"
#include "structure/engine/2d/spk_transform_2d.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_camera.hpp"

#include "structure/graphics/lumina/spk_shader.hpp"
#include "structure/math/spk_matrix.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	class CameraComponent2D : public spk::Component
	{
	public:
		struct Policy
		{
			float nearPlane = 0.1f;
			float farPlane = 1000.0f;
			spk::Vector2 pixelsPerUnit = {32.0f, 32.0f};
		};

	private:
		spk::Camera _camera;
		Policy _policy;

		std::optional<spk::Geometry2D> _geometry;

		Transform2D::Contract _transformContract;

		void _updateUBO();
		void _applyProjection();

	public:
		CameraComponent2D(const std::wstring &p_name);

		void setPixelsPerUnit(const spk::Vector2 &p_pixelsPerUnit);
		void setNearFar(float p_near, float p_far);

		void onGeometryChange(const spk::Geometry2D &p_geometry) override;

		void start() override;

		const spk::Camera &camera() const
		{
			return _camera;
		}
		const Policy &policy() const
		{
			return _policy;
		}

		spk::Vector2 screenToWorld(const spk::Vector2Int &p_screenPosition) const;
		spk::Vector2 ndcToWorld(const spk::Vector2 &p_ndc) const;
	};
}
