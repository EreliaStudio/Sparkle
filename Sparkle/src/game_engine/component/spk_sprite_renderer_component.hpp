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
	class SpriteRenderer : public spk::GameComponent
	{
	public:
		static const spk::Mesh defaultMesh;
		
	private:
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

	public:
		SpriteRenderer(const std::string& p_name);

		void setMesh(const spk::Mesh* p_mesh);

		void setSpriteSheet(const spk::SpriteSheet* p_spriteSheet);
		void setSprite(const spk::Vector2UInt& p_spriteID);
		void setSpriteAnimation(const spk::SpriteAnimation* p_spriteAnimation);
		
		void onRender();
		void onUpdate();
	};
}