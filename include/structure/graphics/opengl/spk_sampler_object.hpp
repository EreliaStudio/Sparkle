#pragma once

#include <string>
#include <stdexcept>
#include <GL/glew.h>
#include "structure/spk_safe_pointer.hpp"
#include "structure/graphics/texture/spk_texture.hpp"

namespace spk::OpenGL
{
	class SamplerObject
	{
	public:
		enum class Type : GLenum
		{
			Texture1D = GL_TEXTURE_1D,
			Texture2D = GL_TEXTURE_2D,
			Texture3D = GL_TEXTURE_3D,
			TextureCubeMap = GL_TEXTURE_CUBE_MAP
		};

	private:
		spk::SafePointer<const Texture> _texture = nullptr;

		std::string _designator;
		GLint _index = -1;
		GLint _uniformDestination = -1;
		Type _type;

	public:
		SamplerObject();
		SamplerObject(const std::string& p_name, Type p_type, size_t p_textureIndex);

		SamplerObject(const SamplerObject& p_other);
		SamplerObject& operator=(const SamplerObject& p_other);
		SamplerObject(SamplerObject&& p_other) noexcept;
		SamplerObject& operator=(SamplerObject&& p_other) noexcept;

		void bind(const spk::SafePointer<const Texture>& p_texture);

		spk::SafePointer<const Texture>& texture();
		const spk::SafePointer<const Texture>& texture() const;

		void activate();
		void deactivate();
	};
}
