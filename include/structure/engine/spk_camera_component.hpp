#pragma once

#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_camera.hpp"
#include "structure/graphics/spk_geometry_2D.hpp"

namespace spk
{
	class CameraComponent : public spk::Component
	{
	private:
		spk::Camera _camera;
		spk::ContractProvider::Contract _transformContract;

		struct Policy
		{
			enum class Mode
			{
				Perspective,
				Orthographic
			};

			Mode mode = Mode::Perspective;

			union Data
			{
				struct {
					float fovDegrees;
					float fallbackAspect; // used before first geometry callback
				} perspective;
	
				struct {
					float pixelsPerUnitX;
					float pixelsPerUnitY;
					float nearPlane;
					float farPlane;
				} orthographic;
			} data;

			float nearPlane;
			float farPlane;
		};

		Policy _policy;

		bool _hasLastGeometry = false;
		spk::Geometry2D _lastGeometry;

		void _updateUBO();
		void _applyProjectionForGeometry(const spk::Geometry2D &p_geometry);

	public:
		CameraComponent(const std::wstring &p_name);

		void setPerspective(float p_fov, float p_aspectRatio, float p_near = 0.1f, float p_far = 1000.0f);
		// Set orthographic by desired cell size on screen (pixels per world unit)
		void setOrthographic(float p_cellPixelWidth, float p_cellPixelHeight, float p_near = 0.1f, float p_far = 1000.0f);

		void start() override;
		void onGeometryChange(const spk::Geometry2D &p_geometry) override;

		const spk::Camera &camera() const;
	};
}
