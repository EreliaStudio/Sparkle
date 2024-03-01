#pragma once

#include <vector>
#include <string>
#include <fstream>
#include "math/spk_vector2.hpp"
#include "math/spk_vector3.hpp"
#include "graphics/texture/spk_sprite_sheet.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"
#include "game_engine/spk_game_object.hpp"
#include "game_engine/component/spk_game_component.hpp"

namespace spk
{
	template <typename TNodeIndexType, size_t TSizeX, size_t TSizeY, size_t TSizeZ>
	class ITilemap : public spk::GameComponent
	{
	public:
		class IChunk : public spk::GameComponent
		{
			friend class ITilemap;
		public:
			using NodeIndexType = TNodeIndexType;

			static inline const size_t SizeX = TSizeX;
			static inline const size_t SizeY = TSizeY;
			static inline const size_t SizeZ = TSizeZ;

		private:
			spk::Vector2Int _position;
			TNodeIndexType _data[TSizeX][TSizeY][TSizeZ];
			bool _needGPUUpdate = true;
		
			void _onUpdate()
			{

			}

			virtual void _onBaking() = 0;
			virtual void _onObjectRendering() = 0;

			void _onRender()
			{
				if (_needGPUUpdate == true)
				{
					_onBaking();
					_needGPUUpdate = false;
				}

				_onObjectRendering();
			}

		public:
			IChunk(const spk::Vector2Int& p_chunkPosition) :
				spk::GameComponent("Chunk " + p_chunkPosition.to_string()),
				_position(p_chunkPosition)
			{
				owner()->transform().translation = spk::Vector3(static_cast<float>(p_chunkPosition.x * static_cast<int>(SizeX)), static_cast<float>(p_chunkPosition.y * static_cast<int>(SizeY)), 0.0f);

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

			void saveToFile(const std::filesystem::path& p_path) const
			{
				std::ofstream outFile(p_path, std::ios::binary | std::ios::out);
				if (!outFile)
				{
					throwException("Could not open file for writing: " + p_path.string());
				}

				outFile.write(reinterpret_cast<const char*>(&_data), sizeof(_data));
				if (!outFile)
				{
					throwException("Error writing to file: " + p_path.string());
				}

				outFile.close();
			}

			void loadFromFile(const std::filesystem::path& p_path)
			{
				std::ifstream inFile(p_path, std::ios::binary | std::ios::in);
				if (!inFile)
				{
					throwException("Could not open file for reading: " + p_path.string());
				}

				inFile.read(reinterpret_cast<char*>(&_data), sizeof(_data));
				if (inFile.fail())
				{
					throwException("Error reading file: " + p_path.string());
				}

				inFile.close();
			}

			const spk::Vector2Int& position() const {return (_position); }
		};

	private:
		std::map<spk::Vector2Int, std::unique_ptr<spk::GameObject>> _chunksObjects;

		virtual IChunk* _insertChunk(spk::GameObject* p_object, const spk::Vector2Int& p_chunkPosition) = 0;

		void _onUpdate()
		{

		}

		void _onRender()
		{

		}

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
				std::floorf(p_worldPosition.x / static_cast<float>(IChunk::SizeX)),
				std::floorf(p_worldPosition.y / static_cast<float>(IChunk::SizeY))
			));
		}

		static spk::Vector2Int convertWorldToChunkPositionXY(const spk::Vector3Int& p_worldPosition)
		{
			return (spk::Vector2Int(
				std::floorf(p_worldPosition.x / static_cast<float>(IChunk::SizeX)),
				std::floorf(p_worldPosition.y / static_cast<float>(IChunk::SizeY))
			));
		}

		static spk::Vector2Int convertWorldToChunkPositionXZ(const spk::Vector3Int& p_worldPosition)
		{
			return (spk::Vector2Int(
				std::floorf(p_worldPosition.x / static_cast<float>(IChunk::SizeX)),
				std::floorf(p_worldPosition.z / static_cast<float>(IChunk::SizeZ))
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

		bool containsChunk(const spk::Vector2Int& p_chunkPosition) const
		{
			return (_chunksObjects.contains(p_chunkPosition));
		}

		const spk::GameObject* chunkObject(const spk::Vector2Int& p_chunkPosition) const
		{
			if (containsChunk(p_chunkPosition) == false)
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
							chunkObject->getComponent<IChunk>()->bake();
						}
						
					}
				}
			}
		}
	};
}