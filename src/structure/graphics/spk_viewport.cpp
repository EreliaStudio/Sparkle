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
		glViewport(static_cast<GLint>(_geometry.x), static_cast<GLint>(_geometry.y), static_cast<GLsizei>(_geometry.width), static_cast<GLsizei>(_geometry.heigth));
	}
}