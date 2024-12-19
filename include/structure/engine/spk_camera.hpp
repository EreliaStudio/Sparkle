#pragma once

#include "structure/engine/spk_component.hpp"
#include "structure/math/spk_matrix.hpp"

namespace spk
{
    class Camera
    {
    private:
        spk::Matrix4x4 _projectionMatrix;

    public:
        Camera();

        void setPerspective(float p_fovDegrees, float p_aspectRatio, float p_nearPlane = 0.1f, float p_farPlane = 1000.0f);
        void setOrthographic(float p_left, float p_right, float p_bottom, float p_top, float p_nearPlane= 0.1f, float p_farPlane = 1000.0f);
		
        const spk::Matrix4x4& projectionMatrix() const;
    };
}
