#pragma once

#include "structure/engine/spk_obj_mesh.hpp"
#include "structure/graphics/lumina/spk_shader.hpp"
#include "structure/graphics/lumina/spk_shader_object_factory.hpp"
#include "structure/graphics/texture/spk_image.hpp"
#include "structure/spk_safe_pointer.hpp"

namespace spk
{
	class ObjMeshRenderer
	{
	public:
		class Painter
		{
		private:
			static inline spk::Lumina::ShaderObjectFactory::Instanciator _instanciator;

			static spk::Lumina::Shader _createShader();
			static inline spk::Lumina::Shader _shader = _createShader();

			spk::Lumina::Shader::Object _object;
			spk::OpenGL::BufferSet &_bufferSet;
			spk::OpenGL::SamplerObject &_diffuseSampler;

		public:
			Painter();

			void clear();
			void prepare(const ObjMesh &p_mesh);
			void validate();
			void render();

			spk::Lumina::Shader &shader();
			spk::OpenGL::SamplerObject &diffuseSampler();
		};

	private:
		spk::SafePointer<spk::Image> _texture;
		Painter _painter;

	public:
		ObjMeshRenderer();

		void setTexture(spk::SafePointer<spk::Image> p_texture);
		spk::SafePointer<spk::Image> &texture();
		const spk::SafePointer<spk::Image> &texture() const;

		spk::Lumina::Shader &shader();
		Painter &painter();
	};
}
