#include "graphics/spk_viewport.hpp"

#include "graphics/pipeline/spk_pipeline.hpp"
#include "math/spk_matrix4x4.hpp"

namespace spk
{
    spk::Vector2 Viewport::convertScreenToGPUPosition(const spk::Vector2Int& p_screenPosition)
    {
        return Vector2(
            2.0f * static_cast<float>(p_screenPosition.x) / static_cast<float>(_activeViewport->size().x) - 1.0f,
            (2.0f * static_cast<float>(p_screenPosition.y) / static_cast<float>(_activeViewport->size().y) - 1.0f) * -1);
    }

    Viewport::Viewport()
    {
        _anchor = 0;
        _size = 0;
    }

    void Viewport::setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
    {
        _anchor = p_anchor;
        _size = p_size;
    }

    void Viewport::activate() const
    {
        if (_screenConstant == nullptr)
            _screenConstant = spk::Pipeline::globalConstant("screenConstants");

        if (_screenConstant != nullptr)
        {
            spk::Matrix4x4 canvasMatrix = spk::Matrix4x4::translationMatrix(spk::Vector3(-1, 1, 0)) * spk::Matrix4x4::scaleMatrix(spk::Vector3(2.0f / size().x, -2.0f / size().y, 0.0001f));
            DLOG(spk::Matrix4x4::translationMatrix(spk::Vector3(-1, 1, 0)));
            DLOG(spk::Matrix4x4::scaleMatrix(spk::Vector3(2.0f / size().x, -2.0f / size().y, 0.0001f)));
            DLOG(canvasMatrix);
            _screenConstant->operator[]("canvasMVP") = canvasMatrix;
            _screenConstant->update();
        }

        glViewport(_anchor.x, _mainViewport->size().y - (_size.y + _anchor.y), _size.x, _size.y);
        _activeViewport = this;
    }

    const spk::Vector2Int& Viewport::anchor() const
    {
        return (_anchor);
    }

    const spk::Vector2UInt& Viewport::size() const
    {
        return (_size);
    }
}