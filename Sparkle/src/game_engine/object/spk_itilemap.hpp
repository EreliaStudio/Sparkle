#pragma once

#include <fstream>
#include "game_engine/spk_game_object.hpp"
#include "game_engine/component/spk_game_component.hpp"

namespace spk
{
	/**
     * @class ITilemap
     * @brief Represents a tilemap composed of multiple chunks.
     * 
     * This class serves as a base for tilemap implementations, utilizing a templated node index type and fixed-size chunks.
     * It provides mechanisms to manage chunks, including creation, querying, and serialization to/from files.
     *
     * @tparam TNodeIndexType The type of the node index used in the tilemap.
     * @tparam TSizeX The width of each chunk.
     * @tparam TSizeY The height of each chunk.
     * @tparam TSizeZ The depth of each chunk.
     */
	template <typename TNodeIndexType, size_t TSizeX, size_t TSizeY, size_t TSizeZ>
	class ITilemap : public spk::GameObject
	{
	public:
		/**
         * @class IChunk
         * @brief Represents a single chunk within the tilemap.
         * 
         * Chunks are individual segments of the tilemap, each containing a fixed number of nodes. 
         * This class provides the base functionality for a chunk, including data storage, rendering,
         * and serialization to/from files.
         */
		class IChunk : public spk::GameObject
		{
		public:
			using NodeIndexType = TNodeIndexType; ///< Type of the node index.

            static inline const size_t SizeX = TSizeX; ///< Width of the chunk.
            static inline const size_t SizeY = TSizeY; ///< Height of the chunk.
            static inline const size_t SizeZ = TSizeZ; ///< Depth of the chunk.

		private:
			/**
             * @class Renderer
             * @brief Responsible for rendering the chunk.
             * 
             * This class handles the rendering logic for a chunk, including updating and drawing the chunk
             * based on its current state.
             */
			class Renderer : public spk::GameComponent
			{
			private:
				IChunk* linkedChunk; ///< Pointer to the chunk associated with this renderer.

				/**
                 * @brief Called on each render frame.
                 * 
                 * Checks if the chunk needs GPU update and triggers rendering of the chunk.
                 */
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
				/**
                 * @brief Constructs a Renderer linked to a specific chunk.
                 * 
                 * @param p_name Name of the renderer component.
                 * @param p_linkedChunk The chunk that this renderer is associated with.
                 */
				Renderer(const std::string& p_name, IChunk* p_linkedChunk) :
					spk::GameComponent(p_name),
					linkedChunk(p_linkedChunk)
				{

				}
			};
			
            /**
             * @brief Handles the baking process of the chunk.
             * 
             * This virtual method should be implemented to handle the baking (pre-processing) logic required
             * for a chunk, typically used to prepare data for GPU rendering.
             */
			virtual void _onBaking() = 0;

            /**
             * @brief Handles the rendering process of the chunk.
             * 
             * This virtual method should be implemented to handle the actual rendering logic of the chunk.
             */
			virtual void _onObjectRendering() = 0;

			spk::Vector2Int _position;
			TNodeIndexType _data[TSizeX][TSizeY][TSizeZ];
			bool _needGPUUpdate = true;

		public:
			/**
             * @brief Constructs a chunk at a given position.
             * 
             * Initializes the chunk's position and allocates memory for its data. All nodes are initially set to a default value.
             * 
             * @param p_chunkPosition The position of the chunk within the tilemap.
             * @param p_parent The parent game object (optional).
             */
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
            /**
             * @brief Retrieves the content at a specified 3D position within the chunk.
             * 
             * @param p_position A Vector3Int representing the 3D position within the chunk.
             * @return The content of type NodeIndexType at the specified position.
             */
			NodeIndexType content(const spk::Vector3Int &p_position) const
			{
				return (_data[p_position.x][p_position.y][p_position.z]);
			}

            /**
             * @brief Retrieves the content at a specified 2D position and depth within the chunk.
             * 
             * @param p_position A Vector2Int representing the 2D position within the chunk.
             * @param p_z The depth within the chunk.
             * @return The content of type NodeIndexType at the specified position and depth.
             */
			NodeIndexType content(const spk::Vector2Int &p_position, int p_z) const
			{
				return (_data[p_position.x][p_position.y][p_z]);
			}

