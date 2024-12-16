#include "widget/component/spk_texture_renderer.hpp"

namespace spk::WidgetComponent
{
	void TextureRenderer::_updateGPUData()
	{
		struct Vertex
		{
			spk::Vector2Int position;
			spk::Vector2 uvs;
		};

		std::vector<Vertex> vertices = {
			{
				.position = {_geometry.anchor.x, _geometry.anchor.y},
				.uvs = {_section.anchor.x, _section.anchor.y}
			},
			{
				.position = {_geometry.anchor.x, _geometry.anchor.y + _geometry.size.y},
				.uvs = {_section.anchor.x, _section.anchor.y + _section.size.y}
			},
			{
				.position = {_geometry.anchor.x + _geometry.size.x, _geometry.anchor.y},
				.uvs = {_section.anchor.x + _section.size.x, _section.anchor.y}
			},
			{
				.position = {_geometry.anchor.x + _geometry.size.x, _geometry.anchor.y + _geometry.size.y},
				.uvs = {_section.anchor.x + _section.size.x, _section.anchor.y + _section.size.y}
			}
		};

		std::vector<unsigned int> indexes = {
			0, 1, 2, 2, 1, 3
		};

		// _object.layout().clear();
		// _object.indexes().clear();

		// _object.layout() << vertices;
		// _object.indexes() << indexes;

		// _object.layout().validate();
		// _object.indexes().validate();
	}

	TextureRenderer::TextureRenderer()
	{
		// _layerElement = _layer;
		// _alphaElement = _alpha;

		// _textureInformations.validate();
	}

	const float& TextureRenderer::layer() const
	{
		return (_layer);
	}
	const float& TextureRenderer::alpha() const
	{
		return (_alpha);
	}
	const spk::Geometry2D& TextureRenderer::geometry() const
	{
		return (_geometry);
	}
	const spk::SafePointer<spk::Image>& TextureRenderer::texture() const
	{
		return (_texture);
	}
	const spk::Image::Section& TextureRenderer::section() const
	{
		return (_section);
	}

	void TextureRenderer::setLayer(const float& p_layer)
	{
		_layer = p_layer;

		// _layerElement = _layer;

		// _textureInformations.validate();
	}
	void TextureRenderer::setAlpha(const float& p_alpha)
	{
		_alpha = p_alpha;

		// _alphaElement = _alpha;

		// _textureInformations.validate();
	}
	void TextureRenderer::setGeometry(const spk::Geometry2D& p_geometry)
	{
		_geometry = p_geometry;
		_needUpdateGPU = true;
	}
	void TextureRenderer::setTexture(const spk::SafePointer<spk::Image>& p_texture)
	{
		_texture = p_texture;
		// _textureSampler2D.bind(_texture);
	}
	void TextureRenderer::setSection(const spk::Image::Section& p_section)
	{
		_section = p_section;
		_needUpdateGPU = true;
	}

	void TextureRenderer::render()
	{
		if (_needUpdateGPU == true)
		{
			_updateGPUData();
			_needUpdateGPU = false;
		}

		// _object.render();
	}
}