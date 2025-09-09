#pragma once

#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_obj_mesh.hpp"
#include "structure/engine/spk_transform.hpp"
#include "structure/graphics/lumina/spk_shader.hpp"
#include "structure/graphics/lumina/spk_shader_object_factory.hpp"
#include "structure/spk_safe_pointer.hpp"

namespace spk
{
	class ObjMeshRenderer : public spk::Component
	{
	public:
		class Painter
		{
		private:
			static spk::Lumina::ShaderObjectFactory::Instanciator &_instanciator()
			{
				static spk::Lumina::ShaderObjectFactory::Instanciator instanciator;
				return instanciator;
			}

			static spk::Lumina::Shader _createShader();
			static spk::Lumina::Shader _shader;

			spk::Lumina::Shader::Object _object;
			spk::OpenGL::BufferSet &_bufferSet;
			std::shared_ptr<spk::OpenGL::SamplerObject> _diffuseSampler;
			;
			std::shared_ptr<spk::OpenGL::UniformBufferObject> _transformUBO;

		public:
			Painter();

			void clear();
			void prepare(const ObjMesh &p_mesh);
			void validate();
			void render();

			void setTransform(const spk::Transform &p_transform);
			void setTexture(const spk::SafePointer<const spk::Texture> &p_texture);
			const spk::SafePointer<const spk::Texture> &texture() const;
		};

	private:
		spk::SafePointer<const spk::ObjMesh> _mesh;
		Painter _painter;
		spk::ObjMesh::MaterialChangeContract _onMeshMaterialChangeContract;
		spk::ContractProvider::Contract _onOwnerTransformEditionContract;

	public:
		ObjMeshRenderer(const std::wstring &p_name);

		void setTexture(spk::SafePointer<const spk::Texture> p_texture);
		const spk::SafePointer<const spk::Texture> &texture() const;

		void setMesh(const spk::SafePointer<const spk::ObjMesh> &p_mesh);
		const spk::SafePointer<const spk::ObjMesh> &mesh() const;

		void onPaintEvent(spk::PaintEvent &p_event) override;
		void start() override;
	};
}
