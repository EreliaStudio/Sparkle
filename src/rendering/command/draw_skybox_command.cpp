#include "rendering/command/draw_skybox_command.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <utility>

#include "exception.hpp"
#include "graphics/internal/resource.hpp"
#include "graphics/opengl/state.hpp"
#include "graphics/opengl/uniform.hpp"

namespace spk
{
	namespace
	{
		struct FaceGeometry
		{
			Vector3 center;
			Vector3 right;
			Vector3 up;
		};

		constexpr std::array<FaceGeometry, 6> CubeFaces{{{{1, 0, 0}, {0, 0, -1}, {0, 1, 0}}, {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}}, {{0, 1, 0}, {1, 0, 0}, {0, 0, 1}}, {{0, -1, 0}, {1, 0, 0}, {0, 0, -1}}, {{0, 0, 1}, {-1, 0, 0}, {0, 1, 0}}, {{0, 0, -1}, {1, 0, 0}, {0, 1, 0}}}};

		void addFace(TextureMesh3D::Builder &builder, const FaceGeometry &face, Texture::Section section)
		{
			const auto at = [&section](float x, float y) {
				return section.anchor + Vector2{x * section.size.x, y * section.size.y};
			};
			const Vector3 normal = -face.center;
			const auto first = builder.addVertex({face.center - face.right + face.up, normal, at(0, 1)});
			(void)builder.addVertex({face.center - face.right - face.up, normal, at(0, 0)});
			(void)builder.addVertex({face.center + face.right - face.up, normal, at(1, 0)});
			(void)builder.addVertex({face.center + face.right + face.up, normal, at(1, 1)});
			const bool inward = face.right.cross(face.up).dot(normal) > 0.0f;
			if (inward)
			{
				builder.addTriangle(first, first + 1, first + 2);
				builder.addTriangle(first, first + 2, first + 3);
			}
			else
			{
				builder.addTriangle(first, first + 2, first + 1);
				builder.addTriangle(first, first + 3, first + 2);
			}
		}
	}

	Program &DrawSkyboxCommand::_sharedProgram()
	{
		static Program result = [] {
			Program program(
				std::string(resources::text("shaders/skybox.vert.glsl")),
				std::string(resources::text("shaders/skybox.frag.glsl")));
			program.bindSampler("uAlbedo", 0);
			return program;
		}();
		return result;
	}

	TextureMesh3D DrawSkyboxCommand::_buildMesh(const Sections &sections)
	{
		TextureMesh3D::Builder builder;
		builder.reserve(FaceCount * 4, FaceCount * 6);
		for (std::size_t index = 0; index < FaceCount; ++index)
		{
			addFace(builder, CubeFaces[index], sections[index]);
		}
		return std::move(builder).build();
	}

	DrawSkyboxCommand::DrawSkyboxCommand(
		Texture::Handle albedo,
		const Sections &sections,
		Matrix4x4 projection,
		Matrix4x4 view) :
		_mesh(_buildMesh(sections)),
		_program(_sharedProgram()),
		_albedo(0, Sampler::Filtering::Nearest, Sampler::Wrap::ClampToEdge, Sampler::MipmapFiltering::Disabled),
		_projection(projection),
		_viewRotation(view)
	{
		if (!albedo)
		{
			throw spk::Exception("Skybox draw requires a valid albedo");
		}
		_viewRotation[3][0] = 0.0f;
		_viewRotation[3][1] = 0.0f;
		_viewRotation[3][2] = 0.0f;
		_albedo.setTexture(std::move(albedo));
		_albedo.validate();
	}

	void DrawSkyboxCommand::execute(RenderContext &context) const
	{
		OpenGL::State::Scope restore(context);
		OpenGL::State::DepthTest{.enabled = false, .write = false}.apply(context);
		OpenGL::State::Rasterization{.cullEnabled = false}.apply(context);
		OpenGL::State::Blend::disabled(context).apply(context);
		_program.activate(context);
		Uniform<Matrix4x4>("uProjection", _projection).activate(context);
		Uniform<Matrix4x4>("uViewRotation", _viewRotation).activate(context);
		_albedo.activate(context);
		_mesh.layout().activate(context);
		_program.render(Program::Primitive::Triangles, _mesh.indexType(), 0, _mesh.indexCount());
	}
}
