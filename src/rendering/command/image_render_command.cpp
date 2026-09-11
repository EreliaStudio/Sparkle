#include "rendering/command/image_render_command.hpp"
namespace spk
{
	TextureMesh2D ImageRenderCommand::_mesh(Texture::Section section, Rect2D rect, float depth)
	{
		const float left = static_cast<float>(rect.x), top = static_cast<float>(rect.y), right = left + rect.width, down = top + rect.height;
		TextureMesh2D::Builder meshBuilder;

		meshBuilder.addShape(
			{{left, top}, depth, section.anchor},
			{{left, down}, depth, {section.anchor.x, section.anchor.y + section.size.y}},
			{{right, down}, depth, section.anchor + section.size},
			{{right, top}, depth, {section.anchor.x + section.size.x, section.anchor.y}});

		return std::move(meshBuilder).build();
	}
	ImageRenderCommand::ImageRenderCommand(Texture::Handle texture, Texture::Section section, Rect2D rect, float depth) :
		_command(std::move(texture), _mesh(section, rect, depth))
	{
	}

	ImageRenderCommand::ImageRenderCommand(const Texture *texture, Texture::Section section, Rect2D rect, float depth) :
		ImageRenderCommand(texture ? texture->handle() : Texture::Handle{}, section, rect, depth)
	{
	}
	void ImageRenderCommand::execute(RenderContext &context) const
	{
		_command.execute(context);
	}
}