            /**
             * @brief Retrieves the content at a specified x, y, and z coordinate within the chunk.
             * 
             * @param p_x The x coordinate within the chunk.
             * @param p_y The y coordinate within the chunk.
             * @param p_z The z coordinate within the chunk.
             * @return The content of type NodeIndexType at the specified coordinates.
             */
			NodeIndexType content(int p_x, int p_y, int p_z) const
			{
				return (_data[p_x][p_y][p_z]);
			}

            /**
             * @brief Sets the content at a specified 3D position within the chunk.
             * 
             * @param p_position A Vector3Int representing the 3D position within the chunk.
             * @param p_value The value of type NodeIndexType to set at the specified position.
             */
			void setContent(const spk::Vector3Int &p_position, NodeIndexType p_value)
			{
				_data[p_position.x][p_position.y][p_position.z] = p_value;
			}

            /**
             * @brief Sets the content at a specified 2D position and depth within the chunk.
             * 
             * @param p_position A Vector2Int representing the 2D position within the chunk.
             * @param p_z The depth within the chunk.
             * @param p_value The value of type NodeIndexType to set at the specified position and depth.
             */
			void setContent(const spk::Vector2Int &p_position, int p_z, NodeIndexType p_value)
			{
				_data[p_position.x][p_position.y][p_z] = p_value;
			}

            /**
             * @brief Sets the content at a specified x, y, and z coordinate within the chunk.
             * 
             * @param p_x The x coordinate within the chunk.
             * @param p_y The y coordinate within the chunk.
             * @param p_z The z coordinate within the chunk.
             * @param p_value The value of type NodeIndexType to set at the specified coordinates.
             */
			void setContent(int p_x, int p_y, int p_z, NodeIndexType p_value)
			{
				_data[p_x][p_y][p_z] = p_value;
			}

            /**
             * @brief Marks the chunk as needing a GPU update.
             * 
             * This method sets a flag indicating that the chunk's graphical representation needs to be updated.
             * It typically triggers a re-baking process during the next render pass.
             */
			void bake()
			{
				_needGPUUpdate = true;
			}

            /**
             * @brief Saves the chunk data to a binary file.
             * 
             * Serializes the chunk's data array to a specified file path. Throws an exception if the file cannot be opened or written to.
             * 
             * @param p_path The filesystem path where the chunk data should be saved.
             */
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

            /**
             * @brief Loads chunk data from a binary file.
             * 
             * Deserializes the chunk's data array from a specified file path. Throws an exception if the file cannot be opened or read from.
             * 
             * @param p_path The filesystem path from which the chunk data should be loaded.
             */
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

            /**
             * @brief Gets the position of the chunk within the tilemap.
             * 
             * @return The position of the chunk as a Vector2Int.
             */
			const spk::Vector2Int& position() const {return (_position); }

            /**
             * @brief Converts an absolute world position to a relative chunk position.
             * 
             * This template method converts a 3D world position to a 3D position relative to the chunk's origin.
             * 
             * @param p_absolutePosition The absolute world position as an IVector3.
             * @return The relative position within the chunk as an IVector3.
             */
			template<typename TVectorType>
			spk::IVector3<TVectorType> convertAbsoluteToRelativePosition(const spk::IVector3<TVectorType>& p_absolutePosition) const
			{
				return (p_absolutePosition - spk::IVector3<TVectorType>(_position.x * SizeX, _position.y * SizeY, 0));
			}

            /**
             * @brief Converts an absolute world position to a relative chunk position.
             * 
             * This template method converts a 2D world position to a 2D position relative to the chunk's origin.
             * 
             * @param p_absolutePosition The absolute world position as an IVector2.
             * @return The relative position within the chunk as an IVector2.
             */
			template<typename TVectorType>
			spk::IVector2<TVectorType> convertAbsoluteToRelativePosition(const spk::IVector2<TVectorType>& p_absolutePosition) const
			{
				return (convertAbsoluteToRelativePosition(spk::IVector3<TVectorType>(p_absolutePosition, 0.0f)).xy());
			}
		};
	private:
		std::map<spk::Vector2Int, std::unique_ptr<IChunk>> _chunks;

		spk::Vector2Int _activeChunkStart;
		spk::Vector2Int _activeChunkEnd;
		std::vector<IChunk*> _activeChunks;

		virtual IChunk* _instanciateNewChunk(const spk::Vector2Int& p_chunkPosition) = 0;

