#include "sparkle.hpp"

#include <set>

#include "miscellaneous/spk_position_alignment.hpp"

class TextRenderer
{
private:
	static inline std::string _renderingPipelineCode = R"(#version 450

		#include <widgetAttribute>
		#include <screenConstants>

		Input -> Geometry : ivec2 modelPosition;
		Input -> Geometry : vec2 modelUVs;

		Geometry -> Render : vec2 fragmentUVs;

		AttributeBlock textRendererAttribute
		{
			vec4 textColor;
			vec4 outlineColor;
		};

		Texture fontTexture;

		void geometryPass()
		{
			pixelPosition = screenConstants.canvasMVP * vec4(modelPosition, widgetAttribute.layer, 1.0f);

			fragmentUVs = modelUVs;
		}

		float strongInterpolation(float x, float minRange, float maxRange, float exponent)
		{
			float adjustedX = (x - minRange) / (maxRange - minRange);
			float strongValue = pow(adjustedX, exponent);
			return strongValue;
		}

		void renderPass()
		{
			float distance = texture(fontTexture, fragmentUVs).r;

			if (distance <= 0.0f)
				discard;

			vec4 resultColor;
			if (distance >= 0.5)
			{
				float t = strongInterpolation(distance, 0.5, 1.0, 20.0);
				resultColor = mix(textRendererAttribute.outlineColor, textRendererAttribute.textColor, t);
			}
			else
			{
				float alpha = strongInterpolation(distance, 0.0, 0.5, 20.0);
				resultColor = vec4(textRendererAttribute.outlineColor.rgb, alpha);
			}

			pixelColor = resultColor;
			//pixelColor = vec4(distance, distance, distance, 1.0f);
		})";
	static inline spk::Pipeline _renderingPipeline = spk::Pipeline(_renderingPipelineCode);

	struct ShaderInput
	{
		spk::Vector2Int position;
		spk::Vector2 uvs;

		ShaderInput() : ShaderInput(0, 0){}

		ShaderInput(const spk::Vector2Int& p_position, const spk::Vector2& p_uvs) :
			position(p_position),
			uvs(p_uvs)
		{

		}
	};

	spk::Pipeline::Object _renderingObject;
	spk::Pipeline::Object::Attribute& _renderingObjectWidgetAttribute;
	spk::Pipeline::Object::Attribute::Element& _renderingObjectWidgetAttributeLayerElement;

	spk::Pipeline::Object::Attribute& _renderingObjectTextRendererAttribute;
	spk::Pipeline::Object::Attribute::Element& _renderingObjectTextRendererAttributeTextColorElement;
	spk::Pipeline::Object::Attribute::Element& _renderingObjectTextRendererAttributeOutlineColorElement;

	spk::Pipeline::Texture& _renderingPipelineTexture;

	bool _needGPUBufferUpdate = false;
	spk::Vector2Int _anchor;
	spk::Vector2Int _size;
	spk::VerticalAlignment _verticalAlignment = spk::VerticalAlignment::Centered;
	spk::HorizontalAlignment _horizontalAlignment = spk::HorizontalAlignment::Centered;

	std::string _text;

	size_t _textSize;
	spk::Color _textColor;
	size_t _outlineSize;
	spk::Color _outlineColor;

	spk::Font* _font;
	spk::Font::Atlas* _fontAtlas;
	
	struct RenderingData
	{
		std::vector<const spk::Font::Glyph*> glyphs;
		spk::Vector2Int anchorOffset = spk::Vector2Int(0, 0);
		spk::Vector2Int size = spk::Vector2Int(0, 0);
	};

	RenderingData _computeRenderingData(spk::Font::Atlas* p_fontAtlas, const std::string& p_text) const
	{
		RenderingData result;

		spk::Vector2Int topLeftCorner = 0;
		spk::Vector2Int downRightCorner = 0;

		for (size_t i = 0; i < p_text.size(); i++)
		{
			const spk::Font::Glyph& glyph = p_fontAtlas->glyph(p_text[i]);


			if (downRightCorner.y < glyph.positions[3].y)
				downRightCorner.y = glyph.positions[3].y;

			if (i == 0)
				topLeftCorner.x = glyph.positions[0].x;

			if (topLeftCorner.y > glyph.positions[0].y)
				topLeftCorner.y = glyph.positions[0].y;

			downRightCorner.x += glyph.positions[3].x;

			result.glyphs.push_back(&glyph);
		}

		result.anchorOffset = -topLeftCorner;
		result.size = downRightCorner - topLeftCorner;

		return (result);
	}

	spk::Vector2Int _computeBaseAnchor(const RenderingData& p_renderingData)
	{
		spk::Vector2Int result = 0;

		switch (_horizontalAlignment)
		{
		case spk::HorizontalAlignment::Left:
		{
			result.x = 0;
			break;
		}

		case spk::HorizontalAlignment::Centered:
		{
			result.x = (_size.x - p_renderingData.size.x) / 2;
			break;
		}

		case spk::HorizontalAlignment::Right:
		default:
		{
			result.x = _size.x - p_renderingData.size.x;
			break;
		}
		}

		switch (_verticalAlignment)
		{
		case spk::VerticalAlignment::Top:
		{
			result.y = 0;
			break;
		}

		case spk::VerticalAlignment::Centered:
		{
			result.y = (_size.y - p_renderingData.size.y) / 2;
			break;
		}

		case spk::VerticalAlignment::Down:
		default:
		{
			result.y = _size.y - p_renderingData.size.y;
			break;
		}
		}

		return (_anchor + result + p_renderingData.anchorOffset);
	}

	void _updateGPUData()
	{
		_fontAtlas = &(_font->atlas(_textSize, _outlineSize));

		spk::Vector2 deltas[4] = {
			spk::Vector2(0, 0),
			spk::Vector2(0, 1),
			spk::Vector2(1, 0),
			spk::Vector2(1, 1),
		};

		static std::vector<ShaderInput> _bufferShaderInput;
		static std::vector<unsigned int> _bufferRenderingPipelineIndexes;

		_bufferShaderInput.clear();
		_bufferRenderingPipelineIndexes.clear();

		RenderingData renderingData = _computeRenderingData(_fontAtlas, _text);

		std::cout << "Text size : " << renderingData.size << std::endl;

		spk::Vector2Int glyphAnchor = _computeBaseAnchor(renderingData);

		for (const spk::Font::Glyph* glyph : renderingData.glyphs)
		{
			unsigned int baseIndexes = static_cast<unsigned int>(_bufferShaderInput.size());

			for (size_t i = 0; i < 4; i++)
			{
				ShaderInput newVertex;

				newVertex.position = glyphAnchor + glyph->positions[i];
				newVertex.uvs = glyph->UVs[i];

				_bufferShaderInput.push_back(newVertex);
			}
			glyphAnchor += glyph->step;

			for (size_t i = 0; i < 6; i++)
			{
				_bufferRenderingPipelineIndexes.push_back(baseIndexes + spk::Font::Glyph::indexesOrder[i]);
			}
		}
		
		_renderingObject.setVertices(_bufferShaderInput);
		_renderingObject.setIndexes(_bufferRenderingPipelineIndexes);
	}

