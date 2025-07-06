#pragma once

#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <memory>

#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/spk_safe_pointer.hpp"
#include "structure/graphics/texture/spk_texture.hpp"
#include "structure/graphics/opengl/spk_texture_collection.hpp"

namespace spk::OpenGL
{
    class FrameBufferObject
    {
    public:
        class Attachment
        {
        public:
            enum class Type
            {
                Color,
                Depth,
                DepthStencil
            };

        private:
            friend class FrameBufferObject;

            std::wstring                                   _name;
            int                                            _bindingPoint;
            Type                                           _type;
            bool _needsResize   = true;
			spk::Vector2UInt _size {0, 0};

            spk::SafePointer<spk::Texture>                 _cpuTexture   = nullptr;
            spk::SafePointer<spk::OpenGL::TextureObject>   _gpuTexture   = nullptr;

            void _allocate(const spk::Vector2UInt& p_size);
            void _resize   (const spk::Vector2UInt& p_size);
            void _attach   (GLuint p_fboID) const;

        public:
            Attachment() = default;
            Attachment(const std::wstring& p_name, int p_binding, Type p_type);

            [[nodiscard]] spk::Texture                                 save()           const;
            [[nodiscard]] spk::SafePointer<const spk::Texture>         bindedTexture()  const { return _cpuTexture; }
        };

        FrameBufferObject();
        ~FrameBufferObject();

        FrameBufferObject(const FrameBufferObject&)            = delete;
        FrameBufferObject& operator=(const FrameBufferObject&) = delete;
        FrameBufferObject(FrameBufferObject&&  p_other) noexcept;
        FrameBufferObject& operator=(FrameBufferObject&& p_other) noexcept;

        void addAttachment(const std::wstring& p_attachmentName,
                           const int&          p_bindingPoint,
                           const Attachment::Type& p_type);

        void resize(const spk::Vector2UInt& p_size);

        void activate();
        void deactivate();

        [[nodiscard]] spk::SafePointer<const Attachment> attachment(const std::wstring& p_attachmentName) const;

        [[nodiscard]] const spk::Vector2UInt& size()   const { return _size; }
        [[nodiscard]] bool                     ready()  const { return _validated; }

    private:
        GLuint                                   _id          = 0;
        bool                                     _validated   = false;
        spk::Vector2UInt                         _size        = {0, 0};

        std::unordered_map<std::wstring, Attachment> _attachments;

        void _allocate();
        void _release();
        void _prepareAttachments();
    };
}