#pragma once

#include <array>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_entity.hpp"
#include "structure/engine/spk_mesh_2d.hpp"
#include "structure/engine/spk_mesh_2d_renderer.hpp"
#include "structure/engine/spk_tile.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/spk_safe_pointer.hpp"
#include "structure/system/spk_exception.hpp"
#include "utils/spk_string_utils.hpp"

namespace spk
{
	template <size_t ChunkSizeX, size_t ChunkSizeY, size_t LayerCount, typename TFlagEnum>
	class TileMap : public spk::Entity
	{
	public:
		using TileType = spk::Tile<TFlagEnum>;

		class Chunk : public spk::Entity
		{
		public:
			static inline const spk::Vector2Int size = spk::Vector2Int(ChunkSizeX, ChunkSizeY);

		private:
			class Data : public spk::Component
			{
			private:
				spk::SafePointer<Mesh2DRenderer> _renderer;
				spk::SafePointer<TileMap> _tileMap;
				spk::Vector2Int _chunkCoordinate = 0;
				std::array<std::array<std::array<typename TileType::ID, LayerCount>, ChunkSizeX>, ChunkSizeY> _content;

				bool _isBaked = false;
				spk::Mesh2D _mesh;

				static spk::Vector2Int _autoTileOffset(unsigned char p_mask)
				{
					static const std::array<spk::Vector2Int, 16> offsets = {
						spk::Vector2Int(1, 4),
						spk::Vector2Int(1, 3),
						spk::Vector2Int(2, 4),
						spk::Vector2Int(2, 3),
						spk::Vector2Int(1, 5),
						spk::Vector2Int(1, 4),
						spk::Vector2Int(2, 5),
						spk::Vector2Int(2, 4),
						spk::Vector2Int(0, 4),
						spk::Vector2Int(0, 3),
						spk::Vector2Int(3, 4),
						spk::Vector2Int(3, 3),
						spk::Vector2Int(0, 5),
						spk::Vector2Int(0, 4),
						spk::Vector2Int(3, 5),
						spk::Vector2Int(3, 4)};
					return offsets[p_mask];
				}

				void _bake()
				{
					_mesh.clear();
					for (int layer = 0; layer < static_cast<int>(LayerCount); ++layer)
					{
						for (int y = 0; y < size.y; ++y)
						{
							for (int x = 0; x < size.x; ++x)
							{
								typename TileType::ID &current = _content[x][y][layer];
								if (current != -1)
								{
									spk::SafePointer<const TileType> tile = _tileMap->tileById(current);
									if (tile == nullptr)
									{
										continue;
									}
									spk::Vector2Int spriteCoord = tile->spriteAnchor();
									if (tile->type() == TileType::Type::Autotile)
									{
										spk::Vector2Int base = _chunkCoordinate * size + spk::Vector2Int(x, y);
										bool up = (_tileMap->content(base + spk::Vector2Int(0, 1), layer) == current);
										bool right = (_tileMap->content(base + spk::Vector2Int(1, 0), layer) == current);
										bool down = (_tileMap->content(base + spk::Vector2Int(0, -1), layer) == current);
										bool left = (_tileMap->content(base + spk::Vector2Int(-1, 0), layer) == current);
										unsigned char mask = (up ? 1 : 0) | (right ? 2 : 0) | (down ? 4 : 0) | (left ? 8 : 0);
										spriteCoord += _autoTileOffset(mask);
									}
									const auto &sprite = _tileMap->spriteSheet()->sprite(spk::Vector2UInt(spriteCoord));
									_mesh.addShape(
										{{spk::Vector2(x, y), sprite.anchor},
										 {spk::Vector2(x + 1.0f, y), {sprite.anchor.x + sprite.size.x, sprite.anchor.y}},
										 {spk::Vector2(x + 1.0f, y + 1.0f), sprite.anchor + sprite.size},
										 {spk::Vector2(x, y + 1.0f), {sprite.anchor.x, sprite.anchor.y + sprite.size.y}}});
								}
							}
						}
					}
					_isBaked = true;
				}

			public:
				Data(const std::wstring &p_name) :
					spk::Component(p_name)
				{
				}

				void setTileMap(spk::SafePointer<TileMap> p_tileMap)
				{
					_tileMap = p_tileMap;
				}

				void setChunkCoordinate(const spk::Vector2Int &p_coord)
				{
					_chunkCoordinate = p_coord;
				}

				void fill(typename TileType::ID p_id)
				{
					for (int l = 0; l < static_cast<int>(LayerCount); ++l)
					{
						for (int y = 0; y < size.y; ++y)
						{
							for (int x = 0; x < size.x; ++x)
							{
								_content[x][y][l] = p_id;
							}
						}
					}
					_isBaked = false;
				}

				void setContent(int p_x, int p_y, int p_layer, typename TileType::ID p_id)
				{
					_content[p_x][p_y][p_layer] = p_id;
					_isBaked = false;
				}

				typename TileType::ID content(int p_x, int p_y, int p_layer) const
				{
					if (p_x < 0 || p_x >= size.x || p_y < 0 || p_y >= size.y || p_layer < 0 || p_layer >= static_cast<int>(LayerCount))
					{
						return -1;
					}
					return (_content[p_x][p_y][p_layer]);
				}

				void start() override
				{
					_renderer = owner()->template getComponent<Mesh2DRenderer>();
				}

				void onPaintEvent(spk::PaintEvent &p_event) override
				{
					if (_isBaked == false)
					{
						_bake();
						if (_renderer != nullptr)
						{
							_renderer->setMesh(mesh());
							p_event.requestPaint();
						}
					}
				}

