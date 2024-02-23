#pragma once

#include <vector>
#include "math/spk_vector2.hpp"
#include "math/spk_vector3.hpp"
#include "graphics/texture/spk_sprite_sheet.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"
#include "game_engine/spk_game_object.hpp"
#include "game_engine/component/spk_game_component.hpp"

namespace spk
{
	template <typename TNodeType, typename TNodeIndexType, size_t TSizeX, size_t TSizeY, size_t TSizeZ>
	class ITilemap : public spk::GameComponent
	{
	public:
		using Node = TNodeType;

		class IChunk : public spk::GameComponent
		{
			friend class ITilemap;
		public:
			using NodeIndexType = TNodeIndexType;

			static inline const size_t SizeX = TSizeX;
			static inline const size_t SizeY = TSizeY;
			static inline const size_t SizeZ = TSizeZ;

		protected:
			struct VertexData
			{
				spk::Vector3 modelVertex;
				spk::Vector2 modelUVs;
				spk::Vector2 modelAnimationOffset;
				int modelAnimationSize;

				VertexData() :
					modelVertex(0),
					modelUVs(0),
					modelAnimationOffset(0),
					modelAnimationSize(0)
				{

				}

				VertexData(const spk::Vector3& p_modelVertex, const spk::Vector2& p_modelUVs, const spk::Vector2& p_modelAnimationOffset, int p_modelAnimationSize) :
					modelVertex(p_modelVertex),
					modelUVs(p_modelUVs),
					modelAnimationOffset(p_modelAnimationOffset),
					modelAnimationSize(p_modelAnimationSize)
				{

				}
			};

		private:
			static inline const std::string _renderingPipelineCode = R"(#version 450
			
			#include <timeConstants>
			#include <cameraConstants>
			#include <transform>
			#include <transformUtils>
			
			Input -> Geometry : vec3 modelVertex;
			Input -> Geometry : vec2 modelUVs;
			Input -> Geometry : vec2 modelAnimationOffset;
			Input -> Geometry : int modelAnimationSize;

			Geometry -> Render : vec2 fragmentUVs;

			AttributeBlock self
			{
				Transform transform;
			};

			Texture textureID;

			void geometryPass()
			{
				vec3 transformedPosition = applyTransform(modelVertex, self.transform);
				vec4 cameraSpacePosition = cameraConstants.view * vec4(transformedPosition, 1.0f);
				pixelPosition = cameraConstants.projection * cameraSpacePosition;
				fragmentUVs = modelUVs;
			}

			void renderPass()
			{
				pixelColor = texture(textureID, fragmentUVs);
				if (pixelColor.a == 0)
					discard;
			}
			)";
			static inline spk::Pipeline _renderingPipeline = spk::Pipeline(_renderingPipelineCode);
			spk::Pipeline::Object _renderingObject;

			spk::Pipeline::Object::Attribute& _renderingObjectSelfAttribute;
			spk::Pipeline::Object::Attribute::Element& _renderingObjectSelfTransformTranslationElement;
			std::unique_ptr<spk::Transform::Contract> _transformTranslationContract = nullptr;

			spk::Pipeline::Object::Attribute::Element& _renderingObjectSelfTransformScaleElement;
			std::unique_ptr<spk::Transform::Contract> _transformScaleContract = nullptr;
			
			spk::Pipeline::Object::Attribute::Element& _renderingObjectSelfTransformRotationElement;
			std::unique_ptr<spk::Transform::Contract> _transformRotationContract = nullptr;

			spk::Pipeline::Texture& _textureObject;
			spk::Texture* _texture;

			TNodeIndexType _data[TSizeX][TSizeY][TSizeZ];

			bool _needGPUUpdate = false;

			void _onUpdate()
			{

			}

			virtual void _bake(std::vector<VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes) = 0;

			void _updateGPUData()
			{
				std::vector<VertexData> data;
				std::vector<unsigned int> indexes;

				_bake(data, indexes);

				_renderingObject.setVertices(data);
				_renderingObject.setIndexes(indexes);
			}

			void _updateTransform()
			{
				_renderingObjectSelfTransformTranslationElement = owner()->globalPosition();
				_renderingObjectSelfTransformScaleElement = owner()->globalScale();
				_renderingObjectSelfTransformRotationElement = owner()->globalRotation();
				_renderingObjectSelfAttribute.update();
			}

			void _onRender()
			{
				if (_needGPUUpdate == true)
				{
					_updateGPUData();
					_needGPUUpdate = false;
				}

				_textureObject.attach(_texture);
				_renderingObject.render();
			}


		public:
			IChunk(const std::string& p_name) :
				spk::GameComponent(p_name),
				_renderingObject(_renderingPipeline.createObject()),
				_textureObject(_renderingPipeline.texture("textureID")),
				_renderingObjectSelfAttribute(_renderingObject.attribute("self")),
				_renderingObjectSelfTransformTranslationElement(_renderingObjectSelfAttribute["transform"]["translation"]),
				_transformTranslationContract(owner()->transform().translation.subscribe([&](){_updateTransform();})),
				_renderingObjectSelfTransformScaleElement(_renderingObjectSelfAttribute["transform"]["scale"]),
				_transformScaleContract(owner()->transform().scale.subscribe([&](){_updateTransform();})),
				_renderingObjectSelfTransformRotationElement(_renderingObjectSelfAttribute["transform"]["rotation"]),
				_transformRotationContract(owner()->transform().rotation.subscribe([&](){_updateTransform();}))
			{
				_updateTransform();

				for (size_t i = 0; i < SizeX; i++)
				{
					for (size_t j = 0; j < SizeY; j++)
					{
						for (size_t k = 0; k < SizeZ; k++)
						{
							_data[i][j][k] = -1;
						}
					}
				}
			}

