#pragma once

#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_transform.hpp"
#include "structure/math/spk_matrix.hpp"
#include "structure/math/spk_vector3.hpp"

namespace spk
{
    class Camera : public spk::Component
    {
    private:
        spk::Matrix4x4 _projectionMatrix;

    public:
        Camera(const std::wstring& p_name);

        void setPerspective(float p_fovDegrees, float p_aspectRatio, float p_nearPlane = 0.1f, float p_farPlane = 1000.0f);
        void setOrthographic(float p_left, float p_right, float p_bottom, float p_top, float p_nearPlane= 0.1f, float p_farPlane = 1000.0f);
		
		const spk::Matrix4x4& viewMatrix();
        const spk::Matrix4x4& projectionMatrix() const;
        spk::Matrix4x4 MVP();
    };
}