				bool isBaked() const
				{
					return _isBaked;
				}

				spk::SafePointer<spk::Mesh2D> mesh()
				{
					return (&_mesh);
				}

				const spk::SafePointer<const spk::Mesh2D> mesh() const
				{
					return (&_mesh);
				}
			};

			spk::SafePointer<Mesh2DRenderer> _renderer;
			spk::SafePointer<Data> _data;

		public:
			Chunk(const std::wstring &p_name, spk::SafePointer<TileMap> p_parent, const spk::Vector2Int &p_coord) :
				spk::Entity(p_name, p_parent)
			{
				_renderer = this->template addComponent<Mesh2DRenderer>(p_name + L"/Mesh2DRenderer");
				_data = this->template addComponent<Data>(p_name + L"/Data");
				_data->setTileMap(p_parent);
				_data->setChunkCoordinate(p_coord);
				_renderer->setPriority(100);
				_data->setPriority(0);
			}

			void setSpriteSheet(spk::SafePointer<const spk::SpriteSheet> p_spriteSheet)
			{
				_renderer->setTexture(p_spriteSheet);
			}

			void fill(typename TileType::ID p_id)
			{
				_data->fill(p_id);
			}

			void setContent(spk::Vector2Int p_position, int p_layer, typename TileType::ID p_id)
			{
				setContent(p_position.x, p_position.y, p_layer, p_id);
			}

			void setContent(int p_x, int p_y, int p_layer, typename TileType::ID p_id)
			{
				_data->setContent(p_x, p_y, p_layer, p_id);
			}

			typename TileType::ID content(spk::Vector2Int p_pos, int p_layer) const
			{
				return _data->content(p_pos.x, p_pos.y, p_layer);
			}

			bool isBaked() const
			{
				return _data->isBaked();
			}

			spk::SafePointer<spk::Mesh2D> mesh()
			{
				return _data->mesh();
			}

			const spk::SafePointer<const spk::Mesh2D> mesh() const
			{
				return _data->mesh();
			}
		};

	private:
		spk::SafePointer<const spk::SpriteSheet> _spriteSheet;

		std::unordered_map<typename TileType::ID, std::unique_ptr<TileType>> _availableTiles;
		std::unordered_map<spk::Vector2Int, std::unique_ptr<Chunk>> _chunks;

		std::vector<spk::SafePointer<Chunk>> _activeChunks;

		std::unique_ptr<Chunk> _generateChunk(const spk::Vector2Int &p_chunkCoordinate)
		{
			const std::wstring chunkName = name() + L"/Chunk[" + p_chunkCoordinate.to_wstring() + L"]";

			std::unique_ptr<Chunk> newChunk = std::make_unique<Chunk>(chunkName, this, p_chunkCoordinate);

			newChunk->transform().place(spk::Vector3(p_chunkCoordinate.x * Chunk::size.x, p_chunkCoordinate.y * Chunk::size.y, 0));
			newChunk->setSpriteSheet(_spriteSheet);
			newChunk->fill(-1);

			_onChunkGeneration(p_chunkCoordinate, *newChunk);

			newChunk->activate();

			return newChunk;
		}

		virtual void _onChunkGeneration(const spk::Vector2Int &p_chunkCoordinate, Chunk &p_chunkToFill)
		{
		}

	public:
		TileMap(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
			spk::Entity(p_name, p_parent)
		{
		}

		void setSpriteSheet(spk::SafePointer<const spk::SpriteSheet> p_spriteSheet)
		{
			_spriteSheet = p_spriteSheet;

			for (auto &[key, value] : _chunks)
			{
				value->setSpriteSheet(_spriteSheet);
			}
		}

		const spk::SafePointer<const spk::SpriteSheet> &spriteSheet() const
		{
			return _spriteSheet;
		}

		void addTileByID(const typename TileType::ID &p_id, const TileType &p_tile)
		{
			if (_availableTiles.contains(p_id) == true)
			{
				GENERATE_ERROR("Tile ID [" + std::to_string(p_id) + "] already exist in TileMap [" + spk::StringUtils::wstringToString(name()) + "]");
			}
			_availableTiles[p_id] = std::make_unique<TileType>(p_tile);
		}

		spk::SafePointer<const TileType> tileById(typename TileType::ID p_id) const
		{
			if (_availableTiles.contains(p_id) == false)
			{
				return (nullptr);
			}
			return (_availableTiles.at(p_id).get());
		}

		typename TileType::ID content(spk::Vector2Int p_global, int p_layer) const
		{
			spk::Vector2Int chunkCoord = {p_global.x / Chunk::size.x, p_global.y / Chunk::size.y};
			if (_chunks.contains(chunkCoord) == false)
			{
				return -1;
			}
			spk::Vector2Int local = {p_global.x - chunkCoord.x * Chunk::size.x, p_global.y - chunkCoord.y * Chunk::size.y};
			return _chunks.at(chunkCoord)->content(local, p_layer);
		}

		void setChunkRange(const spk::Vector2Int &p_start, const spk::Vector2Int &p_end)
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
					spk::Vector2Int chunkPos = {i, j};
					if (_chunks.contains(chunkPos) == false)
					{
						_chunks.emplace(chunkPos, std::move(_generateChunk(chunkPos)));
					}

					_activeChunks.push_back((_chunks[chunkPos].get()));
				}
			}

			for (auto &chunk : _activeChunks)
			{
				chunk->activate();
			}
		}
	};
}
