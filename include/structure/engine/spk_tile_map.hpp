#pragma once

#include <array>
#include <cmath>
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
	template <size_t ChunkSizeX, size_t ChunkSizeY, size_t TLayerCount, typename TFlagEnum>
	class TileMap : public spk::Entity
	{
	public:
		using TileType = spk::Tile<TFlagEnum>;
		static constexpr size_t LayerCount = TLayerCount;

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

                enum class Corner
                {
                    TopLeft = 0,
                    TopRight = 1,
                    BottomRight = 2,
                    BottomLeft = 3
                };

                // For each corner, define the relative offsets to query neighbours:
                // order is [sideA, sideB, diagonal].
                static const std::map<Corner, std::array<spk::Vector2Int, 3>> &_cornerNeighbourOffsets()
                {
                    static const std::map<Corner, std::array<spk::Vector2Int, 3>> kOffsets = {
                        {Corner::TopLeft, {spk::Vector2Int(0, 1), spk::Vector2Int(-1, 0), spk::Vector2Int(-1, 1)}},
                        {Corner::TopRight, {spk::Vector2Int(0, 1), spk::Vector2Int(1, 0), spk::Vector2Int(1, 1)}},
                        {Corner::BottomRight, {spk::Vector2Int(0, -1), spk::Vector2Int(1, 0), spk::Vector2Int(1, -1)}},
                        {Corner::BottomLeft, {spk::Vector2Int(0, -1), spk::Vector2Int(-1, 0), spk::Vector2Int(-1, -1)}}};
                    return (kOffsets);
                }

                static unsigned int _cornerRow(const Corner &p_corner)
                {
                    switch (p_corner)
                    {
                    case Corner::TopLeft:
                        return (0);
                    case Corner::TopRight:
                        return (1);
                    case Corner::BottomRight:
                        return (2);
                    case Corner::BottomLeft:
                        return (3);
                    }
                    return (0);
                }

                // Mapping from (corner, [sideA, sideB, diag]) to sprite offset relative to the tile's spriteAnchor.
                static const std::map<Corner, std::map<std::array<bool, 3>, spk::Vector2UInt>> &_cornerOffsetMap()
                {
                    static const std::map<Corner, std::map<std::array<bool, 3>, spk::Vector2UInt>> kMap = []() {
                        std::map<Corner, std::map<std::array<bool, 3>, spk::Vector2UInt>> m;
                        auto fillForRow = [&](Corner p_corner) {
                            unsigned int row = _cornerRow(p_corner);
                            auto &inner = m[p_corner];
                            auto add = [&](bool a, bool b, bool d, unsigned int col) {
                                inner[{a, b, d}] = spk::Vector2UInt(col, row);
                            };
                            // Enumerate all 8 combinations for (a,b,d)
                            // Policy: column selection by a/b; diag only refines 'both' if desired.
                            // none
                            add(false, false, false, 0);
                            add(false, false, true, 0);
                            // sideA only
                            add(true, false, false, 1);
                            add(true, false, true, 1);
                            // sideB only
                            add(false, true, false, 2);
                            add(false, true, true, 2);
                            // both sides
                            add(true, true, false, 3);
                            add(true, true, true, 3);
                        };
                        fillForRow(Corner::TopLeft);
                        fillForRow(Corner::TopRight);
                        fillForRow(Corner::BottomRight);
                        fillForRow(Corner::BottomLeft);
                        return m;
                    }();
                    return (kMap);
                }

                // Returns [sideA, sideB, diagonal] state for the given corner
                std::array<bool, 3> _getNeightbourState(const spk::Vector2Int &p_globalTileCoord,
                                                        int p_layer,
                                                        typename TileType::ID p_currentId,
                                                        const Corner &p_corner) const
                {
                    const auto &cornerOffsets = _cornerNeighbourOffsets().at(p_corner);
                    std::array<bool, 3> state = {false, false, false};
                    // sideA
                    state[0] = (_tileMap->content(p_globalTileCoord + cornerOffsets[0], p_layer) == p_currentId);
                    // sideB
                    state[1] = (_tileMap->content(p_globalTileCoord + cornerOffsets[1], p_layer) == p_currentId);
                    // diagonal
                    state[2] = (_tileMap->content(p_globalTileCoord + cornerOffsets[2], p_layer) == p_currentId);
                    return (state);
                }

                void _bakeMonoTile(int p_x, int p_y, int p_layer, const TileType &p_tile)
                {
                    const auto &sprite = _tileMap->spriteSheet()->sprite(p_tile.spriteAnchor());
                    _mesh.addShape({{spk::Vector2(p_x, p_y), sprite.anchor},
                                    {spk::Vector2(p_x + 1.0f, p_y), {sprite.anchor.x + sprite.size.x, sprite.anchor.y}},
                                    {spk::Vector2(p_x + 1.0f, p_y + 1.0f), sprite.anchor + sprite.size},
                                    {spk::Vector2(p_x, p_y + 1.0f), {sprite.anchor.x, sprite.anchor.y + sprite.size.y}}});
                }

                void _bakeAutotile(int p_x, int p_y, int p_layer, typename TileType::ID p_currentId, const TileType &p_tile)
                {
                    const auto &sheet = *(_tileMap->spriteSheet());
                    const spk::Vector2UInt &anchor = p_tile.spriteAnchor();
                    const spk::Vector2Int globalBase = _chunkCoordinate * size + spk::Vector2Int(p_x, p_y);

                    // For each corner: TopLeft, TopRight, BottomRight, BottomLeft
                    const std::array<Corner, 4> corners = {Corner::TopLeft, Corner::TopRight, Corner::BottomRight, Corner::BottomLeft};

                    for (const Corner &corner : corners)
                    {
                        std::array<bool, 3> state = _getNeightbourState(globalBase, p_layer, p_currentId, corner);

                        const auto &cornerMap = _cornerOffsetMap().at(corner);
                        spk::Vector2UInt offset = anchor;
                        auto it = cornerMap.find(state);
                        if ((it != cornerMap.end()) == true)
                        {
                            offset += it->second;
                        }
                        else
                        {
                            // Fallback (should not happen): use outer corner column 0 for this row
                            offset += spk::Vector2UInt(0, _cornerRow(corner));
                        }

                        const auto &spr = sheet.sprite(offset);

                        if (corner == Corner::TopLeft)
                        {
                            _mesh.addShape({{spk::Vector2(p_x, p_y + 0.5f), spr.anchor},
                                            {spk::Vector2(p_x + 0.5f, p_y + 0.5f), {spr.anchor.x + spr.size.x, spr.anchor.y}},
                                            {spk::Vector2(p_x + 0.5f, p_y + 1.0f), spr.anchor + spr.size},
                                            {spk::Vector2(p_x, p_y + 1.0f), {spr.anchor.x, spr.anchor.y + spr.size.y}}});
                        }
                        else if (corner == Corner::TopRight)
                        {
                            _mesh.addShape({{spk::Vector2(p_x + 0.5f, p_y + 0.5f), spr.anchor},
                                            {spk::Vector2(p_x + 1.0f, p_y + 0.5f), {spr.anchor.x + spr.size.x, spr.anchor.y}},
                                            {spk::Vector2(p_x + 1.0f, p_y + 1.0f), spr.anchor + spr.size},
                                            {spk::Vector2(p_x + 0.5f, p_y + 1.0f), {spr.anchor.x, spr.anchor.y + spr.size.y}}});
                        }
                        else if (corner == Corner::BottomRight)
                        {
                            _mesh.addShape({{spk::Vector2(p_x + 0.5f, p_y), spr.anchor},
                                            {spk::Vector2(p_x + 1.0f, p_y), {spr.anchor.x + spr.size.x, spr.anchor.y}},
                                            {spk::Vector2(p_x + 1.0f, p_y + 0.5f), spr.anchor + spr.size},
                                            {spk::Vector2(p_x + 0.5f, p_y + 0.5f), {spr.anchor.x, spr.anchor.y + spr.size.y}}});
                        }
                        else
                        {
                            _mesh.addShape({{spk::Vector2(p_x, p_y), spr.anchor},
                                            {spk::Vector2(p_x + 0.5f, p_y), {spr.anchor.x + spr.size.x, spr.anchor.y}},
                                            {spk::Vector2(p_x + 0.5f, p_y + 0.5f), spr.anchor + spr.size},
                                            {spk::Vector2(p_x, p_y + 0.5f), {spr.anchor.x, spr.anchor.y + spr.size.y}}});
                        }
                    }
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
									spk::Vector2UInt spriteCoord = tile->spriteAnchor();
									if (tile->type() == TileType::Type::Autotile)
									{
										spk::Vector2Int base = _chunkCoordinate * size + spk::Vector2Int(x, y);
										bool up = (_tileMap->content(base + spk::Vector2Int(0, 1), layer) == current);
										bool right = (_tileMap->content(base + spk::Vector2Int(1, 0), layer) == current);
										bool down = (_tileMap->content(base + spk::Vector2Int(0, -1), layer) == current);
										bool left = (_tileMap->content(base + spk::Vector2Int(-1, 0), layer) == current);

										// Diagonals
										bool upLeft = (_tileMap->content(base + spk::Vector2Int(-1, 1), layer) == current);
										bool upRight = (_tileMap->content(base + spk::Vector2Int(1, 1), layer) == current);
										bool downRight = (_tileMap->content(base + spk::Vector2Int(1, -1), layer) == current);
										bool downLeft = (_tileMap->content(base + spk::Vector2Int(-1, -1), layer) == current);

										const auto &sheet = *(_tileMap->spriteSheet());
										// TL quadrant (row 0): sides A=up, B=left, diag=upLeft
										{
											spk::Vector2UInt off = _quarterOffset(up, left, upLeft, 0);
											const auto &spr = sheet.sprite(spriteCoord + off);
											_mesh.addShape(
												{{spk::Vector2(x, y + 0.5f), spr.anchor},
												 {spk::Vector2(x + 0.5f, y + 0.5f), {spr.anchor.x + spr.size.x, spr.anchor.y}},
												 {spk::Vector2(x + 0.5f, y + 1.0f), spr.anchor + spr.size},
												 {spk::Vector2(x, y + 1.0f), {spr.anchor.x, spr.anchor.y + spr.size.y}}});
										}
										// TR quadrant (row 1): sides A=up, B=right, diag=upRight
										{
											spk::Vector2UInt off = _quarterOffset(up, right, upRight, 1);
											const auto &spr = sheet.sprite(spriteCoord + off);
											_mesh.addShape(
												{{spk::Vector2(x + 0.5f, y + 0.5f), spr.anchor},
												 {spk::Vector2(x + 1.0f, y + 0.5f), {spr.anchor.x + spr.size.x, spr.anchor.y}},
												 {spk::Vector2(x + 1.0f, y + 1.0f), spr.anchor + spr.size},
												 {spk::Vector2(x + 0.5f, y + 1.0f), {spr.anchor.x, spr.anchor.y + spr.size.y}}});
										}
										// BR quadrant (row 2): sides A=down, B=right, diag=downRight
										{
											spk::Vector2UInt off = _quarterOffset(down, right, downRight, 2);
											const auto &spr = sheet.sprite(spriteCoord + off);
											_mesh.addShape(
												{{spk::Vector2(x + 0.5f, y), spr.anchor},
												 {spk::Vector2(x + 1.0f, y), {spr.anchor.x + spr.size.x, spr.anchor.y}},
												 {spk::Vector2(x + 1.0f, y + 0.5f), spr.anchor + spr.size},
												 {spk::Vector2(x + 0.5f, y + 0.5f), {spr.anchor.x, spr.anchor.y + spr.size.y}}});
										}
										// BL quadrant (row 3): sides A=down, B=left, diag=downLeft
										{
											spk::Vector2UInt off = _quarterOffset(down, left, downLeft, 3);
											const auto &spr = sheet.sprite(spriteCoord + off);
											_mesh.addShape(
												{{spk::Vector2(x, y), spr.anchor},
												 {spk::Vector2(x + 0.5f, y), {spr.anchor.x + spr.size.x, spr.anchor.y}},
												 {spk::Vector2(x + 0.5f, y + 0.5f), spr.anchor + spr.size},
												 {spk::Vector2(x, y + 0.5f), {spr.anchor.x, spr.anchor.y + spr.size.y}}});
										}

										// Autotile fully handled by quadrants; skip full-tile draw
										continue;
									}

									// Monotile: draw single full-size quad
									const auto &sprite = _tileMap->spriteSheet()->sprite(spriteCoord);
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
			// Compute the chunk coordinate using floor division to properly handle negatives
			const int cx = static_cast<int>(std::floor(static_cast<float>(p_global.x) / static_cast<float>(Chunk::size.x)));
			const int cy = static_cast<int>(std::floor(static_cast<float>(p_global.y) / static_cast<float>(Chunk::size.y)));
			spk::Vector2Int chunkCoord = {cx, cy};

			if (_chunks.contains(chunkCoord) == false)
			{
				return -1;
			}

			// Compute local coordinates with positive modulo so they are in [0, size)
			spk::Vector2Int local = {
				static_cast<int>(spk::positiveModulo(p_global.x, Chunk::size.x)), static_cast<int>(spk::positiveModulo(p_global.y, Chunk::size.y))};
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
