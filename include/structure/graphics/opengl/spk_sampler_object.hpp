#pragma once

#include <string>
#include <stdexcept>
#include <GL/glew.h>
#include "structure/spk_safe_pointer.hpp"
#include "spk_texture_object.hpp"

namespace spk::OpenGL
{
	class SamplerObject
	{
	private:
		spk::SafePointer<TextureObject> _texture = nullptr;

		std::string _designator;
		GLint _index = -1;
		GLint _uniformDestination = -1;

	public:
		SamplerObject();

		SamplerObject(const SamplerObject& p_other);
		SamplerObject& operator=(const SamplerObject& p_other);
		SamplerObject(SamplerObject&& p_other) noexcept;
		SamplerObject& operator=(SamplerObject&& p_other) noexcept;

		void setDesignator(const std::string& designator);
		void setIndex(size_t index);
		void bind(const spk::SafePointer<TextureObject>& p_texture);

		void activate();
	};
}
