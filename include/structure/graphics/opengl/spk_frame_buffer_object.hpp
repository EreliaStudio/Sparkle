#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <map>
#include <string>
#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/graphics/opengl/spk_texture_object.hpp"
#include "structure/graphics/spk_viewport.hpp"
#include <cstdint>

namespace spk::OpenGL
{
    class FrameBufferObject
    {
    public:
        enum class Type
        {
            Float4,
            Float3,
            Float2,
            Float,
            Int4,
            Int3,
            Int2,
            Int,
            UInt4,
            UInt3,
            UInt2,
            UInt
        };

    private:
        void _load();
        void _releaseResources();

        GLuint _framebufferID = 0;
        spk::Vector2UInt _size;
        Viewport _viewport;

        struct Attachment
        {
            int binding;
            Type type;
            TextureObject textureObject;
        };

        std::map<std::wstring, Attachment> _attachments;
    public:
        FrameBufferObject() = default;
        FrameBufferObject(const spk::Vector2UInt& p_size);
        FrameBufferObject(const FrameBufferObject& p_other);
        FrameBufferObject& operator=(const FrameBufferObject& p_other);
        FrameBufferObject(FrameBufferObject&& p_other) noexcept;
        FrameBufferObject& operator=(FrameBufferObject&& p_other) noexcept;
        ~FrameBufferObject();

        void addAttachment(const std::wstring& p_name, int p_binding, Type p_type);
        void resize(const spk::Vector2UInt& p_size);

        void activate();
        void deactivate();

        TextureObject* bindedTexture(const std::wstring& p_name);
        TextureObject saveAsTexture(const std::wstring& p_name);
    };
}
