#pragma once

#include <array>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "structure/engine/spk_collision_mesh.hpp"
#include "structure/engine/spk_collision_mesh_renderer.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_entity.hpp"
#include "structure/engine/spk_obj_mesh.hpp"
#include "structure/engine/spk_obj_mesh_renderer.hpp"
#include "structure/engine/spk_rigid_body.hpp"
#include "structure/engine/spk_block.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/spk_safe_pointer.hpp"
#include "structure/system/spk_exception.hpp"

namespace spk
{
	template <size_t ChunkSizeX, size_t ChunkSizeY, size_t ChunkSizeZ>
	class BlockMap : public spk::Entity
	{
	public:
		class Chunk : public spk::Entity
		{
		public:
			static inline const spk::Vector3Int size = spk::Vector3Int(ChunkSizeX, ChunkSizeY, ChunkSizeZ);

		private:
			class Data : public spk::Component
			{
			private:
				spk::SafePointer<spk::ObjMeshRenderer> _renderer;
				spk::SafePointer<spk::CollisionMeshRenderer> _collisionRenderer;
				spk::SafePointer<spk::RigidBody> _rigidBody;

				spk::SafePointer<BlockMap> _blockMap;
				std::array<std::array<std::array<Block::Specifier, ChunkSizeX>, ChunkSizeY>, ChunkSizeZ> _content;

				bool _isBaked = false;
				spk::ObjMesh _mesh;
				spk::CollisionMesh _collisionMesh;

				Block::NeightbourDescriber _computeNeightbourSpecifiers(int p_x, int p_y, int p_z)
				{
					Block::NeightbourDescriber result;

					for (size_t i = 0; i < Block::neightbourCoordinates.size(); i++)
					{
						spk::Vector3 tmpCoord = Block::neightbourCoordinates[i] + spk::Vector3{p_x, p_y, p_z};

						Block::Specifier specifier = content(tmpCoord);

						spk::SafePointer<const Block> block = nullptr;
						if (specifier.first != -1)
						{
							block = _blockMap->blockById(specifier.first);
						}

						result[i] = std::make_pair(block, specifier.second);
					}

					return (result);
				}

				void _bake()
				{
					_mesh.clear();

					for (int z = 0; z < size.z; ++z)
					{
						for (int y = 0; y < size.y; ++y)
						{
							for (int x = 0; x < size.x; ++x)
							{
								Block::Specifier &currentSpecifier = _content[x][y][z];

								if (currentSpecifier.first != -1)
								{
									Block::NeightbourDescriber neightbourSpecifiers = _computeNeightbourSpecifiers(x, y, z);

									spk::SafePointer<const Block> currentBlock = _blockMap->blockById(currentSpecifier.first);

									try
									{
										currentBlock->bake(_mesh, neightbourSpecifiers, {x, y, z}, currentSpecifier.second);
									} catch (const std::exception &e)
									{
										PROPAGATE_ERROR(
											"Failed to bake chunk block ID [" + std::to_string(currentSpecifier.first) + "] at pos [" +
												std::to_string(x) + "][" + std::to_string(y) + "][" + std::to_string(z) + "]",
											e);
									}
								}
							}
						}
					}

					try
					{
						_collisionMesh = spk::CollisionMesh::fromObjMesh(&_mesh);
					} catch (const std::exception &e)
					{
						PROPAGATE_ERROR("Failed to bake chunk while constructing collision mesh", e);
					}

					_isBaked = true;
				}

			public:
				Data(const std::wstring &p_name) :
					spk::Component(p_name)
				{
				}

				void setBlockMap(spk::SafePointer<BlockMap> p_blockMap)
				{
					_blockMap = p_blockMap;
				}

				void fill(Block::ID p_id)
				{
					for (int z = 0; z < size.z; ++z)
					{
						for (int y = 0; y < size.y; ++y)
						{
							for (int x = 0; x < size.x; ++x)
							{
								_content[x][y][z] = std::make_pair(
									p_id, Block::Orientation{Block::HorizontalOrientation::XPositive, Block::VerticalOrientation::YPositive});
							}
						}
					}
					_isBaked = false;
				}