	protected:
		/**
         * @brief Provides access to the internal chunks map.
         * @return A reference to the map of chunks.
         */
		std::map<spk::Vector2Int, std::unique_ptr<IChunk>>& chunks()
		{
			return (_chunks);
		}

	public:
		/**
         * @brief Constructs an ITilemap object with a name and an optional parent.
         * @param p_name The name of the tilemap.
         * @param p_parent An optional pointer to a parent GameObject.
         */
		ITilemap(const std::string& p_name, spk::GameObject* p_parent = nullptr) : 
			spk::GameObject(p_name, p_parent)
		{
			
		}

		/**
         * @brief Provides read-only access to the internal chunks map.
         * @return A const reference to the map of chunks.
         */
		const std::map<spk::Vector2Int, std::unique_ptr<IChunk>>& chunks() const
		{
			return (_chunks);
		}

        /**
         * @brief Converts a world position to a chunk position in the tilemap.
         * 
         * This static method converts a 2D world position to the corresponding 2D chunk position
         * in the tilemap by dividing the world coordinates by the chunk size.
         * 
         * @param p_worldPosition The world position to be converted.
         * @return The corresponding chunk position as a Vector2Int.
         */
		static spk::Vector2Int convertWorldToChunkPosition(const spk::Vector2Int& p_worldPosition)
		{
			return (spk::Vector2Int(
				std::floorf(p_worldPosition.x / static_cast<float>(IChunk::SizeX)),
				std::floorf(p_worldPosition.y / static_cast<float>(IChunk::SizeY))
			));
		}

        /**
         * @brief Converts a 3D world position to a 2D chunk position using the X and Y coordinates.
         * 
         * This method is useful when the Z coordinate is not relevant for the conversion, typically
         * in 2D tilemaps or when working with layers.
         * 
         * @param p_worldPosition The 3D world position to be converted.
         * @return The corresponding 2D chunk position as a Vector2Int.
         */
		static spk::Vector2Int convertWorldToChunkPositionXY(const spk::Vector3Int& p_worldPosition)
		{
			return (spk::Vector2Int(
				std::floorf(p_worldPosition.x / static_cast<float>(IChunk::SizeX)),
				std::floorf(p_worldPosition.y / static_cast<float>(IChunk::SizeY))
			));
		}

        /**
         * @brief Converts a 3D world position to a 2D chunk position using the X and Z coordinates.
         * 
         * This method is useful in scenarios where the height (Y coordinate) is not relevant for the
         * chunk position, such as in top-down or side-view tilemaps.
         * 
         * @param p_worldPosition The 3D world position to be converted.
         * @return The corresponding 2D chunk position as a Vector2Int.
         */
		static spk::Vector2Int convertWorldToChunkPositionXZ(const spk::Vector3Int& p_worldPosition)
		{
			return (spk::Vector2Int(
				std::floorf(p_worldPosition.x / static_cast<float>(IChunk::SizeX)),
				std::floorf(p_worldPosition.z / static_cast<float>(IChunk::SizeZ))
			));
		}

        /**
         * @brief Checks if a chunk exists at the specified position in the tilemap.
         * 
         * @param p_chunkPosition The position of the chunk to check.
         * @return True if the chunk exists, false otherwise.
         */
		bool containsChunk(const spk::Vector2Int& p_chunkPosition) const
		{
			return (_chunks.contains(p_chunkPosition));
		}

        /**
         * @brief Creates a new chunk at the specified position if it does not already exist.
         * 
         * This method delegates the actual chunk creation to the _instanciateNewChunk method, which
         * should be implemented by derived classes to instantiate their specific chunk types.
         * 
         * @param p_chunkPosition The position where the chunk should be created.
         * @return A pointer to the newly created or existing chunk.
         */
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

        /**
         * @brief Retrieves a pointer to the chunk at the specified position.
         * 
         * @param p_chunkPosition The position of the chunk to retrieve.
         * @return A pointer to the chunk if it exists, nullptr otherwise.
         */
		IChunk* chunk(const spk::Vector2Int& p_chunkPosition)
		{
			if (containsChunk(p_chunkPosition) == false)
				return (nullptr);
			return (_chunks.at(p_chunkPosition).get());
		}

