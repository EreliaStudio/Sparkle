#pragma once

#include <fstream>
#include "game_engine/spk_game_object.hpp"
#include "game_engine/component/spk_game_component.hpp"

namespace spk
{
	template <typename TNodeIndexType, size_t TSizeX, size_t TSizeY, size_t TSizeZ>
	class ITilemap : public spk::GameObject
	{
	public:
		class IChunk : public spk::GameObject
		{
		public:
			using NodeIndexType = TNodeIndexType;

			static inline const size_t SizeX = TSizeX;
			static inline const size_t SizeY = TSizeY;
			static inline const size_t SizeZ = TSizeZ;

		private:
			class Renderer : public spk::GameComponent
			{
			private:
				IChunk* linkedChunk;

				void _onUpdate()
				{

				}

				void _onRender()
				{
					if (linkedChunk->_needGPUUpdate == true)
					{
						linkedChunk->_onBaking();
						linkedChunk->_needGPUUpdate = false;
					}

					linkedChunk->_onObjectRendering();
				}

			public:
				Renderer(const std::string& p_name, IChunk* p_linkedChunk) :
					spk::GameComponent(p_name),
					linkedChunk(p_linkedChunk)
				{

				}
			};
			
			virtual void _onBaking() = 0;
			virtual void _onObjectRendering() = 0;

			spk::Vector2Int _position;
			TNodeIndexType _data[TSizeX][TSizeY][TSizeZ];
			bool _needGPUUpdate = true;

		public:
			IChunk(const spk::Vector2Int& p_chunkPosition, spk::GameObject* p_parent = nullptr) : 
				spk::GameObject("Chunk " + p_chunkPosition.to_string(), p_parent),
				_position(p_chunkPosition)
			{
				addComponent<Renderer>("Renderer", this);

				transform().translation = spk::Vector3(static_cast<float>(p_chunkPosition.x * static_cast<int>(SizeX)), static_cast<float>(p_chunkPosition.y * static_cast<int>(SizeY)), 0.0f);

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

			template<typename TVectorType>
			spk::IVector3<TVectorType> convertAbsoluteToRelativePosition(const spk::IVector3<TVectorType>& p_absolutePosition) const
			{
				return (p_absolutePosition - spk::IVector3<TVectorType>(_position.x * SizeX, _position.y * SizeY, 0));
			}

			template<typename TVectorType>
			spk::IVector2<TVectorType> convertAbsoluteToRelativePosition(const spk::IVector2<TVectorType>& p_absolutePosition) const
			{
				return (convertAbsoluteToRelativePosition(spk::IVector3<TVectorType>(p_absolutePosition, 0.0f)).xy());
			}
		};
	private:
		std::map<spk::Vector2Int, std::unique_ptr<IChunk>> _chunks;

		virtual IChunk* _instanciateNewChunk(const spk::Vector2Int& p_chunkPosition) = 0;

	protected:
		std::map<spk::Vector2Int, std::unique_ptr<IChunk>>& chunks()
		{
			return (_chunks);
		}

	public:
		ITilemap(const std::string& p_name, spk::GameObject* p_parent = nullptr) : 
			spk::GameObject(p_name, p_parent)
		{
			
		}

		const std::map<spk::Vector2Int, std::unique_ptr<IChunk>>& chunks() const
		{
			return (_chunks);
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

		bool containsChunk(const spk::Vector2Int& p_chunkPosition) const
		{
			return (_chunks.contains(p_chunkPosition));
		}

		IChunk* createChunk(const spk::Vector2Int p_chunkPosition)
		{
			if (_chunks.contains(p_chunkPosition) == true)
			{
				return (_chunks.at(p_chunkPosition).get());
			}

			IChunk* result = _instanciateNewChunk(p_chunkPosition);

			this->addChild(result);
			_chunks[p_chunkPosition] = std::unique_ptr<IChunk>(result);

			return (result);
		}

		IChunk* chunk(const spk::Vector2Int& p_chunkPosition)
		{
			if (containsChunk(p_chunkPosition) == false)
				return (nullptr);
			return (_chunks.at(p_chunkPosition).get());
		}

		const IChunk* chunk(const spk::Vector2Int& p_chunkPosition) const
		{
			if (containsChunk(p_chunkPosition) == false)
				return (nullptr);
			return (_chunks.at(p_chunkPosition).get());
		}

		void rebakeChunk(const spk::Vector2Int& p_chunkPosition)
		{
			for (int i = -1; i <= 1; i++)
			{
				for (int j = -1; j <= 1; j++)
				{
					spk::Vector2Int chunkPosition = p_chunkPosition + spk::Vector2Int(i, j);
					if (containsChunk(chunkPosition) == true)
					{
						spk::GameObject* chunkObject = chunk(chunkPosition);

						if (chunkObject != nullptr)
						{
							chunkObject->getComponent<IChunk>()->bake();
						}
						
					}
				}
			}
		}

		void saveToFolder(const std::filesystem::path& p_folderPath) const
		{
			for (auto& [key, element] : _chunks)
			{
				element->getComponent<IChunk>()->saveToFile(p_folderPath.string() + "/chunk_" + std::to_string(key.x) + "_" + std::to_string(key.y) + ".chk");
			}
		}

		void activateChunk(const spk::Vector2Int& p_chunkPosition)
		{
			_chunks[p_chunkPosition]->activate();
		}

		void deactivateChunk(const spk::Vector2Int& p_chunkPosition)
		{
			_chunks[p_chunkPosition]->deactivate();
		}
	};
}