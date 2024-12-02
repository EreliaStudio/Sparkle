#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <map>
#include <string>
#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/graphics/opengl/spk_texture_object.hpp"
#include "structure/graphics/spk_viewport.hpp"
#include "structure/spk_safe_pointer.hpp"
#include <cstdint>

namespace spk::OpenGL
{
	/**
	 * @class FrameBufferObject
	 * @brief Manages an OpenGL Framebuffer Object (FBO) for rendering to textures or other outputs.
	 * 
	 * The FrameBufferObject allows creating and managing framebuffer objects with attachments
	 * for off-screen rendering. It supports multiple color attachments with customizable formats.
	 * 
	 * Example usage:
	 * @code
	 * spk::OpenGL::FrameBufferObject::Factory factory;
	 * factory.addAttachment(L"Color", 0, spk::OpenGL::FrameBufferObject::Type::Float4);
	 * spk::OpenGL::FrameBufferObject fbo = factory.construct({800, 600});
	 * fbo.activate();
	 * // Perform rendering...
	 * fbo.deactivate();
	 * spk::OpenGL::TextureObject texture = fbo.saveAsTexture(L"Color");
	 * @endcode
	 */
	class FrameBufferObject
	{
	public:
		/**
		 * @enum Type
		 * @brief Specifies the type of data stored in a framebuffer attachment.
		 */
		enum class Type
		{
			Float4, ///< Four-component floating-point format.
			Float3, ///< Three-component floating-point format.
			Float2, ///< Two-component floating-point format.
			Float,  ///< Single-component floating-point format.
			Int4,   ///< Four-component signed integer format.
			Int3,   ///< Three-component signed integer format.
			Int2,   ///< Two-component signed integer format.
			Int,	///< Single-component signed integer format.
			UInt4,  ///< Four-component unsigned integer format.
			UInt3,  ///< Three-component unsigned integer format.
			UInt2,  ///< Two-component unsigned integer format.
			UInt	///< Single-component unsigned integer format.
		};

		/**
		 * @class Factory
		 * @brief Facilitates the creation of FrameBufferObject instances.
		 * 
		 * The Factory class allows defining attachments and constructing a FrameBufferObject.
		 */
		class Factory
		{
			friend class FrameBufferObject;
			friend class ShaderParser;

		public:
			/**
			 * @brief Constructs an empty Factory object.
			 */
			Factory();

			/**
			 * @brief Adds an attachment specification to the framebuffer.
			 * @param p_name The name of the attachment.
			 * @param p_colorAttachmentIndex The index of the color attachment.
			 * @param p_type The type of data for the attachment.
			 */
			void addAttachment(const std::wstring& p_name, int p_colorAttachmentIndex, Type p_type);

			/**
			 * @brief Constructs a FrameBufferObject based on the specified attachments.
			 * @param p_size The size of the framebuffer.
			 * @return A new FrameBufferObject instance.
			 */
			FrameBufferObject construct(const spk::Vector2UInt& p_size) const;

		private:
			/**
			 * @struct AttachmentSpec
			 * @brief Specifies an attachment for the framebuffer.
			 */
			struct AttachmentSpec
			{
				int colorAttachmentIndex; ///< Index of the color attachment.
				Type type; ///< Type of data for the attachment.
			};

			std::map<std::wstring, AttachmentSpec> _attachments; ///< Map of attachment specifications.
		};

	private:
		/**
		 * @brief Constructs a FrameBufferObject with the specified attachments and size.
		 * @param p_attachments The attachments for the framebuffer.
		 * @param p_size The size of the framebuffer.
		 */
		FrameBufferObject(const std::map<std::wstring, Factory::AttachmentSpec>& p_attachments, const spk::Vector2UInt& p_size);

		/**
		 * @brief Loads the framebuffer object and its attachments.
		 */
		void _load();

		/**
		 * @brief Releases the resources associated with the framebuffer.
		 */
		void _releaseResources();

		GLuint _framebufferID = 0; ///< OpenGL ID of the framebuffer.
		spk::Vector2UInt _size; ///< Size of the framebuffer.
		Viewport _viewport; ///< Viewport associated with the framebuffer.

		/**
		 * @struct Attachment
		 * @brief Represents an attachment to the framebuffer.
		 */
		struct Attachment
		{
			int colorAttachmentIndex; ///< Index of the color attachment.
			Type type; ///< Type of data for the attachment.
			TextureObject textureObject; ///< Texture associated with the attachment.
		};
		std::map<std::wstring, Attachment> _attachments; ///< Map of attachments.

	public:
		/**
		 * @brief Default constructor.
		 */
		FrameBufferObject() = default;

		/**
		 * @brief Copy constructor.
		 * @param p_other The FrameBufferObject to copy from.
		 */
		FrameBufferObject(const FrameBufferObject& p_other);

		/**
		 * @brief Copy assignment operator.
		 * @param p_other The FrameBufferObject to assign from.
		 * @return A reference to this object.
		 */
		FrameBufferObject& operator=(const FrameBufferObject& p_other);

		/**
		 * @brief Move constructor.
		 * @param p_other The FrameBufferObject to move from.
		 */
		FrameBufferObject(FrameBufferObject&& p_other) noexcept;

		/**
		 * @brief Move assignment operator.
		 * @param p_other The FrameBufferObject to assign from.
		 * @return A reference to this object.
		 */
		FrameBufferObject& operator=(FrameBufferObject&& p_other) noexcept;

		/**
		 * @brief Destructor. Releases resources associated with the framebuffer.
		 */
		~FrameBufferObject();

		/**
		 * @brief Resizes the framebuffer to the specified size.
		 * @param p_size The new size of the framebuffer.
		 */
		void resize(const spk::Vector2UInt& p_size);

		/**
		 * @brief Activates the framebuffer for rendering.
		 */
		void activate();

		/**
		 * @brief Deactivates the framebuffer, restoring default framebuffer.
		 */
		void deactivate();

		/**
		 * @brief Retrieves a pointer to the texture bound to a specific attachment.
		 * @param p_name The name of the attachment.
		 * @return A safe pointer to the TextureObject, or nullptr if not found.
		 */
		spk::SafePointer<TextureObject> bindedTexture(const std::wstring& p_name);

		/**
		 * @brief Saves the content of a specific attachment as a texture.
		 * @param p_name The name of the attachment.
		 * @return A TextureObject containing the attachment data.
		 */
		TextureObject saveAsTexture(const std::wstring& p_name);
	};
}
