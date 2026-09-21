#include "rendering/command/image_render_command.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace spk
{
	Texture::Section ImageRenderCommand::_texelCenteredSection(
		Texture::Section section,
		const Vector2UInt &textureSize)
	{
		if (section == Texture::Section::whole || textureSize.x == 0 || textureSize.y == 0)
		{
			return section;
		}
		const auto correction = [](float extent, unsigned int dimension) {
			const float halfTexel = 0.5f / static_cast<float>(dimension);
			const float distance = std::min(halfTexel, std::abs(extent) * 0.5f);
			return std::copysign(distance, extent);
		};
		const Vector2 inset{
			correction(section.size.x, textureSize.x),
			correction(section.size.y, textureSize.y)};
		section.anchor += inset;
		section.size -= inset * Vector2{2.0f, 2.0f};
		return section;
	}

	TextureMesh2D ImageRenderCommand::_mesh(
		Texture::Section section,
		Rect2D rect,
		float depth,
		const Vector2UInt &textureSize)
	{
		section = _texelCenteredSection(section, textureSize);
		const float left = static_cast<float>(rect.x);
		const float top = static_cast<float>(rect.y);
		const float right = left + rect.width;
		const float bottom = top + rect.height;
		TextureMesh2D::Builder builder;
		builder.addShape(
			{{left, top}, depth, section.anchor},
			{{left, bottom}, depth, {section.anchor.x, section.anchor.y + section.size.y}},
			{{right, bottom}, depth, section.anchor + section.size},
			{{right, top}, depth, {section.anchor.x + section.size.x, section.anchor.y}});
		return std::move(builder).build();
	}

	DrawTextureMeshRenderCommand ImageRenderCommand::_resolve(
		const Texture *texture,
		Texture::Section section,
		Rect2D rect,
		float depth)
	{
		const auto resolution = texture
			? texture->resolve(section, rect.size)
			: Texture::Resolution{{}, rect.size};
		rect.anchor += Vector2Int{
			static_cast<int>((rect.width - resolution.size.x) / 2),
			static_cast<int>((rect.height - resolution.size.y) / 2)};
		rect.size = resolution.size;
		const Vector2UInt textureSize = resolution.texture
			? resolution.texture->size()
			: Vector2UInt{};
		return DrawTextureMeshRenderCommand(
			resolution.texture,
			_mesh(section, rect, depth, textureSize));
	}

	ImageRenderCommand::ImageRenderCommand(
		Texture::Handle texture,
		Texture::Section section,
		Rect2D rect,
		float depth) :
		_command(
			texture,
			_mesh(section, rect, depth, texture ? texture->size() : Vector2UInt{}))
	{
	}

	ImageRenderCommand::ImageRenderCommand(
		const Texture *texture,
		Texture::Section section,
		Rect2D rect,
		float depth) :
		_command(_resolve(texture, section, rect, depth))
	{
	}

	void ImageRenderCommand::execute(RenderContext &context) const
	{
		_command.execute(context);
	}
}
