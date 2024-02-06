#include "graphics/opengl/spk_opengl_object.hpp"

#include <iostream>

namespace spk::OpenGL
{
    SamplerObject::SamplerObject(GLint location) 
        : _location(location), _textureUnit(0)
    {
    }

    SamplerObject& SamplerObject::operator=(GLint textureUnit)
    {
        _textureUnit = textureUnit;
        return *this;
    }

    void SamplerObject::activate()
    {
        glUniform1i(_location, _textureUnit);
    }
}