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
		struct GLState
		{
			// Blend
			GLboolean blendEnabled;
			GLint blendSrcRGB;
			GLint blendDstRGB;
			GLint blendSrcAlpha;
			GLint blendDstAlpha;

			// Cull face
			GLboolean cullFaceEnabled;
			GLint cullFaceMode;
			GLint frontFaceMode;

			// Depth
			GLboolean depthTestEnabled;
			GLboolean depthMask;
			GLfloat clearDepth;
			GLint depthFunc;

			// Stencil
			GLboolean stencilTestEnabled;
			GLint stencilFunc;
			GLint stencilRef;
			GLuint stencilValueMask;
			GLint stencilFail;
			GLint stencilZFail;
			GLint stencilZPass;
			GLuint stencilWriteMask;

			// Scissor
			GLboolean scissorTestEnabled;

			void save();
			void load();
		};
		GLState _glState;


        void _load();
        void _releaseResources();

		bool _needReload = false;
        GLuint _framebufferID = 0;
		GLuint _depthBufferID = 0;
        spk::Vector2UInt _size;
        Viewport _viewport;
		spk::SafePointer<const Viewport> _previousViewport;

        struct Attachment
        {
            int binding;
            Type type;
            TextureObject textureObject;
        };

        std::map<std::wstring, Attachment> _attachments;
    public:
        FrameBufferObject();
        FrameBufferObject(const spk::Vector2UInt& p_size);
        FrameBufferObject(const FrameBufferObject& p_other);
        FrameBufferObject& operator=(const FrameBufferObject& p_other);
        FrameBufferObject(FrameBufferObject&& p_other) noexcept;
        FrameBufferObject& operator=(FrameBufferObject&& p_other) noexcept;
        ~FrameBufferObject();

        void addAttachment(const std::wstring& p_name, int p_binding, Type p_type);
        void resize(const spk::Vector2UInt& p_size);
		const spk::Vector2UInt& size() const;

        void activate();
        void deactivate();

        TextureObject* bindedTexture(const std::wstring& p_name);
        TextureObject saveAsTexture(const std::wstring& p_name);

		void saveAsPNG(const std::wstring& p_attachmentName, const std::string& p_filePath, bool flipVertically = true);
    };
}
