#include "structure/graphics/spk_viewport.hpp"

#include <GL/glew.h>

#include "structure/spk_iostream.hpp"
#include "structure/math/spk_matrix.hpp"

#include "structure/graphics/spk_pipeline.hpp"

namespace spk
{
	static spk::Pipeline::Constant* ScreenConstants = nullptr;

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
		if (ScreenConstants == nullptr)
		{
			try
			{
				ScreenConstants = &(spk::Pipeline::constants(L"spk::ScreenConstants"));
			}
			catch (...)
			{
				ScreenConstants = nullptr;
			}
		}

		if (ScreenConstants != nullptr)
		{
			spk::Matrix4x4 canvasMatrix = spk::Matrix4x4::ortho(0.0f, static_cast<float>(_geometry.width), static_cast<float>(_geometry.heigth), 0.0f, -_maxLayer, 0.0f);

			ScreenConstants->operator[](L"canvasMVP") = canvasMatrix;
			ScreenConstants->validate();
		}

		if (_geometry.width == 0 || _geometry.heigth == 0)
		{
			throw std::runtime_error("Can't apply a viewport of size [" + std::to_string(_geometry.width) + " / " + std::to_string(_geometry.heigth) + "]");
		}

		glViewport(static_cast<GLint>(_geometry.x), static_cast<GLint>(_geometry.y), static_cast<GLsizei>(_geometry.width), static_cast<GLsizei>(_geometry.heigth));
	}
}