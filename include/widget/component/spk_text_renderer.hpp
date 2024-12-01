#pragma once

#include "structure/graphics/spk_pipeline.hpp"
#include "structure/spk_safe_pointer.hpp"
#include "structure/graphics/texture/spk_font.hpp"
#include "structure/graphics/spk_text_alignment.hpp"
#include "structure/graphics/spk_color.hpp"

namespace spk::WidgetComponent
{
	class TextRenderer
	{
	private:
		static std::string _pipelineCode;
		static spk::Pipeline _pipeline;

		spk::Pipeline::Object _object;
		spk::Pipeline::Object::Attribute& _textInformations;
		spk::Pipeline::Object::Attribute::Element& _layerElement;
		spk::Pipeline::Object::Attribute::Element& _outlineColorElement;
		spk::Pipeline::Object::Attribute::Element& _glyphColorElement;
		spk::Pipeline::Object::Sampler2D& _fontTextureSampler2D;

		bool _needUpdateGPUData = false;

		float _layer = 0.0f;
		spk::Geometry2D _geometry;
		spk::SafePointer<spk::Font> _font = nullptr;
		spk::Color _outlineColor = spk::Color::black;
		spk::Color _glyphColor = spk::Color::white;
		spk::Font::Size _fontSize = {26, 4};
		std::wstring _text = L"";
		HorizontalAlignment _horizontalAlignment = spk::HorizontalAlignment::Centered;
		VerticalAlignment _verticalAlignment = spk::VerticalAlignment::Centered;

		void _updateGPUData();

	public:
		TextRenderer();

		const float& layer() const;
		const spk::Geometry2D& geometry() const;
		const spk::SafePointer<spk::Font>& font() const;
		const std::wstring& text() const;
		const spk::Font::Size& fontSize() const;
		const HorizontalAlignment& horizontalAlignment() const;
		const VerticalAlignment& verticalAlignment() const;
		const spk::Color& outlineColor() const;
		const spk::Color& glyphColor() const;

		void setLayer(const float& p_layer);
		void setGeometry(const spk::Geometry2D& p_geometry);
		void setFont(const spk::SafePointer<spk::Font> p_font);
		void setFontSize(const spk::Font::Size& p_fontSize);
		void setText(const std::wstring& p_text);
		void setColors(const spk::Color& p_outlineColor, const spk::Color& p_glyphColor);
		void setOutlineColor(const spk::Color& p_outlineColor);
		void setGlyphColor(const spk::Color& p_glyphColor);
		void setTextHorizontalAlignment(const HorizontalAlignment& p_horizontalAlignment);
		void setTextVerticalAlignment(const VerticalAlignment& p_verticalAlignment);

		void render();
	};
}