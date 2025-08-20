#include "structure/graphics/spk_viewport.hpp"

#include "structure/system/spk_exception.hpp"

#include <GL/glew.h>

#include <GL/gl.h>

#include "structure/math/spk_matrix.hpp"
#include "structure/spk_iostream.hpp"

namespace spk
{
	float Viewport::_maxLayer = 10000;
	Matrix4x4 Viewport::_matrix = spk::Matrix4x4();
	spk::Vector2 Viewport::_convertionOffset = spk::Vector2(0, 0);
	spk::Vector2 Viewport::_clippingOffset = spk::Vector2(0, 0);
	spk::SafePointer<const Viewport> Viewport::_appliedViewport = nullptr;

	Viewport::Viewport() :
		_geometry({0, 0}, {1, 1})
	{
	}

	Viewport::Viewport(const Geometry2D &p_geometry) :
		_geometry(p_geometry)
	{
	}

	const Geometry2D &Viewport::geometry() const
	{
		return (_geometry);
	}

	void Viewport::setGeometry(const Geometry2D &p_geometry)
	{
		_geometry = p_geometry;
	}

	const Geometry2D &Viewport::clippedGeometry() const
	{
		return (_clippedGeometry);
	}

	void Viewport::setClippedGeometry(const Geometry2D &p_clippedGeometry)
	{
		_clippedGeometry = p_clippedGeometry;
	}

	void Viewport::setWindowSize(const spk::Vector2UInt &p_windowSize)
	{
		_windowSize = p_windowSize;
	}

	const spk::Vector2UInt &Viewport::windowSize() const
	{
		return (_windowSize);
	}

	void Viewport::invertXAxis()
	{
		_invertXAxis = !_invertXAxis;
	}

	void Viewport::invertYAxis()
	{
		_invertYAxis = !_invertYAxis;
	}

	void Viewport::invertZAxis()
	{
		_invertZAxis = !_invertZAxis;
	}

	spk::SafePointer<const Viewport> Viewport::activeViewport()
	{
		return (_appliedViewport);
	}

	void Viewport::apply() const
	{
		if (_windowSize.x <= 0 || _windowSize.y <= 0)
		{
			GENERATE_ERROR(
				"Viewport::apply() - window size must be positive "
				"(got " +
				std::to_string(_windowSize.x) + " x " + std::to_string(_windowSize.y) + ')');
		}

		if (_geometry.width <= 0 || _geometry.height <= 0)
		{
			GENERATE_ERROR(
				"Viewport::apply() - viewport width/height must be positive "
				"(got " +
				std::to_string(_geometry.width) + " x " + std::to_string(_geometry.height) + ')');
		}

		if (_clippedGeometry.width <= 0 || _clippedGeometry.height <= 0)
		{
			GENERATE_ERROR(
				"Viewport::apply() - viewport clipped width/height must be positive "
				"(got " +
				std::to_string(_clippedGeometry.width) + " x " + std::to_string(_clippedGeometry.height) + ')');
		}

		_convertionOffset = static_cast<spk::Vector2>(_geometry.size) / 2.0f;

		float left = (_invertXAxis == false ? _geometry.x : static_cast<float>(_geometry.x + _geometry.width));
		float right = (_invertXAxis == true ? _geometry.x : static_cast<float>(_geometry.x + _geometry.width));

		float top = (_invertYAxis == false ? _geometry.y : static_cast<float>(_geometry.y + _geometry.height));
		float down = (_invertYAxis == true ? _geometry.y : static_cast<float>(_geometry.y + _geometry.height));

		float near = (_invertZAxis == false ? -_maxLayer : 0);
		float far = (_invertZAxis == true ? -_maxLayer : 0);

		_matrix = spk::Matrix4x4::ortho(left, right, down, top, near, far);

		GLint viewportX = static_cast<GLint>(_geometry.x);
		GLint viewportY = static_cast<GLint>(_windowSize.y - _geometry.y - _geometry.height); // flip Y
		GLsizei viewportW = static_cast<GLsizei>(_geometry.width);
		GLsizei viewportH = static_cast<GLsizei>(_geometry.height);

		GLint scissorX = static_cast<GLint>(_clippedGeometry.x);
		GLint scissorY = static_cast<GLint>(_windowSize.y - _clippedGeometry.y - _clippedGeometry.height); // flip Y
		GLsizei scissorW = static_cast<GLsizei>(_clippedGeometry.width);
		GLsizei scissorH = static_cast<GLsizei>(_clippedGeometry.height);

		if (viewportW <= 0 || viewportH <= 0)
		{
			GENERATE_ERROR(
				"Viewport::apply() - viewport width/height must be positive "
				"(got " +
				std::to_string(viewportW) + " x " + std::to_string(viewportH) + ')');
		}

		try
		{
			glViewport(viewportX, viewportY, viewportW, viewportH);
			glScissor(scissorX, scissorY, scissorW, scissorH);
		} catch (...)
		{
			GENERATE_ERROR("Viewport::apply() - Error while applying a viewport");
		}

		_appliedViewport = this;
	}

	spk::Matrix4x4 Viewport::matrix()
	{
		return (_matrix);
	}

	spk::Vector2 Viewport::convertScreenToOpenGL(const spk::Vector2Int p_screenPosition)
	{
		return ((_matrix * spk::Vector4(static_cast<spk::Vector2>(p_screenPosition) - _convertionOffset, 0, 0)).xy() + _clippingOffset);
	}

	spk::Vector2 Viewport::convertScreenToOpenGL(int p_screenPositionX, int p_screenPositionY)
	{
		return (
			(_matrix *
			 spk::Vector4(
				 static_cast<float>(p_screenPositionX) - _convertionOffset.x, static_cast<float>(p_screenPositionY) - _convertionOffset.y, 0, 0))
				.xy() +
			_clippingOffset);
	}

	float Viewport::convertLayerToOpenGL(const float &p_layer)
	{
		return (p_layer / _maxLayer);
	}

	spk::Vector3 Viewport::convertScreenToOpenGL(const spk::Vector2Int p_screenPosition, const float &p_layer)
	{
		return (
			(_matrix * spk::Vector4(static_cast<spk::Vector2>(p_screenPosition) - _convertionOffset, p_layer, 0)).xyz() +
			spk::Vector3(_clippingOffset, 0));
	}

	spk::Vector3 Viewport::convertScreenToOpenGL(int p_screenPositionX, int p_screenPositionY, const float &p_layer)
	{
		return (
			(_matrix * spk::Vector4(
						   static_cast<float>(p_screenPositionX) - _convertionOffset.x,
						   static_cast<float>(p_screenPositionY) - _convertionOffset.y,
						   p_layer,
						   0))
				.xyz() +
			spk::Vector3(_clippingOffset, 0));
	}
}