public:
	TextRenderer() :
		_renderingObject(_renderingPipeline.createObject()),
		_renderingObjectWidgetAttribute(_renderingObject.attribute("widgetAttribute")),
		_renderingObjectWidgetAttributeLayerElement(_renderingObjectWidgetAttribute["layer"]),
		_renderingObjectTextRendererAttribute(_renderingObject.attribute("textRendererAttribute")),
		_renderingObjectTextRendererAttributeTextColorElement(_renderingObjectTextRendererAttribute["textColor"]),
		_renderingObjectTextRendererAttributeOutlineColorElement(_renderingObjectTextRendererAttribute["outlineColor"]),
		_renderingPipelineTexture(_renderingPipeline.texture("fontTexture")),
		_font(nullptr)
	{

	}

	void render()
	{
		if (_font == nullptr)
			return ;

		if (_needGPUBufferUpdate == true)
		{
			_updateGPUData();
			_needGPUBufferUpdate = false;
		}

		_renderingPipelineTexture.attach(_fontAtlas);
		_renderingObject.render();
	}

	void setFont(spk::Font* p_font)
	{
		_font = p_font;
		_needGPUBufferUpdate = true;
	}

	void setText(const std::string& p_text)
	{
		_text = p_text;
		_needGPUBufferUpdate = true;
	}

	void setTextColor(const spk::Color& p_textColor)
	{
		_textColor;
		_renderingObjectTextRendererAttributeTextColorElement = p_textColor;
		_renderingObjectTextRendererAttribute.update();
	}

	void setTextSize(const size_t& p_textSize)
	{
		_textSize = p_textSize;
		_needGPUBufferUpdate = true;
	}

	void setOutlineColor(const spk::Color& p_outlineColor)
	{
		_outlineColor;
		_renderingObjectTextRendererAttributeOutlineColorElement = p_outlineColor;
		_renderingObjectTextRendererAttribute.update();
	}

	void setOutlineSize(const size_t& p_outlineSize)
	{
		_outlineSize = p_outlineSize;
		_needGPUBufferUpdate = true;
	}

	void setVerticalAlignment(const spk::VerticalAlignment& p_verticalAlignment)
	{
		_verticalAlignment = p_verticalAlignment;
		_needGPUBufferUpdate = true;
	}

	void setHorizontalAlignment(const spk::HorizontalAlignment& p_horizontalAlignment)
	{
		_horizontalAlignment = p_horizontalAlignment;
		_needGPUBufferUpdate = true;
	}

	const spk::Vector2& anchor() const
	{
		return _anchor;
	}

	const spk::Vector2& size() const
	{
		return _size;
	}

	size_t textSize() const
	{
		return (_textSize);
	}

	const spk::Color& textColor() const
	{
		return (_textColor);
	}

	size_t outlineSize() const
	{
		return (_outlineSize);
	}

	const spk::Color& outlineColor() const
	{
		return (_outlineColor);
	}

	const spk::VerticalAlignment& verticalAlignment() const
	{
		return (_verticalAlignment);
	}

	const spk::HorizontalAlignment& horizontalAlignment() const
	{
		return (_horizontalAlignment);
	}

	void setLayer(const float& p_layer)
	{
		_renderingObjectWidgetAttributeLayerElement = p_layer;
		_renderingObjectWidgetAttribute.update();
	}

	void setGeometry(const spk::Vector2& p_anchor, const spk::Vector2& p_newSize)
	{
		_anchor = p_anchor;
		_size = p_newSize;
		_needGPUBufferUpdate = true;
	}
};

