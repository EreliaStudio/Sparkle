#pragma once

#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_transform.hpp"
#include "structure/math/spk_matrix.hpp"
#include "structure/math/spk_vector3.hpp"

#include "structure/graphics/spk_pipeline.hpp"

namespace spk
{
    class Camera : public spk::Component
    {
    private:
		static spk::Pipeline::Constant* _cameraConstants;
		static Camera* _activeCamera;

		ContractProvider::Contract _onTransformEditionContract;

        spk::Matrix4x4 _projectionMatrix;

		void _updateConstants();

    public:
        Camera(const std::wstring& p_name);

		void activate();

        void setPerspective(float p_fovDegrees, float p_aspectRatio, float p_nearPlane = 0.1f, float p_farPlane = 1000.0f);
        void setOrthographic(float p_left, float p_right, float p_bottom, float p_top, float p_nearPlane= 0.1f, float p_farPlane = 1000.0f);
		
		const spk::Matrix4x4& viewMatrix();
        const spk::Matrix4x4& projectionMatrix() const;
        spk::Matrix4x4 MVP();

		void awake() override;
		void sleep() override;
    };
}