			NodeIndexType content(const spk::Vector3Int &p_position) const
			{
				return (_data[p_position.x][p_position.y][p_position.z]);
			}

			NodeIndexType content(const spk::Vector2Int &p_position, int p_z) const
			{
				return (_data[p_position.x][p_position.y][p_z]);
			}

			NodeIndexType content(int p_x, int p_y, int p_z) const
			{
				return (_data[p_x][p_y][p_z]);
			}

			void setContent(const spk::Vector3Int &p_position, NodeIndexType p_value)
			{
				_data[p_position.x][p_position.y][p_position.z] = p_value;
			}

			void setContent(const spk::Vector2Int &p_position, int p_z, NodeIndexType p_value)
			{
				_data[p_position.x][p_position.y][p_z] = p_value;
			}

			void setContent(int p_x, int p_y, int p_z, NodeIndexType p_value)
			{
				_data[p_x][p_y][p_z] = p_value;
			}

			void bake()
			{
				_needGPUUpdate = true;
			}

			void setTexture(spk::Texture* p_texture)
			{
				_texture = p_texture;
			}

			spk::Texture* texture()
			{
				return (_texture);
			}
		};

	private:
		std::map<spk::Vector2Int, std::unique_ptr<spk::GameObject>> _chunksObjects;

		virtual IChunk* _insertChunk(spk::GameObject* p_object, const spk::Vector2Int& p_chunkPosition) = 0;

	protected:
		std::map<spk::Vector2Int, std::unique_ptr<spk::GameObject>>& chunksObjects()
		{
			return (_chunksObjects);
		}

	public:
		ITilemap(const std::string& p_name) :
			spk::GameComponent(p_name)
		{
			
		}

		static spk::Vector2Int convertWorldToChunkPosition(const spk::Vector2Int& p_worldPosition)
		{
			return (spk::Vector2Int(
				p_worldPosition.x / IChunk::SizeX,
				p_worldPosition.y / IChunk::SizeY
			));
		}

		static spk::Vector2Int convertWorldToChunkPositionXY(const spk::Vector3Int& p_worldPosition)
		{
			return (spk::Vector2Int(
				p_worldPosition.x / IChunk::SizeX,
				p_worldPosition.y / IChunk::SizeY
			));
		}

		static spk::Vector2Int convertWorldToChunkPositionXZ(const spk::Vector3Int& p_worldPosition)
		{
			return (spk::Vector2Int(
				p_worldPosition.x / IChunk::SizeX,
				p_worldPosition.z / IChunk::SizeZ
			));
		}

		IChunk* createEmpyChunk(const spk::Vector2Int& p_chunkPosition)
		{
			if (_chunksObjects.contains(p_chunkPosition) == true)
			{
				return (_chunksObjects.at(p_chunkPosition)->getComponent<IChunk>());
			}
			std::unique_ptr<spk::GameObject> newObject = std::make_unique<spk::GameObject>("Chunk [" + p_chunkPosition.to_string() + "]", owner());
			IChunk* chunkComponent = _insertChunk(newObject.get(), p_chunkPosition);
			chunkComponent->bake();
			_chunksObjects.emplace(p_chunkPosition, std::move(newObject));
			return (chunkComponent);
		}

		spk::GameObject* chunkObject(const spk::Vector2Int& p_chunkPosition)
		{
			if (_chunksObjects.contains(p_chunkPosition) == false)
			{
				return (nullptr);
			}

			return (_chunksObjects.at(p_chunkPosition).get());
		}

		const spk::GameObject* chunkObject(const spk::Vector2Int& p_chunkPosition) const
		{
			if (_chunksObjects.contains(p_chunkPosition) == false)
			{
				return (nullptr);
			}

			return (_chunksObjects.at(p_chunkPosition).get());
		}

		void activateChunk(const spk::Vector2Int& p_chunkPosition)
		{
			_chunksObjects[p_chunkPosition].activate();
		}

		void deactivateChunk(const spk::Vector2Int& p_chunkPosition)
		{
			_chunksObjects[p_chunkPosition].deactivate();
		}

		void rebakeChunk(const spk::Vector2Int& p_chunkPosition)
		{
			for (int i = 1; i <= 1; i++)
			{
				for (int j = 1; j <= 1; j++)
				{
					spk::Vector2Int chunkPosition = p_chunkPosition + spk::Vector2Int(i, j);
					if (_chunksObjects.contains(chunkPosition) == true)
					{
						spk::GameObject* chunkObject = _chunksObjects[chunkPosition];

						if (chunkObject != nullptr)
						{
							IChunk* tmpChunk = chunkObject->getComponent<IChunk>();

							if (tmpChunk != nullptr)
							{
								tmpChunk->bake();
							}
						}
						
					}
				}
			}
		}
	};
}