				void setContent(
					int p_x,
					int p_y,
					int p_z,
					const Block::ID &p_data,
					const Block::Orientation &p_orientation = {Block::HorizontalOrientation::XPositive, Block::VerticalOrientation::YPositive})
				{
					_content[p_x][p_y][p_z] = std::make_pair(p_data, p_orientation);
					_isBaked = false;
				}

				Block::Specifier content(const spk::Vector3Int &p_coord) const
				{
					return (content(p_coord.x, p_coord.y, p_coord.z));
				}

				Block::Specifier content(const spk::Vector2Int &p_coord, int p_z) const
				{
					return (content(p_coord.x, p_coord.y, p_z));
				}

				Block::Specifier content(int p_x, int p_y, int p_z) const
				{
					if (p_x < 0 || p_x >= size.x || p_y < 0 || p_y >= size.y || p_z < 0 || p_z >= size.z)
					{
						return std::make_pair(-1, Block::Orientation{});
					}
					return (_content[p_x][p_y][p_z]);
				}

				void start() override
				{
					_renderer = owner()->template getComponent<spk::ObjMeshRenderer>();
					_collisionRenderer = owner()->template getComponent<spk::CollisionMeshRenderer>();
					_rigidBody = owner()->template getComponent<spk::RigidBody>();
				}

				void onPaintEvent(spk::PaintEvent &p_event) override
				{
					if (_isBaked == false)
					{
						_bake();

						if (_renderer != nullptr)
						{
							_renderer->setMesh(mesh());
							_collisionRenderer->setMesh(collisionMesh());
							_rigidBody->setCollider(collisionMesh());
							p_event.requestPaint();
						}
					}
				}

				spk::SafePointer<spk::ObjMesh> mesh()
				{
					return (&_mesh);
				}
				const spk::SafePointer<const spk::ObjMesh> mesh() const
				{
					return (&_mesh);
				}

				spk::SafePointer<spk::CollisionMesh> collisionMesh()
				{
					return (&_collisionMesh);
				}
				const spk::SafePointer<const spk::CollisionMesh> collisionMesh() const
				{
					return (&_collisionMesh);
				}
			};

			spk::SafePointer<spk::ObjMeshRenderer> _renderer;
			spk::SafePointer<spk::CollisionMeshRenderer> _collisionRenderer;
			spk::SafePointer<spk::RigidBody> _rigidBody;
			spk::SafePointer<Data> _data;

		public:
			Chunk(const std::wstring &p_name, spk::SafePointer<BlockMap> p_parent) :
				spk::Entity(p_name, p_parent)
			{
				_renderer = this->template addComponent<spk::ObjMeshRenderer>(p_name + L"/ObjMeshRenderer");
				_collisionRenderer = this->template addComponent<spk::CollisionMeshRenderer>(p_name + L"/CollisionMeshRenderer");
				_rigidBody = this->template addComponent<spk::RigidBody>(p_name + L"/RigidBody");
				_data = this->template addComponent<Data>(p_name + L"/Data");
				_data->setBlockMap(p_parent);

				_renderer->setPriority(100);
				_collisionRenderer->setPriority(100);
				_collisionRenderer->setWireframe(true);
				_collisionRenderer->deactivate();
				_data->setPriority(0);
			}

			void setTexture(spk::SafePointer<const spk::Texture> p_texture)
			{
				_renderer->setTexture(p_texture);
			}

			void fill(Block::ID p_id)
			{
				_data->fill(p_id);
			}

			void setContent(
				spk::Vector3Int p_position,
				Block::ID p_id,
				const Block::Orientation &p_orientation = {Block::HorizontalOrientation::XPositive, Block::VerticalOrientation::YPositive})
			{
				setContent(p_position.x, p_position.y, p_position.z, p_id, p_orientation);
			}
			void setContent(
				spk::Vector2Int p_position,
				int p_z,
				Block::ID p_id,
				const Block::Orientation &p_orientation = {Block::HorizontalOrientation::XPositive, Block::VerticalOrientation::YPositive})
			{
				setContent(p_position.x, p_position.y, p_z, p_id, p_orientation);
			}

