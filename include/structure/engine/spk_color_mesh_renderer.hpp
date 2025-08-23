#pragma once

#include "structure/engine/spk_color_mesh.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_transform.hpp"
#include "structure/graphics/lumina/spk_shader.hpp"

namespace spk
{
	class ColorMeshRenderer : public spk::Component
	{
	private:
		class Painter
		{
		private:
			static spk::Lumina::Shader _createShader();
			static spk::Lumina::Shader _shader;

			spk::Lumina::Shader::Object _object;
			spk::OpenGL::BufferSet _bufferSet;
			spk::Lumina::Shader::UBO _transformUBO;

		public:
			Painter();

			void clear();
			void prepare(const spk::ColorMesh &p_mesh);
			void validate();
			void render();
			void setTransform(const spk::Transform &p_transform);
		};

		Painter _painter;
		spk::SafePointer<const spk::ColorMesh> _mesh;
		spk::Transform::Contract _onOwnerTransformEditionContract;

	public:
		ColorMeshRenderer(const std::wstring &p_name);

		void setMesh(const spk::SafePointer<const spk::ColorMesh> &p_mesh);
		const spk::SafePointer<const spk::ColorMesh> &mesh() const;

		void onPaintEvent(spk::PaintEvent &p_event) override;
		void start() override;
	};
}