        /**
         * @brief Retrieves a const pointer to the chunk at the specified position.
         * 
         * @param p_chunkPosition The position of the chunk to retrieve.
         * @return A const pointer to the chunk if it exists, nullptr otherwise.
         */
		const IChunk* chunk(const spk::Vector2Int& p_chunkPosition) const
		{
			if (containsChunk(p_chunkPosition) == false)
				return (nullptr);
			return (_chunks.at(p_chunkPosition).get());
		}

        /**
         * @brief Rebakes the chunk at the specified position and its adjacent chunks.
         * 
         * This method is used to update the chunks' graphical representation by triggering
         * a re-baking process. It affects the specified chunk and its immediate neighbors.
         * 
         * @param p_chunkPosition The position of the chunk to rebake.
         */
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

        /**
         * @brief Saves the tilemap's chunks to a specified folder.
         * 
         * Each chunk is saved to a separate file within the folder. The file names are generated
         * based on the chunk's position to ensure uniqueness.
         * 
         * @param p_folderPath The path to the folder where the chunks should be saved.
         */
		void saveToFolder(const std::filesystem::path& p_folderPath) const
		{
			for (auto& [key, element] : _chunks)
			{
				element->getComponent<IChunk>()->saveToFile(p_folderPath.string() + "/chunk_" + std::to_string(key.x) + "_" + std::to_string(key.y) + ".chk");
			}
		}

        /**
         * @brief Activates the chunk at the specified position.
         * 
         * Activation typically involves making the chunk visible or updating its state to be included in game logic updates.
         * 
         * @param p_chunkPosition The position of the chunk to activate.
         */
		void activateChunk(const spk::Vector2Int& p_chunkPosition)
		{
			_chunks[p_chunkPosition]->activate();
		}

        /**
         * @brief Deactivates the chunk at the specified position.
         * 
         * Deactivation typically involves hiding the chunk or excluding it from game logic updates.
         * 
         * @param p_chunkPosition The position of the chunk to deactivate.
         */
		void deactivateChunk(const spk::Vector2Int& p_chunkPosition)
		{
			_chunks[p_chunkPosition]->deactivate();
		}

		/**
		 * @brief Sets the range of active chunks in the tilemap, defining the area that should be updated and rendered.
		 *
		 * @param p_activeChunkStart The starting position of the active chunk range.
		 * @param p_activeChunkEnd The ending position of the active chunk range.
		 */
		void setActiveChunkRange(const spk::Vector2Int& p_activeChunkStart, const spk::Vector2Int& p_activeChunkEnd)
		{
			_activeChunkStart = p_activeChunkStart;
			_activeChunkEnd = p_activeChunkEnd;
		}

		/**
		 * @brief Return the list of active chunks
		 * 
		 * @return The list of active chunk
		*/
		const std::vector<IChunk*>& activeChunks() const
		{
			return (_activeChunks);
		}

		/**
		 * @brief Calculates which chunks are missing within the active range and should be loaded or generated.
		 *
		 * @return A vector containing the positions of the missing chunks.
		 */
		std::vector<spk::Vector2Int> missingChunks() const
		{
			std::vector<spk::Vector2Int> result;

			for (int x = _activeChunkStart.x; x <= _activeChunkEnd.x; x++)
			{
				for (int y = _activeChunkStart.y; y <= _activeChunkEnd.y; y++)
				{
					spk::Vector2Int chunkPosition = spk::Vector2Int(x, y);
					if (containsChunk(chunkPosition) == false)
						result.push_back(chunkPosition);
				}
			}

			return (result);
		}

		/**
		 * @brief Updates the active status of chunks within the specified range, activating or deactivating them as needed.
		 */
		void updateActiveChunks()
		{
			std::vector<IChunk*> chunksToActivate;

			for (int x = _activeChunkStart.x; x <= _activeChunkEnd.x; x++)
			{
				for (int y = _activeChunkStart.y; y <= _activeChunkEnd.y; y++)
				{
					IChunk* chunkToActivate = chunk(spk::Vector2Int(x, y));

					if (chunkToActivate != nullptr)
					{
						chunksToActivate.push_back(chunkToActivate);
					}
				}
			}

			for (auto& element : _activeChunks)
			{
				if (std::find(chunksToActivate.begin(), chunksToActivate.end(), element) == chunksToActivate.end())
					element->deactivate();
			}

			_activeChunks = chunksToActivate;
			for (auto& element : _activeChunks)
			{
				if (element->isActive() == false)
				{
					element->activate();
				}
			}
		}
	};
}