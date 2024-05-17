#pragma once

#include "graphics/pipeline/spk_pipeline.hpp"
#include "graphics/spk_color.hpp"
#include "graphics/texture/font/spk_font.hpp"
#include "math/spk_vector2.hpp"
#include <string>

namespace spk::WidgetComponent
{
	/**
	 * @class TextLabel
	 * @brief A component for rendering text within the UI, supporting various alignments, styles, and outlines.
	 *
	 * This class is a versatile UI component designed for displaying text within the SPK engine's widget system.
	 * It supports custom fonts, color, outlines, and alignment, making it suitable for a wide range of UI text rendering
	 * tasks. The component utilizes the SPK engine's rendering pipeline to efficiently render text using font atlases,
	 * ensuring high performance and quality.
	 *
	 * The class allows for detailed customization of text appearance, including font selection, size, color, outline style,
	 * and color. It provides alignment options both vertically (Top, Centered, Bottom) and horizontally (Left, Centered,
	 * Right), enabling precise control over text layout within its bounding box.
	 *
	 * Usage example:
	 * @code
	 * class MyCustomWidget : public spk::Widget
	 * {
	 * private:
	 *	 spkComponent::TextLabel _label;
	 *
	 *	 void _onGeometryChange()
	 *	 {
	 *		 _label.setAnchor(spk::Vector2Int(0, 0));
	 *	 }
	 *
	 *	 void _onRender() override
	 *	 {
	 *		 _label.render();
	 *	 }
	 *
	 *	 void _onUpdate()
	 *	 {
	 *
	 *	 }
	 *
	 * public:
	 *	 MyCustomWidget(const std::string& name, spk::Widget* parent = nullptr) :
	 *		 spk::Widget(name, parent),
	 *		 _label()
	 *	 {
	 *		 _label.setFont(myFont); // Assuming myFont is a preloaded Font instance
	 *		 _label.setText("Hello, SPK!");
	 *		 _label.setTextSize(24);
	 *		 _label.setTextColor(spk::Color::White);
	 *		 _label.setAnchor({10, 10});
	 *		 _label.setVerticalAlignment(spk::VerticalAlignment::Centered);
	 *		 _label.setHorizontalAlignment(spk::HorizontalAlignment::Centered);
	 *	 }
	 * };
	 * @endcode
	 *
	 * @see Font, Pipeline
	 */
	class TextLabel
	{
	private:
		struct ShaderInput
		{
			spk::Vector2Int position;
			spk::Vector2 uvs;

			ShaderInput();
			ShaderInput(const spk::Vector2Int& p_position, const spk::Vector2& p_uvs);
		};

		struct RenderingData
		{
			std::vector<const spk::Font::Atlas::GlyphData*> glyphs;
			spk::Vector2Int anchorOffset = spk::Vector2Int(0, 0);
			spk::Vector2Int size = spk::Vector2Int(0, 0);
		};

		static const std::string _renderingPipelineCode;
		static spk::Pipeline _renderingPipeline;

		spk::Pipeline::Object _renderingObject;
		spk::Pipeline::Object::Attribute& _renderingObjectWidgetAttribute;
		spk::Pipeline::Object::Attribute::Element& _layerAttributeElement;

		spk::Pipeline::Object::Attribute& _renderingObjectTextRendererAttribute;
		spk::Pipeline::Object::Attribute::Element& _textColorAttributeElement;
		spk::Pipeline::Object::Attribute::Element& _outlineColorAttributeElement;

		spk::Pipeline::Texture& _renderingPipelineTexture;

		bool _needGPUBufferUpdate = true;

		spk::Font* _font = nullptr;
		spk::Font::Atlas* _fontAtlas = nullptr;

		std::string _text = "";

		size_t _textSize = 1;
		spk::Color _textColor = spk::Color(0, 0, 0);

		spk::Font::OutlineStyle _outlineStyle = spk::Font::OutlineStyle::Standard;
		size_t _outlineSize = 0;
		spk::Color _outlineColor = spk::Color(255, 255, 255);

		spk::Vector2Int _anchor;
		spk::Vector2Int _size;

		VerticalAlignment _verticalAlignment = VerticalAlignment::Centered;
		HorizontalAlignment _horizontalAlignment = HorizontalAlignment::Centered;

		TextLabel::RenderingData _computeRenderingData(const spk::Font::Atlas* p_fontAtlas, const std::string& p_text) const;
		spk::Vector2Int _computeBaseAnchor(const TextLabel::RenderingData& p_renderingData);

		void _updateGPUBuffer();

	public:
		/**
		 * @brief Default constructor for TextLabel, initializing default values for text rendering.
		 */
		TextLabel();

