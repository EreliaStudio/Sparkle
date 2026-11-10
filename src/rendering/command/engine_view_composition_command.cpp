#include "rendering/command/engine_view_composition_command.hpp"

#include <GL/glew.h>

#include <memory>
#include <string>
#include <utility>

#include "core/context/render_context.hpp"
#include "graphics/internal/resource.hpp"
#include "graphics/opengl/program.hpp"

namespace spk
{
	Program &EngineViewCompositionCommand::_sharedProgram()
	{
		static auto program = [] {
			auto result = std::make_unique<Program>(
				std::string(resources::text("shaders/draw_texture_mesh.vert.glsl")),
				std::string(resources::text("shaders/engine_view_composition.frag.glsl")));
			result->bindUniformBlock("ViewportData", ViewportUniformRenderCommand::MatrixUBOBindingPoint);
			result->bindSampler("uOpaque", 0);
			result->bindSampler("uAccumulation", 1);
			result->bindSampler("uRevealage", 2);
			result->validate();
			return result;
		}();
		return *program;
	}

	TextureMesh2D EngineViewCompositionCommand::_quad(Vector2UInt resolution)
	{
		const float width = static_cast<float>(resolution.x);
		const float height = static_cast<float>(resolution.y);
		TextureMesh2D::Builder builder;
		builder.addShape(
			{{0.0f, 0.0f}, 0.0f, {0.0f, 1.0f}},
			{{0.0f, height}, 0.0f, {0.0f, 0.0f}},
			{{width, height}, 0.0f, {1.0f, 0.0f}},
			{{width, 0.0f}, 0.0f, {1.0f, 1.0f}});
		return std::move(builder).build();
	}

	EngineViewCompositionCommand::EngineViewCompositionCommand(
		Texture::Handle opaque,
		Texture::Handle accumulation,
		Texture::Handle revealage,
		Vector2UInt resolution,
		float exposure) :
		_mesh(_quad(resolution)),
		_opaque(0),
		_accumulation(1),
		_revealage(2),
		_viewport({.anchor = {0, 0}, .size = resolution}),
		_exposure(exposure)
	{
		_opaque.setTexture(std::move(opaque));
		_accumulation.setTexture(std::move(accumulation));
		_revealage.setTexture(std::move(revealage));
		_opaque.validate();
		_accumulation.validate();
		_revealage.validate();
	}

	void EngineViewCompositionCommand::execute(RenderContext &context) const
	{
		if (context.targetSurface == nullptr || context.targetSurface->lifeCycle() != Window::LifeCycle::Ready)
		{
			return;
		}
		_viewport.execute(context);
		auto &program = _sharedProgram();
		program.activate(context);
		_opaque.activate(context);
		_accumulation.activate(context);
		_revealage.activate(context);
		GLint programID = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &programID);
		glUniform1f(glGetUniformLocation(static_cast<GLuint>(programID), "uExposure"), _exposure);
		_mesh.layout().activate(context);
		program.render(Program::Primitive::Triangles, _mesh.indexType(), 0, _mesh.indexCount());
	}
}
