#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>
#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/graphics/spk_viewport.hpp"
#include "structure/graphics/texture/spk_texture.hpp"
#include "structure/graphics/opengl/spk_texture_object.hpp"
#include "structure/graphics/opengl/spk_texture_collection.hpp"
#include "structure/spk_safe_pointer.hpp"

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

        class Attachment
        {
        private:
            int _bindingPoint;
            Type _type;
            spk::Texture _bindedTexture;
            spk::OpenGL::TextureObject _bindedTextureObject;

            static void convertTypeToGL(const Type& p_type, GLint& internalFormat, GLenum& externalFormat, GLenum& glType)
            {
                switch (p_type)
                {
                case Type::Float4:
				{
					internalFormat = GL_RGBA32F;
					externalFormat = GL_RGBA;
					glType = GL_FLOAT;
					break;
				}
                case Type::Float3:
				{
					internalFormat = GL_RGB32F;
					externalFormat = GL_RGB;
					glType = GL_FLOAT;
					break;
				}
                case Type::Float2:
				{
					internalFormat = GL_RG32F;
					externalFormat = GL_RG;
					glType = GL_FLOAT;
					break;
				}
                case Type::Float:
				{
					internalFormat = GL_R32F;
					externalFormat = GL_RED;
					glType = GL_FLOAT;
					break;
				}
                case Type::Int4:
				{
					internalFormat = GL_RGBA32I;
					externalFormat = GL_RGBA_INTEGER;
					glType = GL_INT;
					break;
				}
                case Type::Int3:
				{
					internalFormat = GL_RGB32I;
					externalFormat = GL_RGB_INTEGER;
					glType = GL_INT;
					break;
				}
                case Type::Int2:
				{
					internalFormat = GL_RG32I;
					externalFormat = GL_RG_INTEGER;
					glType = GL_INT;
					break;
				}
                case Type::Int:
				{
					internalFormat = GL_R32I;
					externalFormat = GL_RED_INTEGER;
					glType = GL_INT;
					break;
				}
                case Type::UInt4:
				{
					internalFormat = GL_RGBA32UI;
					externalFormat = GL_RGBA_INTEGER;
					glType = GL_UNSIGNED_INT;
					break;
				}
                case Type::UInt3:
				{
					internalFormat = GL_RGB32UI;
					externalFormat = GL_RGB_INTEGER;
					glType = GL_UNSIGNED_INT;
					break;
				}
                case Type::UInt2:
				{
					internalFormat = GL_RG32UI;
					externalFormat = GL_RG_INTEGER;
					glType = GL_UNSIGNED_INT;
					break;
				}
                case Type::UInt:
				{
					internalFormat = GL_R32UI;
					externalFormat = GL_RED_INTEGER;
					glType = GL_UNSIGNED_INT;
					break;
				}
                default:
				{
					internalFormat = GL_RGBA8;
					externalFormat = GL_RGBA;
					glType = GL_UNSIGNED_BYTE;
					break;
				}
                }
            }

        public:
            Attachment(const int& bindingPoint, const Type& p_type) :
				_bindingPoint(bindingPoint),
				_type(p_type)
            {
            }

            void initialize(const spk::Vector2UInt& size, spk::Texture::Filtering filtering, spk::Texture::Wrap wrap, spk::Texture::Mipmap mipmap)
            {
                GLint internalFormat = GL_NONE; GLenum externalFormat = GL_NONE; GLenum glDataType = GL_NONE;
                convertTypeToGL(_type, internalFormat, externalFormat, glDataType);
                spk::Texture::Format spkFormat = spk::Texture::Format::RGBA;
                if (externalFormat == GL_RGB)
				{
					spkFormat = spk::Texture::Format::RGB;
				}
				if (externalFormat == GL_RG)
				{
					spkFormat = spk::Texture::Format::DualChannel;
				}
				if (externalFormat == GL_RED)
				{
					spkFormat = spk::Texture::Format::GreyLevel;
				}

				_bindedTexture.setPixels(nullptr, size, spkFormat, filtering, wrap, mipmap);

                glBindTexture(GL_TEXTURE_2D, _bindedTextureObject.id());
                glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.x, size.y, 0, externalFormat, glDataType, nullptr);
                _bindedTextureObject.updateSettings(&_bindedTexture);
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            void activate() const
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + _bindingPoint, GL_TEXTURE_2D, _bindedTextureObject.id(), 0);
            }

            spk::Texture save() const
            {
                const spk::Vector2UInt& sz = _bindedTexture.size();

                if (sz.x == 0 || sz.y == 0)
				{
					return spk::Texture{};
				}

                GLint internalFmt = GL_NONE; GLenum externalFmt = GL_NONE; GLenum glDataType = GL_NONE;
                convertTypeToGL(_type, internalFmt, externalFmt, glDataType);

                spk::Texture newTexture;
                spk::Texture::Format spkFormat = spk::Texture::Format::RGBA;

                if (externalFmt == GL_RGB)
				{
					spkFormat = spk::Texture::Format::RGB;
				}
                if (externalFmt == GL_RG) 
				{
					spkFormat = spk::Texture::Format::DualChannel;
				}
                if (externalFmt == GL_RED)
				{
					spkFormat = spk::Texture::Format::GreyLevel;
				}

                newTexture.setPixels(nullptr, sz, spkFormat, _bindedTexture.filtering(), _bindedTexture.wrap(), _bindedTexture.mipmap());
                glBindTexture(GL_TEXTURE_2D, _bindedTextureObject.id());
                size_t pixelCount = sz.x * sz.y; size_t bpp = 4;

                switch (spkFormat)
                {
					case spk::Texture::Format::GreyLevel:
					{
						bpp = 1;
						break;
					}
					case spk::Texture::Format::DualChannel:
					{
						bpp = 2;
						break;
					}
					case spk::Texture::Format::RGB:
					{
						bpp = 3;
						break;
					}
					case spk::Texture::Format::RGBA:
					{
						bpp = 4;
						break;
					}
					default:
					{
						bpp = 4;
						break;
					}
                }

                std::vector<uint8_t> readBackData(pixelCount * bpp, 0);
                glGetTexImage(GL_TEXTURE_2D, 0, externalFmt, glDataType, readBackData.data());
                glBindTexture(GL_TEXTURE_2D, 0);
                newTexture.setPixels(readBackData, sz, spkFormat, _bindedTexture.filtering(), _bindedTexture.wrap(), _bindedTexture.mipmap());
                return newTexture;
            }

            spk::SafePointer<const spk::Texture> bindedTexture() const
            {
                return &_bindedTexture;
            }
        };

    private:
        GLuint _framebufferID = 0;
        GLuint _depthBufferID = 0;
        spk::Vector2UInt _size;
        spk::Texture::Filtering _filtering = spk::Texture::Filtering::Nearest;
        spk::Texture::Wrap _wrap = spk::Texture::Wrap::ClampToEdge;
        spk::Texture::Mipmap _mipmap = spk::Texture::Mipmap::Disable;
        std::map<std::wstring, std::unique_ptr<Attachment>> _attachments;

        void _setupDepthBuffer()
        {
            if (_depthBufferID == 0)
			{
				glGenRenderbuffers(1, &_depthBufferID);
			}
			
			glBindRenderbuffer(GL_RENDERBUFFER, _depthBufferID);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, _size.x, _size.y);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, _framebufferID);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBufferID);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

    public:
        FrameBufferObject()
        {
            glGenFramebuffers(1, &_framebufferID);
        }

        FrameBufferObject(const spk::Vector2UInt& p_size) : _size(p_size)
        {
            glGenFramebuffers(1, &_framebufferID);
            _setupDepthBuffer();
        }

        FrameBufferObject(const FrameBufferObject&) = delete;
        FrameBufferObject& operator=(const FrameBufferObject&) = delete;

        FrameBufferObject(FrameBufferObject&& p_other) noexcept
        {
            _framebufferID = p_other._framebufferID; p_other._framebufferID = 0;
            _depthBufferID = p_other._depthBufferID; p_other._depthBufferID = 0;
            _size = p_other._size;
            _filtering = p_other._filtering;
            _wrap = p_other._wrap;
            _mipmap = p_other._mipmap;
            _attachments = std::move(p_other._attachments);
        }

        FrameBufferObject& operator=(FrameBufferObject&& p_other) noexcept
        {
            if (this != &p_other)
            {
                if (_framebufferID != 0)
				{
					glDeleteFramebuffers(1, &_framebufferID);
				}

				if (_depthBufferID != 0)
				{
					glDeleteRenderbuffers(1, &_depthBufferID);
				}
				
				_framebufferID = p_other._framebufferID;
                _depthBufferID = p_other._depthBufferID;
                _size = p_other._size;
                _filtering = p_other._filtering;
                _wrap = p_other._wrap;
                _mipmap = p_other._mipmap;
                _attachments = std::move(p_other._attachments);

				p_other._framebufferID = 0;
				p_other._depthBufferID = 0;
            }
            return *this;
        }

        ~FrameBufferObject()
        {
            if (_framebufferID != 0)
			{
				glDeleteFramebuffers(1, &_framebufferID);
			}
            if (_depthBufferID != 0)
			{
				glDeleteRenderbuffers(1, &_depthBufferID);
			}
        }

        void setProperties(spk::Texture::Filtering p_filtering, spk::Texture::Wrap p_wrap, spk::Texture::Mipmap p_mipmap)
        {
            _filtering = p_filtering;
			_wrap = p_wrap;
			_mipmap = p_mipmap;
        }

        void resize(const spk::Vector2UInt& p_size)
        {
            _size = p_size;
            _setupDepthBuffer();
            activate();
            for (auto& [name, attachmentPtr] : _attachments)
            {
                if (attachmentPtr)
                {
                    attachmentPtr->initialize(_size, _filtering, _wrap, _mipmap);
                    attachmentPtr->activate();
                }
            }
            if (!_attachments.empty())
            {
                std::vector<GLenum> drawBufs; drawBufs.reserve(_attachments.size());
                for (auto& [k, att] : _attachments)
                {
                    if (att) drawBufs.push_back(GL_COLOR_ATTACHMENT0 + 0);
                }
                if (!drawBufs.empty()) glDrawBuffers(GLsizei(drawBufs.size()), drawBufs.data()); else glDrawBuffer(GL_NONE);
            }
            else glDrawBuffer(GL_NONE);
            deactivate();
        }

        const spk::Vector2UInt& size() const { return _size; }
        spk::Texture::Filtering filtering() const { return _filtering; }
        spk::Texture::Wrap wrap() const { return _wrap; }
        spk::Texture::Mipmap mipmap() const { return _mipmap; }

        void addAttachment(const std::wstring& p_attachmentName, const int& bindingPoint, const Type& p_type)
        {
            auto newAttach = std::make_unique<Attachment>(bindingPoint, p_type);
            _attachments[p_attachmentName] = std::move(newAttach);
            if (_size.x > 0 && _size.y > 0)
            {
                activate();
                _attachments[p_attachmentName]->initialize(_size, _filtering, _wrap, _mipmap);
                _attachments[p_attachmentName]->activate();
                deactivate();
            }
        }

        spk::SafePointer<const Attachment> attachment(const std::wstring& p_attachmentName)
        {
            auto it = _attachments.find(p_attachmentName);
            if (it == _attachments.end()) return nullptr;
            return it->second.get();
        }

        void activate() { glBindFramebuffer(GL_FRAMEBUFFER, _framebufferID); }
        void deactivate() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
    };
}
