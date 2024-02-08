#include "game_engine/component/spk_sprite_renderer_component.hpp"
#include "game_engine/spk_game_object.hpp"

namespace spk
{
	const spk::Mesh SpriteRenderer::_defaultMesh = spk::createSpriteMesh();
	std::string SpriteRenderer::_renderingPipelineCode = R"(#version 450

	#include <transform>
	#include <transformUtils>
	#include <cameraConstants>
	#include <timeConstants>
	#include <spriteAnimation>

	Input -> Geometry : vec3 modelVertex;
	Input -> Geometry : vec2 modelUVs;
	Geometry -> Render : vec2 fragmentUVs;

	Texture textureID;

	AttributeBlock self
	{
		Transform transform;
	};

	AttributeBlock sprite
	{
		ivec2 anchor;
		int animationStartEpoch;
		SpriteAnimation animation;
	};

	vec2 computeSpriteAnimationOffset(int p_animationStartEpoch, SpriteAnimation p_animation)
	{
		if (p_animation.lenght == 0)
			return (vec2(0, 0));
		int nbFrame = (timeConstants.epoch - p_animationStartEpoch) / (p_animation.duration / p_animation.lenght);
		return (p_animation.firstFrame + p_animation.frameOffset * ivec2(nbFrame, nbFrame));
	}

	void geometryPass()
	{
		vec3 transformedPosition = applyTransform(modelVertex, self.transform);
		pixelPosition = mainCamera.MVP * vec4(transformedPosition, 1.0f);
		fragmentUVs = (modelUVs + sprite.anchor + computeSpriteAnimationOffset(sprite.animationStartEpoch, sprite.animation)) * textureID.unit;
	}

	void renderPass()
	{
		pixelColor = texture(textureID.handle, fragmentUVs);
		if (pixelColor.a == 0)
			discard;
	}
	)";

	SpriteRenderer::RenderingPipelineVertex::RenderingPipelineVertex() :
		position(0, 0, 0),
		uvs(0, 0)
	{

	}

	SpriteRenderer::RenderingPipelineVertex::RenderingPipelineVertex(const spk::Vector3& p_position, const spk::Vector2& p_uvs) :
		position(p_position),
		uvs(p_uvs)
	{

	}

	spk::Pipeline SpriteRenderer::_renderingPipeline = spk::Pipeline(_renderingPipelineCode);

	void SpriteRenderer::_updateGPUData()
	{
		Mesh::Data data = _mesh->bake();

		_renderingObject.setVertices(data.vertexes, data.vertexSize);
		_renderingObject.setIndexes(data.indexes);
	}

	void SpriteRenderer::_updateTransform()
	{
		_renderingObjectSelfTransformTranslationAttribute = owner()->globalPosition();
		_renderingObjectSelfTransformScaleAttribute = owner()->globalScale();
		_renderingObjectSelfTransformRotationAttribute = owner()->globalRotation();
		_renderingObjectSelfAttribute.update();
	}

	void SpriteRenderer::_updateSprite()
	{
		if (_spriteSheet == nullptr)
			return ;

		_renderingObjectSpriteAttributeAnchor = _spriteID;
		_renderingObjectSpriteAttribute.update();
	}

	void SpriteRenderer::_updateSpriteAnimation()
	{
		_renderingObjectSpriteAttributeStartEpoch = static_cast<int>(spk::getTime() % 100000);

		if (_spriteAnimation != nullptr)
		{
			_renderingObjectSpriteAttributeAnimation = *_spriteAnimation;
		}
		else
		{
			_renderingObjectSpriteAttributeAnimationLenght = 0;
		}
		
		_renderingObjectSpriteAttribute.update();
	}

	SpriteRenderer::SpriteRenderer(const std::string& p_name) :
		spk::GameComponent(p_name),
		_renderingObject(_renderingPipeline.createObject()),
		_renderingObjectSelfAttribute(_renderingObject.attribute("self")),
		_renderingObjectSelfTransformTranslationAttribute(_renderingObjectSelfAttribute["transform"]["translation"]),
		_renderingObjectSelfTransformScaleAttribute(_renderingObjectSelfAttribute["transform"]["scale"]),
		_renderingObjectSelfTransformRotationAttribute(_renderingObjectSelfAttribute["transform"]["rotation"]),
		_renderingObjectSpriteAttribute(_renderingObject.attribute("sprite")),
		_renderingObjectSpriteAttributeAnchor(_renderingObjectSpriteAttribute["anchor"]),
		_renderingObjectSpriteAttributeStartEpoch(_renderingObjectSpriteAttribute["animationStartEpoch"]),
		_renderingObjectSpriteAttributeAnimation(_renderingObjectSpriteAttribute["animation"]),
		_renderingObjectSpriteAttributeAnimationLenght(_renderingObjectSpriteAttributeAnimation["lenght"]),
		_transformTranslationContract(owner()->transform().translation.subscribe([&](){_updateTransform();})),
		_transformScaleContract(owner()->transform().scale.subscribe([&](){_updateTransform();})),
		_transformRotationContract(owner()->transform().rotation.subscribe([&](){_updateTransform();})),
		_renderingPipelineTexture(_renderingPipeline.texture("textureID"))
	{
		setMesh(&_defaultMesh);

		_updateTransform();
		_updateSprite();
		_updateSpriteAnimation();
	}

	void SpriteRenderer::setMesh(const spk::Mesh* p_mesh)
	{
		_mesh = p_mesh;
		_meshEditionContract = std::move(_mesh->subscribeToEdition([&](){
			_needGPUDataUpdate = true;
		}));
		_needGPUDataUpdate = true;
	}

	void SpriteRenderer::setSpriteSheet(const spk::SpriteSheet* p_spriteSheet)
	{
		_spriteSheet = p_spriteSheet;
		_renderingPipelineTexture.attach(_spriteSheet);
		_updateSprite();
	}

	void SpriteRenderer::setSprite(const spk::Vector2UInt& p_spriteID)
	{
		_spriteID = p_spriteID;
		_updateSprite();
	}

	void SpriteRenderer::setSpriteAnimation(const spk::SpriteAnimation* p_spriteAnimation)
	{
		_spriteAnimation = p_spriteAnimation;
		_updateSpriteAnimation();
	}
	
	void SpriteRenderer::_onRender()
	{
		if (_needGPUDataUpdate == true)
		{
			_updateGPUData();
			_needGPUDataUpdate = false;
		}
	
		_renderingObject.render();
	}

	void SpriteRenderer::_onUpdate()
	{
		
	}
}