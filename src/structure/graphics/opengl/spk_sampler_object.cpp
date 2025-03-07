#include "structure/graphics/opengl/spk_sampler_object.hpp"

namespace spk::OpenGL
{
    SamplerObject::SamplerObject() :
		_designator(""),
		_index(-1),
		_texture(nullptr),
		_uniformDestination(-1),
		_type(Type::Texture2D)
    {
    }

    SamplerObject::SamplerObject(const std::string& p_name, Type p_type, size_t p_textureIndex) :
		_designator(p_name),
		_index(static_cast<GLint>(p_textureIndex)),
		_texture(nullptr),
		_uniformDestination(-1),
		_type(p_type)
    {
    }

    SamplerObject::SamplerObject(const SamplerObject& p_other) :
		_designator(p_other._designator),
		_index(p_other._index),
		_texture(p_other._texture),
		_type(p_other._type),
		_uniformDestination(p_other._uniformDestination)
    {
    }

    SamplerObject& SamplerObject::operator=(const SamplerObject& p_other)
    {
        if (this != &p_other)
        {
            _designator = p_other._designator;
            _index = p_other._index;
            _texture = p_other._texture;
            _type = p_other._type;
            _uniformDestination = p_other._uniformDestination;
        }
        return *this;
    }

    SamplerObject::SamplerObject(SamplerObject&& p_other) noexcept :
		_designator(std::move(p_other._designator)),
		_index(p_other._index),
		_texture(p_other._texture),
		_type(p_other._type),
		_uniformDestination(p_other._uniformDestination)
    {
        p_other._texture = nullptr;
        p_other._index   = -1;
        p_other._uniformDestination = -1;
    }

    SamplerObject& SamplerObject::operator=(SamplerObject&& p_other) noexcept
    {
        if (this != &p_other)
        {
            _designator = std::move(p_other._designator);
            _index = p_other._index;
            _texture = p_other._texture;
            _type = p_other._type;
            _uniformDestination = p_other._uniformDestination;

            p_other._texture = nullptr;
            p_other._index = -1;
            p_other._uniformDestination = -1;
        }
        return *this;
    }

    void SamplerObject::bind(const spk::SafePointer<const Texture>& p_texture)
    {
        _texture = p_texture;
    }

    spk::SafePointer<const Texture>& SamplerObject::texture()
    {
        return _texture;
    }

    const spk::SafePointer<const Texture>& SamplerObject::texture() const
    {
        return _texture;
    }

    void SamplerObject::activate()
    {
        if (_uniformDestination == -1)
        {
            GLint prog = 0;
            glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
            glUseProgram(prog);

            _uniformDestination = glGetUniformLocation(prog, _designator.c_str());
            glUniform1i(_uniformDestination, _index);
        }

        glActiveTexture(GL_TEXTURE0 + _index);

        if (_texture == nullptr)
        {
            glBindTexture(static_cast<GLenum>(_type), 0);
            return;
        }

        auto cpuID = _texture->_id;  
        if (cpuID < 0)
        {
            glBindTexture(static_cast<GLenum>(_type), 0);
            return;
        }

        auto it = _textureObjects.find(cpuID);
        if (it == _textureObjects.end())
        {
            auto newGPUTexture = std::make_unique<spk::OpenGL::TextureObject>();
            it = _textureObjects.insert({ cpuID, std::move(newGPUTexture) }).first;
        }

        auto& gpuTexture = *(it->second);

        if (_texture->_needUpdate)
        {
            gpuTexture.upload(_texture);

            _texture->_needUpdate = false;
        }

        else if (_texture->_needSettings)
        {
            gpuTexture.updateSettings(_texture);
            _texture->_needSettings = false;
        }

        glBindTexture(static_cast<GLenum>(_type), gpuTexture.id());
    }

    void SamplerObject::deactivate()
    {
        glBindTexture(static_cast<GLenum>(_type), 0);
    }
}
