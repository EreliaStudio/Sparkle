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

        class Factory
        {
            friend class FrameBufferObject;
            friend class ShaderParser;

        public:
            Factory();

            void addAttachment(const std::wstring& p_name, int p_colorAttachmentIndex, Type p_type);
            FrameBufferObject construct(const spk::Vector2UInt& p_size) const;

        private:
            struct AttachmentSpec
            {
                int colorAttachmentIndex;
                Type type;
            };

            std::map<std::wstring, AttachmentSpec> _attachments;
        };

    private:
        FrameBufferObject(const std::map<std::wstring, Factory::AttachmentSpec>& p_attachments, const spk::Vector2UInt& p_size);

        void _load();
        void _releaseResources();

        GLuint _framebufferID = 0;
        spk::Vector2UInt _size;
        Viewport _viewport;

        struct Attachment
        {
            int colorAttachmentIndex;
            Type type;
            TextureObject textureObject;
        };
        std::map<std::wstring, Attachment> _attachments;

    public:
        FrameBufferObject() = default;
        FrameBufferObject(const FrameBufferObject& p_other);
        FrameBufferObject& operator=(const FrameBufferObject& p_other);

        FrameBufferObject(FrameBufferObject&& p_other) noexcept;
        FrameBufferObject& operator=(FrameBufferObject&& p_other) noexcept;

        ~FrameBufferObject();

        void resize(const spk::Vector2UInt& p_size);

        void activate();
        void deactivate();

        TextureObject* bindedTexture(const std::wstring& p_name);
        TextureObject saveAsTexture(const std::wstring& p_name);
    };
}