class FontRequester : public spk::Widget
{
private:
	spk::Font* _font;
	TextRenderer _renderer;

	void _onUpdate() override
	{
		if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::F1) == spk::InputState::Pressed)
		{
			_renderer.setTextSize(20);
			_renderer.setOutlineSize(2);
			_renderer.setText("Ceci est un test a 20 / 2");
		}
		if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::F2) == spk::InputState::Pressed)
		{
			_renderer.setTextSize(30);
			_renderer.setOutlineSize(3);
			_renderer.setText("Ceci est un test a 30 / 3");
		}
		if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::F3) == spk::InputState::Pressed)
		{
			_renderer.setTextSize(40);
			_renderer.setOutlineSize(4);
			_renderer.setText("Ceci est un test a 40 / 4");
		}
		
		if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::F5) == spk::InputState::Pressed)
		{
			_renderer.setTextSize(20);
			_renderer.setOutlineSize(4);
			_renderer.setText("Ceci est un test a 20 / 4");
		}
		if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::F6) == spk::InputState::Pressed)
		{
			_renderer.setTextSize(30);
			_renderer.setOutlineSize(6);
			_renderer.setText("Ceci est un test a 30 / 6");
		}
		if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::F7) == spk::InputState::Pressed)
		{
			_renderer.setTextSize(40);
			_renderer.setOutlineSize(8);
			_renderer.setText("Ceci est un test a 40 / 8");
		}
	}

	void _onGeometryChange() override
	{
		_renderer.setGeometry(anchor(), size());
		_renderer.setLayer(layer());
	}

	void _onRender() override
	{
		_renderer.render();
	}

public:
	FontRequester(spk::Font* p_font, spk::Widget* p_parent) :
		FontRequester(p_font, "Unnamed FontRequester", p_parent)
	{
		
	}

	FontRequester(spk::Font* p_font, const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent),
		_font(p_font)
	{
		_renderer.setFont(_font);
		_renderer.setTextColor(spk::Color::blue);
		_renderer.setOutlineColor(spk::Color::red);
		_renderer.setTextSize(40);
		_renderer.setOutlineSize(10);
		_renderer.setText("Ceci est un test a 40 / 10");
	}
};

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(640, 640), spk::Application::Mode::Multithread);

	spk::Font font("Playground/resources/font/Roboto-Regular.ttf");

	FontRequester fontRequester(&font, "FontRequester", nullptr);
	fontRequester.setGeometry(0, app.size());
	fontRequester.activate();

    return (app.run());
}
