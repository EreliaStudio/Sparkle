#pragma once

#include "structure/engine/spk_collision_mesh_2d.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_transform.hpp"
#include "structure/graphics/lumina/spk_shader.hpp"
#include "structure/graphics/lumina/spk_shader_object_factory.hpp"

namespace spk
{
	class CollisionMesh2DRenderer : public spk::Component
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
			std::shared_ptr<spk::OpenGL::UniformBufferObject> _transformUBO;

		public:
			Painter();

			void clear();
			void prepare(const spk::CollisionMesh2D &p_mesh);
			void validate();
			void render(bool p_wireframe);
			void setTransform(const spk::Transform &p_transform);
		};

		spk::SafePointer<const spk::CollisionMesh2D> _mesh;
		Painter _painter;
		spk::ContractProvider::Contract _onOwnerTransformEditionContract;
		bool _wireframe = true;

	public:
		CollisionMesh2DRenderer(const std::wstring &p_name);

		void setMesh(const spk::SafePointer<const spk::CollisionMesh2D> &p_mesh);
		const spk::SafePointer<const spk::CollisionMesh2D> &mesh() const;

		void setWireframe(bool p_wireframe);
		bool wireframe() const;

		void onPaintEvent(spk::PaintEvent &p_event) override;
		void start() override;
	};
}
