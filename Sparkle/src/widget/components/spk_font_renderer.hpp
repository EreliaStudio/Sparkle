#pragma once

#include <string>
#include <vector>
#include "graphics/pipeline/spk_pipeline.hpp"
#include "math/spk_vector2.hpp"
#include "miscellaneous/spk_position_alignment.hpp"
#include "graphics/spk_color.hpp"
#include "graphics/texture/font/spk_font.hpp"

namespace spk::WidgetComponent
{
	class FontRenderer
	{
	private:
		static std::string _renderingPipelineCode;
		static spk::Pipeline _renderingPipeline;

		struct ShaderInput
		{
			spk::Vector2Int position;
			spk::Vector2 uvs;

			ShaderInput();
			ShaderInput(const spk::Vector2Int& p_position, const spk::Vector2& p_uvs);
		};

		spk::Pipeline::Object _renderingObject;
		spk::Pipeline::Object::Attribute& _renderingObjectWidgetAttribute;
		spk::Pipeline::Object::Attribute::Element& _renderingObjectWidgetAttributeLayerElement;

		spk::Pipeline::Object::Attribute& _renderingObjectFontRendererAttribute;
		spk::Pipeline::Object::Attribute::Element& _renderingObjectFontRendererAttributeTextColorElement;
		spk::Pipeline::Object::Attribute::Element& _renderingObjectFontRendererAttributeTextEdgePowerElement;
		spk::Pipeline::Object::Attribute::Element& _renderingObjectFontRendererAttributeOutlineColorElement;
		spk::Pipeline::Object::Attribute::Element& _renderingObjectFontRendererAttributeOutlineEdgePowerElement;

		spk::Pipeline::Texture& _renderingPipelineTexture;

		bool _needGPUBufferUpdate;
		spk::Vector2Int _anchor;
		spk::Vector2Int _size;
		spk::VerticalAlignment _verticalAlignment;
		spk::HorizontalAlignment _horizontalAlignment;

		std::string _text;

		size_t _textSize;
		spk::Color _textColor;
		float _textEdgeStrenght;
		size_t _outlineSize;
		spk::Color _outlineColor;
		float _outlineEdgeStrenght;

		spk::Font* _font;
		spk::Font::Atlas* _fontAtlas;

		struct RenderingData
		{
			std::vector<const spk::Font::Glyph*> glyphs;
			spk::Vector2Int anchorOffset;
			spk::Vector2Int size;
		};

		RenderingData _computeRenderingData(spk::Font::Atlas* p_fontAtlas, const std::string& p_text) const;
		spk::Vector2Int _computeBaseAnchor(const RenderingData& p_renderingData);
		void _updateGPUData();

	public:
		FontRenderer();

		void render();

		void setFont(spk::Font* p_font);
		void setFontSize(const spk::Font::Size& p_fontSize);
		void setText(const std::string& p_text);
		void setTextColor(const spk::Color& p_textColor);
		void setTextSize(const size_t& p_textSize);
		void setTextEdgeStrenght(const float& p_textEdgeStrenght);
		void setOutlineColor(const spk::Color& p_outlineColor);
		void setOutlineSize(const size_t& p_outlineSize);
		void setOutlineEdgeStrenght(const float& p_outlineEdgeStrenght);
		void setVerticalAlignment(const spk::VerticalAlignment& p_verticalAlignment);
		void setHorizontalAlignment(const spk::HorizontalAlignment& p_horizontalAlignment);

		spk::Font* font() const;
		const std::string& text() const;
		const spk::Vector2Int& anchor() const;
		const spk::Vector2Int& size() const;
		size_t textSize() const;
		const spk::Color& textColor() const;
		const float& textEdgeStrenght() const;
		size_t outlineSize() const;
		const spk::Color& outlineColor() const;
		const float& outlineEdgeStrenght() const;
		const spk::VerticalAlignment& verticalAlignment() const;
		const spk::HorizontalAlignment& horizontalAlignment() const;

		void setLayer(const float& p_layer);
		void setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2Int& p_newSize);
	};
}
