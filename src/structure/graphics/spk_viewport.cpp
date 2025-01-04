#include "structure/graphics/spk_viewport.hpp"

#include <GL/glew.h>

#include "structure/spk_iostream.hpp"
#include "structure/math/spk_matrix.hpp"

namespace spk
{
	Viewport::Viewport() :
		_geometry({0, 0}, {1, 1})
	{

	}

	Viewport::Viewport(const Geometry2D& p_geometry) :
		_geometry(p_geometry)
	{

	}

	const Geometry2D& Viewport::geometry() const
	{
		return (_geometry);
	}

	void Viewport::setGeometry(const Geometry2D& p_geometry)
	{
		_geometry = p_geometry;
	}

	void Viewport::apply() const
	{
		_convertionOffset = _geometry.size / 2.0f;
		_matrix = spk::Matrix4x4::ortho(0.0f, static_cast<float>(_geometry.width), static_cast<float>(_geometry.height), 0.0f, -_maxLayer, 0.0f);
		glViewport(static_cast<GLint>(_geometry.x), static_cast<GLint>(_geometry.y), static_cast<GLsizei>(_geometry.width), static_cast<GLsizei>(_geometry.height));
	}

	spk::Matrix4x4 Viewport::matrix()
	{
		return (_matrix);
	}

	spk::Vector2 Viewport::convertScreenToOpenGL(const spk::Vector2Int p_screenPosition)
	{
		return ((_matrix * spk::Vector4(p_screenPosition - _convertionOffset, 0, 0)).xy());
	}

	spk::Vector2 Viewport::convertScreenToOpenGL(int p_screenPositionX, int p_screenPositionY)
	{
		return ((_matrix * spk::Vector4(p_screenPositionX - _convertionOffset.x, p_screenPositionY - _convertionOffset.y, 0, 0)).xy());
	}

	float Viewport::convertLayerToOpenGL(const float& p_layer)
	{
		return (p_layer / _maxLayer);	
	}

	spk::Vector3 Viewport::convertScreenToOpenGL(const spk::Vector2Int p_screenPosition, const float& p_layer)
	{
		return ((_matrix * spk::Vector4(p_screenPosition - _convertionOffset, p_layer, 0)).xyz());
	}

	spk::Vector3 Viewport::convertScreenToOpenGL(int p_screenPositionX, int p_screenPositionY, const float& p_layer)
	{
		return ((_matrix * spk::Vector4(p_screenPositionX - _convertionOffset.x, p_screenPositionY - _convertionOffset.y, p_layer, 0)).xyz());
	}
}