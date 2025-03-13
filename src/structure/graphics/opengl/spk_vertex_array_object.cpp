#include "structure/graphics/opengl/spk_vertex_array_object.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

namespace spk::OpenGL
{
	VertexArrayObject::VertexArrayObject() :
		_id(0)
	{
	}

	VertexArrayObject::~VertexArrayObject()
	{
		if (_id != 0)
		{
			_release();
		}
	}

	VertexArrayObject::VertexArrayObject(const VertexArrayObject &p_other) :
		_id(0)
	{
	}

	VertexArrayObject::VertexArrayObject(VertexArrayObject &&p_other) :
		_id(p_other._id)
	{
		p_other._id = 0;
	}

	VertexArrayObject &VertexArrayObject::operator=(const VertexArrayObject &p_other)
	{
		if (this != &p_other)
		{
			_id = 0;
		}
		return (*this);
	}

	VertexArrayObject &VertexArrayObject::operator=(VertexArrayObject &&p_other)
	{
		if (this != &p_other)
		{
			_id = p_other._id;
			p_other._id = 0;
		}
		return (*this);
	}

	void VertexArrayObject::_allocate()
	{
		if (wglGetCurrentContext() != nullptr)
		{
			glGenVertexArrays(1, &_id);
		}
	}

	void VertexArrayObject::_release()
	{
		if (wglGetCurrentContext() != nullptr)
		{
			glDeleteVertexArrays(1, &_id);
		}
		_id = 0;
	}

	void VertexArrayObject::activate()
	{
		if (_id == 0)
		{
			_allocate();
		}
		glBindVertexArray(_id);
	}

	void VertexArrayObject::deactivate()
	{
		glBindVertexArray(0);
	}
}
