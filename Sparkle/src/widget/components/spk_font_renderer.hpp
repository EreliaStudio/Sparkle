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
	/**
	 * @class FontRenderer
	 * @brief Manages rendering of text using a specified font and properties.
	 *
	 * The FontRenderer class provides functionalities to render text with a given font, size, color, alignment, and other attributes. It handles GPU buffer updates, text layout calculations, and texture management for efficient text rendering.
	 * 
	 * Usage example:
	 * @code
	 * spk::FontRenderer fontRenderer;
	 * fontRenderer.setFont(&myFont);
	 * fontRenderer.setText("Hello World");
	 * fontRenderer.setTextColor(spk::Color(255, 255, 255, 255));
	 * fontRenderer.render();
	 * @endcode
	 *
	 * @note Ensure the font object provided is valid and loaded properly before setting it to the FontRenderer.
	 */
	class FontRenderer
	{
	private:
		static std::string _renderingPipelineCode;
		static spk::Pipeline _renderingPipeline;

		/**
		 * @struct ShaderInput
		 * @brief Represents the input data for the shader.
		 *
		 * This structure holds the position and UV coordinates for the shader input.
		 */
		struct ShaderInput
		{
			spk::Vector2Int position; ///< The position of the vertex.
			spk::Vector2 uvs; ///< The UV coordinates of the vertex.

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
		spk::Vector2Int _size; ///< The size of the rendered text.
		spk::VerticalAlignment _verticalAlignment;
		spk::HorizontalAlignment _horizontalAlignment;

		std::string _text;

		spk::Font::Size _fontSize;
		spk::Color _textColor;
		float _textEdgeStrenght;
		spk::Color _outlineColor;
		float _outlineEdgeStrenght;

		spk::Font* _font;
		spk::Font::Atlas* _fontAtlas;

		/**
		 * @struct RenderingData
		 * @brief Contains data necessary for rendering text.
		 *
		 * This structure holds the glyphs to render, the anchor offset, and the overall size of the rendered text.
		 */
		struct RenderingData
		{
			std::vector<const spk::Font::Glyph*> glyphs; //!< The glyphs to render.
			spk::Vector2Int anchorOffset; //!< The offset of the anchor.
			spk::Vector2Int size; //!< The overall size of the rendered text.
		};

		RenderingData _computeRenderingData(spk::Font::Atlas* p_fontAtlas, const std::string& p_text) const;
		spk::Vector2Int _computeBaseAnchor(const RenderingData& p_renderingData);
		void _updateGPUData();

	public:
		/**
		 * @brief Default constructor for the FontRenderer class.
		 */
		FontRenderer();

		/**
		 * Compute the optimized size of the text for a specific area and outline ratio.
		 * @param p_availibleArea The area where the text must stay inside
		 * @param p_outlineRatio The ratio between the outline size and the text size
		 * @return The resulting font size, containing information about text and outline size
		*/
		spk::Font::Size computeOptimalFontSize(const spk::Vector2Int& p_availibleArea, const float& p_outlineRatio);

		/**
		 * @brief Renders the text using the current settings.
		 */
		void render();

		/**
		 * @brief Sets the font to use for rendering text.
		 * @param p_font Pointer to the font object.
		 */
		void setFont(spk::Font* p_font);

		/**
		 * @brief Sets the size of the font.
		 * @param p_fontSize The size of the font.
		 */
		void setFontSize(const spk::Font::Size& p_fontSize);

		/**
		 * @brief Sets the text to render.
		 * @param p_text The text string.
		 */
		void setText(const std::string& p_text);

		/**
		 * @brief Sets the color of the text.
		 * @param p_textColor The color of the text.
		 */
		void setTextColor(const spk::Color& p_textColor);

		/**
		 * @brief Sets the size of the text.
		 * @param p_textSize The size of the text.
		 */
		void setTextSize(const size_t& p_textSize);

		/**
		 * @brief Sets the edge strength of the text.
		 * @param p_textEdgeStrenght The edge strength of the text.
		 */
		void setTextEdgeStrenght(const float& p_textEdgeStrenght);

		/**
		 * @brief Sets the color of the text outline.
		 * @param p_outlineColor The color of the outline.
		 */
		void setOutlineColor(const spk::Color& p_outlineColor);

		/**
		 * @brief Sets the size of the text outline.
		 * @param p_outlineSize The size of the outline.
		 */
		void setOutlineSize(const size_t& p_outlineSize);

		/**
		 * @brief Sets the edge strength of the text outline.
		 * @param p_outlineEdgeStrenght The edge strength of the outline.
		 */
		void setOutlineEdgeStrenght(const float& p_outlineEdgeStrenght);

		/**
		 * @brief Sets the vertical alignment of the text.
		 * @param p_verticalAlignment The vertical alignment.
		 */
		void setVerticalAlignment(const spk::VerticalAlignment& p_verticalAlignment);

		/**
		 * @brief Sets the horizontal alignment of the text.
		 * @param p_horizontalAlignment The horizontal alignment.
		 */
		void setHorizontalAlignment(const spk::HorizontalAlignment& p_horizontalAlignment);

		/**
		 * @brief Gets the current font used for rendering.
		 * @return Pointer to the font object.
		 */
		spk::Font* font() const;

		/**
		 * @brief Gets the current text to render.
		 * @return The text string.
		 */
		const std::string& text() const;

		/**
		 * @brief Gets the anchor point of the rendered text.
		 * @return The anchor point.
		 */
		const spk::Vector2Int& anchor() const;

		/**
		 * @brief Gets the size of the rendered text.
		 * @return The size of the text.
		 */
		const spk::Vector2Int& size() const;

		/**
		 * @brief Get the font size of the renderer.
		 * @return The text and outline size of the font.
		*/
		const spk::Font::Size& fontSize() const;

		/**
		 * @brief Gets the size of the text.
		 * @return The text size.
		 */
		size_t textSize() const;

		/**
		 * @brief Gets the color of the text.
		 * @return The text color.
		 */
		const spk::Color& textColor() const;

		/**
		 * @brief Gets the edge strength of the text.
		 * @return The text edge strength.
		 */
		const float& textEdgeStrenght() const;

		/**
		 * @brief Gets the size of the text outline.
		 * @return The outline size.
		 */
		size_t outlineSize() const;

		/**
		 * @brief Gets the color of the text outline.
		 * @return The outline color.
		 */
		const spk::Color& outlineColor() const;

		/**
		 * @brief Gets the edge strength of the text outline.
		 * @return The outline edge strength.
		 */
		const float& outlineEdgeStrenght() const;

		/**
		 * @brief Gets the vertical alignment of the text.
		 * @return The vertical alignment.
		 */
		const spk::VerticalAlignment& verticalAlignment() const;

		/**
		 * @brief Gets the horizontal alignment of the text.
		 * @return The horizontal alignment.
		 */
		const spk::HorizontalAlignment& horizontalAlignment() const;

		/**
		 * @brief Sets the layer of the rendering object.
		 * @param p_layer The layer value.
		 */
		void setLayer(const float& p_layer);

		/**
		 * @brief Sets the geometry of the rendering area.
		 * @param p_anchor The anchor point of the rendering area.
		 * @param p_newSize The new size of the rendering area.
		 */
		void setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2Int& p_newSize);
	};
}
