#include "rendering/command/nine_slice_render_command.hpp"

#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "graphics/internal/resource.hpp"
#include "graphics/opengl/program.hpp"
#include "graphics/opengl/uniform_buffer.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"

namespace spk
{
	const SpriteSheet &NineSliceRenderCommand::_sheet(const SpriteSheet *spriteSheet)
	{
		if (spriteSheet == nullptr || spriteSheet->nbSprite() != Vector2UInt{3, 3})
		{
			throw std::invalid_argument("NineSliceRenderCommand requires a 3x3 sprite sheet");
		}
		if (spriteSheet->size().x % 3 != 0 || spriteSheet->size().y % 3 != 0)
		{
			throw std::invalid_argument("NineSliceRenderCommand requires evenly divisible sprite cells");
		}
		return *spriteSheet;
	}

	TextureMesh2D NineSliceRenderCommand::_mesh(Rect2D rect, Vector2UInt corner, float depth)
	{
		if (corner.x > rect.width / 2 || corner.y > rect.height / 2)
		{
			throw std::invalid_argument("Nine-slice corners exceed half the destination size");
		}
		if (rect.width > std::numeric_limits<std::int32_t>::max() ||
			rect.height > std::numeric_limits<std::int32_t>::max())
		{
			throw std::invalid_argument("Nine-slice destination exceeds shader integer range");
		}
		const float left = static_cast<float>(rect.x);
		const float top = static_cast<float>(rect.y);
		const float right = left + rect.width;
		const float bottom = top + rect.height;
		TextureMesh2D::Builder builder;
		builder.addShape(
			{{left, top}, depth, {}},
			{{left, bottom}, depth, {}},
			{{right, bottom}, depth, {}},
			{{right, top}, depth, {}});
		return std::move(builder).build();
	}

	Program &NineSliceRenderCommand::_sharedProgram()
	{
		static auto program = [] {
			auto result = std::make_unique<Program>(
				std::string(resources::text("shaders/nine_slice.vert.glsl")),
				std::string(resources::text("shaders/nine_slice.frag.glsl")));
			result->bindUniformBlock("ViewportData", ViewportUniformRenderCommand::MatrixUBOBindingPoint);
			result->bindUniformBlock("NineSliceRenderData", RenderDataUBOBindingPoint);
			result->bindSampler("uTexture", TextureSamplerBindingPoint);
			result->validate();
			return result;
		}();
		return *program;
	}

	UniformBuffer &NineSliceRenderCommand::_sharedBuffer()
	{
		static UniformBuffer buffer(RenderDataUBOBindingPoint, sizeof(RenderData));
		return buffer;
	}

	NineSliceRenderCommand::NineSliceRenderCommand(
		const SpriteSheet *spriteSheet,
		Rect2D rect,
		Vector2UInt cornerSize,
		float depth) :
		_meshData(_mesh(rect, cornerSize, depth)),
		_data{
			.geometry = {
				rect.x,
				rect.y,
				static_cast<std::int32_t>(rect.width),
				static_cast<std::int32_t>(rect.height)},
			.corner = {cornerSize.x, cornerSize.y, 0, 0}},
		_sampler(TextureSamplerBindingPoint)
	{
		const SpriteSheet &sheet = _sheet(spriteSheet);
		_sampler.setTexture(sheet.handle());
		_sampler.validate();
	}

	void NineSliceRenderCommand::execute(RenderContext &context) const
	{
		if (_meshData.empty())
		{
			return;
		}
		auto &buffer = _sharedBuffer();
		buffer.setData(_data);
		buffer.validate();
		buffer.activate(context);
		auto &program = _sharedProgram();
		program.activate(context);
		_sampler.activate(context);
		_meshData.layout().activate(context);
		program.render(Program::Primitive::Triangles, _meshData.indexType(), 0, _meshData.indexCount());
	}
}
