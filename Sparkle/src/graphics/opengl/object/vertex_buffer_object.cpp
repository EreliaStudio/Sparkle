#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk::OpenGL
{
    VertexBufferObject::VertexBufferObject(const Mode &p_mode) :
		_mode(static_cast<GLenum>(p_mode)),
		_allocatedSize(0),
		_size(0)
	{
        glGenBuffers(1, &_handle);
    }

    VertexBufferObject::~VertexBufferObject()
	{
        if (_handle != 0)
        {
            glDeleteBuffers(1, &_handle);
        }
    }

    VertexBufferObject::VertexBufferObject(VertexBufferObject&& p_other) :
        _handle(p_other._handle),
        _mode(p_other._mode),
        _allocatedSize(p_other._allocatedSize),
        _size(p_other._size)
    {
        p_other._handle = 0;
        p_other._allocatedSize = 0;
        p_other._size = 0;
    }
    
    VertexBufferObject& VertexBufferObject::operator=(VertexBufferObject&& p_other)
    {
        if (this != &p_other)
        {
            glDeleteBuffers(1, &_handle);

            _handle = p_other._handle;
            _mode = p_other._mode;
            _allocatedSize = p_other._allocatedSize;
            _size = p_other._size;

            p_other._handle = 0;
            p_other._allocatedSize = 0;
            p_other._size = 0;
        }
        return *this;
    }

    void VertexBufferObject::activate()
	{
        glBindBuffer(_mode, _handle);
    }

    void VertexBufferObject::deactivate()
	{
        glBindBuffer(_mode, 0);
    }

    void VertexBufferObject::push(const void* p_data, size_t p_dataSize)
	{
        activate();
        if (p_dataSize > _allocatedSize)
		{
            glBufferData(_mode, p_dataSize, p_data, GL_DYNAMIC_DRAW);
            _allocatedSize = p_dataSize;
        }
		else
		{
            glBufferSubData(_mode, 0, p_dataSize, p_data);
        }
        _size = p_dataSize;
    }

    void VertexBufferObject::edit(size_t p_offset, const void* p_data, size_t p_dataSize)
	{
        glBindBuffer(_mode, _handle);
        if (p_offset + p_dataSize <= _allocatedSize)
		{
            glBufferSubData(_mode, p_offset, p_dataSize, p_data);
        }
		else
		{
            throw std::out_of_range("Attempting to edit beyond the allocated buffer size.");
        }
    }


    void VertexBufferObject::clear()
    {
        _size = 0;
    }

    GLuint VertexBufferObject::handle() const
	{
        return (_handle);
    }
            
    size_t VertexBufferObject::size() const
    {
        return (_size);
    }
}