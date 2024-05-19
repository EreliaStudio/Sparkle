#include "sparkle.hpp"

#include <set>

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

	bool _needGPUInputUpdate = false;
	spk::Vector2Int _anchor;
	spk::Vector2Int _size;

	size_t _textSize;
	size_t _outlineSize;

	spk::Font* _font;
	spk::Font::Atlas* _fontAtlas;

	void _updateGPUData()
	{
		_fontAtlas = &(_font->atlas(_textSize, _outlineSize));
		_fontAtlas->loadGlyphs(L"abcdefghijkl");

		std::vector<ShaderInput> data;
		std::vector<unsigned int> indexes = {0, 1, 2, 2, 1, 3};

		spk::Vector2 deltas[4] = {
			spk::Vector2(0, 0),
			spk::Vector2(0, 1),
			spk::Vector2(1, 0),
			spk::Vector2(1, 1),
		};

		for (size_t i = 0; i < 4; i++)
		{
			data.push_back(ShaderInput(_anchor + _size * deltas[i], deltas[i] / 10));
		}

		_renderingObject.setVertices(data);
		_renderingObject.setIndexes(indexes);
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

		if (_needGPUInputUpdate == true)
		{
			_updateGPUData();
			_needGPUInputUpdate = false;
		}

		_renderingPipelineTexture.attach(_fontAtlas);
		_renderingObject.render();
	}

	void setFont(spk::Font* p_font)
	{
		_font = p_font;
		_needGPUInputUpdate = true;
	}

	void setTextColor(const spk::Color& p_textColor)
	{
		_renderingObjectTextRendererAttributeTextColorElement = p_textColor;
		_renderingObjectTextRendererAttribute.update();
	}

	void setTextSize(const size_t& p_textSize)
	{
		_textSize = p_textSize;
		_needGPUInputUpdate = true;
	}

	void setOutlineColor(const spk::Color& p_outlineColor)
	{
		_renderingObjectTextRendererAttributeOutlineColorElement = p_outlineColor;
		_renderingObjectTextRendererAttribute.update();
	}

	void setOutlineSize(const size_t& p_outlineSize)
	{
		_outlineSize = p_outlineSize;
		_needGPUInputUpdate = true;
	}

	const spk::Vector2& anchor() const
	{
		return _anchor;
	}

	const spk::Vector2& size() const
	{
		return _size;
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
		_needGPUInputUpdate = true;
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
		}
		if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::F2) == spk::InputState::Pressed)
		{
			_renderer.setTextSize(30);
			_renderer.setOutlineSize(3);
		}
		if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::F3) == spk::InputState::Pressed)
		{
			_renderer.setTextSize(40);
			_renderer.setOutlineSize(4);
		}
		
		if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::F5) == spk::InputState::Pressed)
		{
			_renderer.setTextSize(20);
			_renderer.setOutlineSize(4);
		}
		if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::F6) == spk::InputState::Pressed)
		{
			_renderer.setTextSize(30);
			_renderer.setOutlineSize(6);
		}
		if (spk::Application::activeApplication()->keyboard().getKey(spk::Keyboard::F7) == spk::InputState::Pressed)
		{
			_renderer.setTextSize(40);
			_renderer.setOutlineSize(8);
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
