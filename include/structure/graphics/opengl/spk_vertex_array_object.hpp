#pragma once

#include <GL/glew.h>

#include <GL/gl.h>

namespace spk::OpenGL
{
	class VertexArrayObject
	{
	private:
		GLuint _id;

		void _allocate();
		void _release();

	public:
		VertexArrayObject();
		~VertexArrayObject();

		VertexArrayObject(const VertexArrayObject &p_other);
		VertexArrayObject(VertexArrayObject &&p_other);

		VertexArrayObject &operator=(const VertexArrayObject &p_other);
		VertexArrayObject &operator=(VertexArrayObject &&p_other);

		void activate();
		void deactivate();
	};
}
