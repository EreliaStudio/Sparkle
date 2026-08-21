#include "image_render_command.hpp"
namespace spk
{
	TextureMesh2D ImageRenderCommand::_mesh(Texture::Section s, Rect2D r, float d)
	{
		const float l = static_cast<float>(r.x), t = static_cast<float>(r.y), rr = l + r.width, b = t + r.height;
		TextureMesh2D::Builder m;
		m.addShape({{l, t}, d, s.anchor}, {{l, b}, d, {s.anchor.x, s.anchor.y + s.size.y}}, {{rr, b}, d, s.anchor + s.size}, {{rr, t}, d, {s.anchor.x + s.size.x, s.anchor.y}});
		return std::move(m).build();
	}
	ImageRenderCommand::ImageRenderCommand(const Texture *t, Texture::Section s, Rect2D r, float d) :
		_command(t, _mesh(s, r, d))
	{
	}
	void ImageRenderCommand::execute(RenderContext &c) const
	{
		_command.execute(c);
	}
}