			void setContent(
				int p_x,
				int p_y,
				int p_z,
				Block::ID p_id,
				const Block::Orientation &p_orientation = {Block::HorizontalOrientation::XPositive, Block::VerticalOrientation::YPositive})
			{
				_data->setContent(p_x, p_y, p_z, p_id, p_orientation);
			}

			bool isBaked() const
			{
				return _data->isBaked();
			}

			spk::SafePointer<spk::ObjMesh> mesh()
			{
				return (_data->mesh());
			}
			const spk::SafePointer<const spk::ObjMesh> mesh() const
			{
				return (_data->mesh());
			}

			void useCollisionRenderer(bool p_use)
			{
				if (p_use == true)
				{
					_renderer->deactivate();
					_collisionRenderer->activate();
				}
				else
				{
					_collisionRenderer->deactivate();
					_renderer->activate();
				}
			}
		};

	private:
		spk::SafePointer<const spk::Texture> _texture;

		std::unordered_map<Block::ID, std::unique_ptr<Block>> _availableBlocks;
		std::unordered_map<spk::Vector3Int, std::unique_ptr<Chunk>> _chunks;

		std::vector<spk::SafePointer<Chunk>> _activeChunks;

		bool _useCollisionRenderer = false;

		std::unique_ptr<Chunk> _generateChunk(const spk::Vector3Int &p_chunkCoordinate)
		{
			const std::wstring chunkName = name() + L"/Chunk[" + p_chunkCoordinate.to_wstring() + L"]";

			std::unique_ptr<Chunk> newChunk = std::make_unique<Chunk>(chunkName, this);

			newChunk->transform().place(p_chunkCoordinate * Chunk::size);
			newChunk->setTexture(_texture);
			newChunk->fill(-1);

			_onChunkGeneration(p_chunkCoordinate, *newChunk);

			newChunk->activate();

			newChunk->useCollisionRenderer(_useCollisionRenderer);

			return newChunk;
		}

		virtual void _onChunkGeneration(const spk::Vector3Int &p_chunkCoordinate, Chunk &p_chunkToFill)
		{
		}

	public:
		BlockMap(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
			spk::Entity(p_name, p_parent)
		{
		}

		void setTexture(spk::SafePointer<const spk::Texture> p_texture)
		{
			_texture = p_texture;

			for (auto &[key, value] : _chunks)
			{
				value->setTexture(_texture);
			}
		}

		void addBlockByID(const Block::ID &p_id, std::unique_ptr<Block> &&p_block)
		{
			if (_availableBlocks.contains(p_id) == true)
			{
				GENERATE_ERROR(
					"Block ID [" + std::to_string(p_id) + "] already exist in BlockMap [" + spk::StringUtils::wstringToString(name()) + "]");
			}
			_availableBlocks[p_id] = std::move(p_block);
		}

		spk::SafePointer<const Block> blockById(Block::ID p_id) const
		{
			if (_availableBlocks.contains(p_id) == false)
			{
				return (nullptr);
			}
			return (_availableBlocks.at(p_id).get());
		}

		void setChunkRange(const spk::Vector3Int &p_start, const spk::Vector3Int &p_end)
		{
			for (auto &chunk : _activeChunks)
			{
				chunk->deactivate();
			}
			_activeChunks.clear();

			for (int i = p_start.x; i <= p_end.x; i++)
			{
				for (int j = p_start.y; j <= p_end.y; j++)
				{
					for (int k = p_start.z; k <= p_end.z; k++)
					{
						spk::Vector3Int chunkPosition = {i, j, k};
						if (_chunks.contains(chunkPosition) == false)
						{
							_chunks.emplace(chunkPosition, std::move(_generateChunk(chunkPosition)));
						}

						_activeChunks.push_back((_chunks[chunkPosition].get()));
					}
				}
			}

			for (auto &chunk : _activeChunks)
			{
				chunk->activate();
				chunk->useCollisionRenderer(_useCollisionRenderer);
			}
		}
		void useCollisionRenderer(bool p_use)
		{
			_useCollisionRenderer = p_use;
			for (auto &chunk : _activeChunks)
			{
				chunk->useCollisionRenderer(_useCollisionRenderer);
			}
		}
	};

} // namespace spk
