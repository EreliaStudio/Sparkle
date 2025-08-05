#pragma once

#include "structure/engine/spk_mesh.hpp"
#include "structure/graphics/lumina/spk_shader.hpp"
#include "structure/graphics/lumina/spk_shader_object_factory.hpp"
#include "structure/graphics/texture/spk_image.hpp"
#include "structure/spk_safe_pointer.hpp"

namespace spk
{
	class MeshRenderer
	{
	public:
		class Painter
		{
		private:
			static spk::Lumina::ShaderObjectFactory::Instanciator _initializeObjectFactory();
			static inline spk::Lumina::ShaderObjectFactory::Instanciator _instanciator = _initializeObjectFactory();

			static spk::Lumina::Shader _createShader();
			static inline spk::Lumina::Shader _shader = _createShader();

			spk::Lumina::Shader::Object _object;
			spk::Lumina::Shader::BufferSet &_bufferSet;
			spk::Lumina::Shader::Sampler &_diffuseSampler;

		public:
			Painter();

			void clear();
			void prepare(const spk::Mesh &p_mesh);
			void validate();
			void render();

			spk::Lumina::Shader &shader();
			spk::Lumina::Shader::Sampler &diffuseSampler();
		};

	private:
		spk::SafePointer<spk::Image> _texture;
		Painter _painter;

	public:
		MeshRenderer();

		void setTexture(spk::SafePointer<spk::Image> p_texture);
		spk::SafePointer<spk::Image> &texture();
		const spk::SafePointer<spk::Image> &texture() const;

		spk::Lumina::Shader &shader();
		Painter &painter();
	};
}
