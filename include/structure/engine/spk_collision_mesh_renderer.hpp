#pragma once

#include "structure/engine/spk_collision_mesh.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_transform.hpp"
#include "structure/graphics/lumina/spk_shader.hpp"
#include "structure/graphics/lumina/spk_shader_object_factory.hpp"

namespace spk
{
	class CollisionMeshRenderer : public spk::Component
	{
	private:
		class Painter
		{
		private:
			static inline spk::Lumina::ShaderObjectFactory::Instanciator _instanciator;

			static spk::Lumina::Shader _createShader();
			static spk::Lumina::Shader _shader;

			spk::Lumina::Shader::Object _object;
			spk::OpenGL::BufferSet &_bufferSet;
			spk::OpenGL::UniformBufferObject &_transformUBO;

		public:
			Painter();

			void clear();
			void prepare(const spk::CollisionMesh &p_mesh);
			void validate();
			void render(bool p_wireframe);
			void setTransform(const spk::Transform &p_transform);
		};

		spk::SafePointer<const spk::CollisionMesh> _mesh;
		Painter _painter;
		spk::ContractProvider::Contract _onOwnerTransformEditionContract;
		bool _wireframe = true;

	public:
		CollisionMeshRenderer(const std::wstring &p_name);

		void setMesh(const spk::SafePointer<const spk::CollisionMesh> &p_mesh);
		const spk::SafePointer<const spk::CollisionMesh> &mesh() const;

		void setWireframe(bool p_wireframe);
		bool wireframe() const;

		void onPaintEvent(spk::PaintEvent &p_event) override;
		void start() override;
	};
}
