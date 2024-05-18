#pragma once

#include "graphics/pipeline/spk_pipeline.hpp"
#include "graphics/spk_color.hpp"

namespace spk::WidgetComponent
{
	/**
	 * @class TextureRenderer
	 * @brief A component for rendering textures within the UI, supporting various transformations and UV mapping.
	 *
	 * This class is a core UI component designed for displaying textures within the SPK engine's widget system.
	 * It allows for the rendering of textures with specific UV mapping, size, and anchor positioning, enabling
	 * the flexible display of images in the UI. The component leverages the SPK engine's rendering pipeline
	 * for efficient texture rendering.
	 *
	 * The class supports various texture manipulations, including setting the texture, adjusting its UV mapping,
	 * and modifying its size and position within the UI. These features make it suitable for a wide range of
	 * texture rendering tasks within the SPK engine.
	 *
	 * Usage example:
	 * @code
	 * class MyCustomWidget : public spk::Widget
	 * {
	 * private:
	 *	 spkComponent::TextureRenderer _textureRenderer;
	 *
	 *	 void _onRender() override
	 *	 {
	 *		 _textureRenderer.render();
	 *	 }
	 *
	 * public:
	 *	 MyCustomWidget(const std::string& name, spk::Widget* parent = nullptr) :
	 *		 spk::Widget(name, parent),
	 *		 _textureRenderer()
	 *	 {
	 *		 _textureRenderer.setTexture(myTexture); // Assuming myTexture is a preloaded Texture instance
	 *		 _textureRenderer.setGeometry({10, 10}, {100, 100});
	 *		 _textureRenderer.setUVMapping({0, 0}, {1, 1});
	 *	 }
	 * };
	 * @endcode
	 *
	 * @see Pipeline, Texture
	 */
	class TextureRenderer
	{
	private:
		static const std::string _renderingPipelineCode;
		static spk::Pipeline _renderingPipeline;

		struct ShaderInput
		{
			spk::Vector2Int position;
			spk::Vector2 uvs;

			ShaderInput(const spk::Vector2Int& p_position, const spk::Vector2& p_uvs);
		};

		spk::Pipeline::Object _renderingObject;
		spk::Pipeline::Object::Attribute& _selfAttribute;
		spk::Pipeline::Object::Attribute::Element& _selfLayerElement;
		spk::Pipeline::Texture& _renderingPipelineTexture;

		spk::Vector2 _anchor;
		spk::Vector2 _size;

		const spk::Texture* _texture;
		spk::Vector2 _uvAnchor;
		spk::Vector2 _uvSize;

		bool _needGPUInputUpdate;

		void _updateGPUData();

	public:
		/**
		 * @brief Default constructor for TextureRenderer.
		 *
		 * Initializes a new instance of the TextureRenderer class, setting up the necessary rendering pipeline
		 * and preparing the object for texture rendering within the UI. This constructor initializes all internal
		 * structures and prepares the object for subsequent configuration and rendering operations.
		 */
		TextureRenderer();

		/**
		 * @brief Renders the texture to the screen.
		 *
		 * Executes the rendering pipeline to display the texture on the screen based on the current configuration
		 * of the TextureRenderer. This includes using the specified texture, geometry, and UV mapping settings. It
		 * ensures that the texture is rendered at the specified anchor point and size, with the correct UV coordinates.
		 */
		void render();

		/**
		 * @brief Sets the texture to be rendered.
		 *
		 * @param p_texture A pointer to the texture object to be used for rendering. This texture should be managed
		 * externally and must remain valid for the lifetime of the TextureRenderer or until a new texture is set.
		 */
		void setTexture(const spk::Texture* p_texture);

		/**
		 * @brief Sets the geometry for texture UV mapping.
		 *
		 * @param p_uvAnchor The UV coordinate of the texture's anchor point. This point corresponds to the upper-left
		 * corner of the texture area to be used.
		 * @param p_uvSize The size of the texture area to be used, defined in UV coordinates. This determines the
		 * portion of the texture to be rendered and how it is stretched or shrunk.
		 */
		void setTextureGeometry(const spk::Vector2& p_uvAnchor, const spk::Vector2& p_uvSize);

		/**
		 * @brief Gets the anchor point of the texture in screen space.
		 *
		 * @return The anchor point of the texture, representing the upper-left corner of the texture as it is rendered
		 * on the screen.
		 */
		const Vector2& anchor() const;

		/**
		 * @brief Gets the size of the texture as it is rendered on the screen.
		 *
		 * @return The size of the texture in screen space, which determines how the texture is stretched or shrunk
		 * during rendering.
		 */
		const Vector2& size() const;

		/**
		 * @brief Gets the UV anchor point of the texture.
		 *
		 * @return The UV coordinates of the anchor point, defining the upper-left corner of the texture area to be
		 * used for rendering.
		 */
		const Vector2& uvAnchor() const;

		/**
		 * @brief Gets the UV size of the texture.
		 *
		 * @return The size of the texture area to be used for rendering, defined in UV coordinates. This size determines
		 * the portion of the texture to be rendered.
		 */
		const Vector2& uvSize() const;

		/**
		 * @brief Sets the geometry of the texture in screen space.
		 *
		 * @param p_anchor The new anchor point for the texture in screen space.
		 * @param p_newSize The new size for the texture as it will be rendered on the screen.
		 */
		void setGeometry(const Vector2& p_anchor, const Vector2& p_newSize);

		/**
		 * @brief Sets the layer depth for the texture rendering.
		 *
		 * @param p_layer The depth value for the texture. This value determines the rendering order of the texture
		 * relative to other objects in the scene. Lower values are rendered first and may be overlaid by objects with
		 * higher values.
		 */
		void setLayer(const float& p_layer);

	};
}