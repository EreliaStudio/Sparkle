#pragma once

#include "game_engine/component/spk_game_component.hpp"
#include "math/spk_vector2.hpp"
#include "math/spk_vector3.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"
#include "graphics/texture/spk_sprite_sheet.hpp"
#include "game_engine/spk_transform.hpp"
#include "game_engine/spk_mesh.hpp"
#include "game_engine/spk_sprite_animator.hpp"

namespace spk
{
	/**
	 * @class SpriteRenderer
	 * @brief Represents a sprite renderer component for rendering sprites in world space.
	 * 
	 * This class allows you to create a sprite renderer component that can be attached to game objects.
	 * It provides settings for specifying the mesh, sprite sheet, sprite, and sprite animation to render.
	 * 
	 * Usage example:
	 * @code
	 * // Create an object to hold the sprite renderer.
	 * spk::GameObject gameObject("SpriteObject");
	 * 
	 * // Create a sprite renderer component
	 * SpriteRenderer* spriteRenderer = gameObject.addComponent<spk::SpriteRenderer>("MySpriteRenderer");
	 * 
	 * // Set the mesh for rendering the sprite
	 * spriteRenderer->setMesh(&spk::SpriteRenderer::_defaultMesh);
	 * 
	 * // Set the sprite sheet and sprite ID
	 * spriteRenderer->setSpriteSheet(&mySpriteSheet);
	 * spriteRenderer->setSprite(spk::Vector2UInt(0, 0));
	 * 
	 * // Set the sprite animation (optional)
	 * spriteRenderer->setSpriteAnimation(&mySpriteAnimation);
	 * @endcode
	 * 
	 * @see GameComponent, GameObject, Texture, SpriteAnimation, IVector2
	 */

	class SpriteRenderer : public spk::GameComponent
	{
	private:
		static const spk::Mesh _defaultMesh;
		static std::string _renderingPipelineCode;

		struct RenderingPipelineVertex
		{
			spk::Vector3 position;
			spk::Vector2 uvs;

			RenderingPipelineVertex();
			RenderingPipelineVertex(const spk::Vector3& p_position, const spk::Vector2& p_uvs);
		};

		static spk::Pipeline _renderingPipeline;

		spk::Pipeline::Object _renderingObject;
		spk::Pipeline::Object::Attribute& _renderingObjectSelfAttribute;
		spk::Pipeline::Object::Attribute::Element& _renderingObjectSelfTransformTranslationAttribute;
		spk::Pipeline::Object::Attribute::Element& _renderingObjectSelfTransformScaleAttribute;
		spk::Pipeline::Object::Attribute::Element& _renderingObjectSelfTransformRotationAttribute;

		spk::Pipeline::Object::Attribute& _renderingObjectSpriteAttribute;
		spk::Pipeline::Object::Attribute::Element& _renderingObjectSpriteAttributeAnchor;
		spk::Pipeline::Object::Attribute::Element& _renderingObjectSpriteAttributeStartEpoch;
		spk::Pipeline::Object::Attribute::Element& _renderingObjectSpriteAttributeAnimation;
		spk::Pipeline::Object::Attribute::Element& _renderingObjectSpriteAttributeAnimationLenght;
		
		spk::Pipeline::Texture& _renderingPipelineTexture;

		bool _needGPUDataUpdate = false;
		const spk::Mesh* _mesh;
		std::unique_ptr<spk::Mesh::Contract> _meshEditionContract;
		const spk::SpriteSheet* _spriteSheet = nullptr;
		spk::Vector2UInt _spriteID = spk::Vector2UInt(0, 0);
		const spk::SpriteAnimation* _spriteAnimation = nullptr;

		std::unique_ptr<Transform::Contract> _transformTranslationContract = nullptr;
		std::unique_ptr<Transform::Contract> _transformScaleContract = nullptr;
		std::unique_ptr<Transform::Contract> _transformRotationContract = nullptr;

		void _updateGPUData();
		void _updateTransform();
		void _updateSprite();
		void _updateSpriteAnimation();

		void _onRender();
		void _onUpdate();

	public:
		/**
		 * @brief Constructor
		 * 
		 * @param p_name The name of the sprite renderer component, used for identification purposes.
		 */
		SpriteRenderer(const std::string& p_name);

		/**
		 * @brief Sets the mesh used by the sprite renderer.
		 * 
		 * Specifies the mesh that will be used to render the sprite. This mesh typically defines the geometry
		 * on which the sprite texture will be applied.
		 * 
		 * @param p_mesh Pointer to the Mesh object to be used for rendering the sprite.
		 */
		void setMesh(const spk::Mesh* p_mesh);

		/**
		 * @brief Sets the sprite sheet used by the renderer.
		 * 
		 * Specifies the sprite sheet that contains the sprite textures. The sprite sheet allows for efficient texture
		 * management by packing multiple sprites into a single texture.
		 * 
		 * @param p_spriteSheet Pointer to the SpriteSheet object to be used by the renderer.
		 */
		void setSpriteSheet(const spk::SpriteSheet* p_spriteSheet);

		/**
		 * @brief Sets the sprite to be rendered.
		 * 
		 * Specifies the sprite within the sprite sheet to be rendered by identifying its position within the sprite sheet grid.
		 * 
		 * @param p_spriteID A Vector2UInt representing the grid position (x, y) of the sprite within the sprite sheet.
		 */
		void setSprite(const spk::Vector2UInt& p_spriteID);

		/**
		 * @brief Sets the sprite animation.
		 * 
		 * Specifies the sprite animation to be used for dynamic sprite rendering. This allows the sprite to change over time,
		 * creating animated effects.
		 * 
		 * @param p_spriteAnimation Pointer to the SpriteAnimation object to be used by the renderer.
		 */
		void setSpriteAnimation(const spk::SpriteAnimation* p_spriteAnimation);
	};
}