		/**
		 * @brief Renders the text label according to its configured properties.
		 *
		 * This method prepares the text for rendering by computing the rendering data, updating the GPU buffer if necessary,
		 * and finally executing the rendering pipeline to display the text on the screen. It accounts for the text's font,
		 * size, color, alignment, and outline properties.
		 */
		void render();

		/**
		 * @brief Compute the biggest font size possible to allow the text stored inside the TextLabel to be rendered inside the area described by p_desiredArea
		 * @param p_desiredArea The area where the text need to fit in
		 * @return The font size in pixel
		*/
		size_t computeOptimalTextSize(const spk::Vector2Int& p_desiredArea);

		/**
		 * @brief Sets the font used for rendering the text.
		 *
		 * @param p_font Pointer to a Font instance to be used for rendering the text label.
		 */
		void setFont(spk::Font* p_font);

		/**
		 * @brief Sets the geometry of the text label.
		 *
		 * This method configures the anchor point and size of the TextLabel component within its parent container. The anchor point determines where the text label starts within the container, and the size dictates the dimensions within which the text will be rendered. This is crucial for proper placement and scaling of text in diverse UI layouts.
		 *
		 * Setting the geometry correctly is essential for the text to be rendered within the expected area, especially when dealing with responsive or dynamic layouts that may change based on different conditions (such as window resizing or orientation changes).
		 *
		 * @param p_anchor The position of the text label's upper-left corner within its parent container, specified as a `spk::Vector2Int`. This defines the starting point for rendering the text.
		 * @param p_size The size of the area allocated for the text label, specified as a `spk::Vector2Int`. This defines the width and height of the text label within which the text content will be fitted.
		 */
		void setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2Int& p_size);

		/**
		 * @brief Sets the layer of the text label in the rendering pipeline.
		 *
		 * @param p_layer A float value representing the layer.
		 */
		void setLayer(const float& p_layer);

		/**
		 * @brief Sets the text content of the label.
		 *
		 * @param p_text The text string to be rendered.
		 */
		void setText(const std::string& p_text);

		/**
		 * @brief Sets the size of the text.
		 *
		 * @param p_textSize The size of the text in points.
		 */
		void setTextSize(const size_t& p_textSize);

		/**
		 * @brief Sets the color of the text.
		 *
		 * @param p_textColor A Color object representing the text color.
		 */
		void setTextColor(const spk::Color& p_textColor);

		/**
		 * @brief Sets the outline style of the text.
		 *
		 * @param p_outlineStyle The outline style to be used.
		 */
		void setOutlineStyle(const spk::Font::OutlineStyle& p_outlineStyle);

		/**
		 * @brief Sets the size of the text outline.
		 *
		 * @param p_outlineSize The size of the outline in pixels.
		 */
		void setOutlineSize(const size_t& p_outlineSize);

		/**
		 * @brief Sets the color of the text outline.
		 *
		 * @param p_outlineColor A Color object representing the outline color.
		 */
		void setOutlineColor(const spk::Color& p_outlineColor);

		/**
		 * @brief Sets the vertical alignment of the text within its bounding box.
		 *
		 * @param p_verticalAlignment The vertical alignment mode.
		 */
		void setVerticalAlignment(const VerticalAlignment& p_verticalAlignment);

		/**
		 * @brief Sets the horizontal alignment of the text within its bounding box.
		 *
		 * @param p_horizontalAlignment The horizontal alignment mode.
		 */
		void setHorizontalAlignment(const HorizontalAlignment& p_horizontalAlignment);

		/**
		 * @brief Get the current label rendered text.
		 * @return The current text of the label
		*/
		const std::string& text() const;

		/**
		 * @brief Gets the font used for rendering the text.
		 * @return Pointer to the Font instance currently being used.
		 */
		spk::Font* font() const;

		/**
		 * @brief Gets the size of the text.
		 *
		 * @return The size of the text in points.
		 */
		size_t textSize() const;

		/**
		 * @brief Gets the color of the text.
		 *
		 * @return The current text color.
		 */
		const spk::Color& textColor() const;

		/**
		 * @brief Gets the outline style of the text.
		 *
		 * @return The current outline style.
		 */
		const spk::Font::OutlineStyle& outlineStyle() const;

		/**
		 * @brief Gets the size of the text outline.
		 *
		 * @return The size of the outline in pixels.
		 */
		size_t outlineSize() const;

		/**
		 * @brief Gets the color of the text outline.
		 *
		 * @return The current outline color.
		 */
		const spk::Color& outlineColor() const;

		/**
		 * @brief Gets the vertical alignment of the text within its bounding box.
		 *
		 * @return The current vertical alignment mode.
		 */
		const VerticalAlignment& verticalAlignment() const;

		/**
		 * @brief Gets the horizontal alignment of the text within its bounding box.
		 *
		 * @return The current horizontal alignment mode.
		 */
		const HorizontalAlignment& horizontalAlignment() const;
	};
}