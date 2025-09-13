#pragma once

#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/engine/spk_collision_mesh_2d.hpp"
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

			class Collider : public spk::Component
			{
			private:
				bool _needBaking = true;
				spk::SafePointer<Chunk> _chunk;
				spk::SafePointer<spk::Collider2D> _collider;
				spk::CollisionMesh2D _mesh;
				spk::ContractProvider::Contract _editionContract;
				spk::SafePointer<spk::TileMap<ChunkSizeX, ChunkSizeY, TLayerCount, TFlagEnum>> _tileMap;
				spk::Flags<TFlagEnum> _flags;

				void _bake()
				{
					DEBUG_LINE();
					if (_chunk != nullptr)
					{
						_mesh = _chunk->bakeCollisionMesh(_flags);

						if (_collider != nullptr)
						{
							_collider->setCollisionMesh(&_mesh);
						}
					}
					DEBUG_LINE();
				}

			public:
				Collider(const std::wstring &p_name) :
					spk::Component(p_name)
				{
				}

				void setFlag(spk::Flags<TFlagEnum> p_flags)
				{
					_flags = p_flags;
					_needBaking = true;
				}

				void onUpdateEvent(spk::UpdateEvent &p_event) override
				{
					if (_needBaking == true)
					{
						_bake();

						_needBaking = false;
					}
				}

				void start() override
				{
					_chunk = dynamic_cast<Chunk *>(owner().get());

					if (_chunk == nullptr)
					{
						GENERATE_ERROR("Can't create a Collider on another object type than Chunk");
					}

					_tileMap = _chunk->_tilemap;

					_collider = owner()->template addComponent<spk::Collider2D>(name() + L"/Collider2D");

					_editionContract = _chunk->onEdition([this]() { _needBaking = true; });
					_needBaking = true;
				}
			};

		private:
			class Data : public spk::Component
			{
			private:
				spk::SafePointer<Mesh2DRenderer> _renderer;
				spk::SafePointer<TileMap<ChunkSizeX, ChunkSizeY, TLayerCount, TFlagEnum>> _tileMap;
				spk::Vector2Int _chunkCoordinates = 0;
				std::array<std::array<std::array<typename TileType::ID, LayerCount>, ChunkSizeX>, ChunkSizeY> _content;

				bool _isBaked = false;
				spk::Mesh2D _mesh;

				void _insertData(float p_x, float p_y, float p_width, float p_height, int p_layer, const spk::SpriteSheet::Section &p_sprite)
				{
					// Define quad vertices in counter-clockwise order (BL, BR, TR, TL)
					_mesh.addShape(
						{{spk::Vector2(p_x, p_y), {p_sprite.anchor.x, p_sprite.anchor.y + p_sprite.size.y}},
						 {spk::Vector2(p_x + p_width, p_y), p_sprite.anchor + p_sprite.size},
						 {spk::Vector2(p_x + p_width, p_y + p_height), {p_sprite.anchor.x + p_sprite.size.x, p_sprite.anchor.y}},
						 {spk::Vector2(p_x, p_y + p_height), p_sprite.anchor}});
				}

				void _bakeMonoTile(int p_x, int p_y, int p_layer, const TileType &p_tile)
				{
					const auto &sprite = _tileMap->spriteSheet()->sprite(p_tile.spriteAnchor());
					_insertData(static_cast<float>(p_x), static_cast<float>(p_y), 1.0f, 1.0f, p_layer, sprite);
				}

				enum class Corner
				{
					TopLeft = 0,
					TopRight = 1,
					BottomRight = 2,
					BottomLeft = 3
				};

				std::array<bool, 3> _getNeighbourState(
					const spk::Vector2Int &p_globalTileCoord, int p_layer, typename TileType::ID p_currentId, const Corner &p_corner) const
				{
					static const std::map<Corner, std::array<spk::Vector2Int, 3>> cornerNeighbourOffsets = {
						{Corner::TopLeft, {spk::Vector2Int(0, 1), spk::Vector2Int(-1, 0), spk::Vector2Int(-1, 1)}},
						{Corner::TopRight, {spk::Vector2Int(0, 1), spk::Vector2Int(1, 0), spk::Vector2Int(1, 1)}},
						{Corner::BottomRight, {spk::Vector2Int(0, -1), spk::Vector2Int(1, 0), spk::Vector2Int(1, -1)}},
						{Corner::BottomLeft, {spk::Vector2Int(0, -1), spk::Vector2Int(-1, 0), spk::Vector2Int(-1, -1)}}};

					const auto &cornerOffsets = cornerNeighbourOffsets.at(p_corner);

					std::array<bool, 3> state = {false, false, false};

					state[0] = (_tileMap->content(p_globalTileCoord + cornerOffsets[0], p_layer) != p_currentId);
					state[1] = (_tileMap->content(p_globalTileCoord + cornerOffsets[1], p_layer) != p_currentId);
					state[2] = (_tileMap->content(p_globalTileCoord + cornerOffsets[2], p_layer) != p_currentId);

					return (state);
				}

				spk::Vector2UInt _neightbourOffsetToApply(Corner p_corner, const std::array<bool, 3> &p_neightbourState)
				{
					using OffsetRow = std::array<spk::Vector2UInt, 2>;
					using OffsetMat = std::array<OffsetRow, 2>;
					using OffsetCube = std::array<OffsetMat, 2>;

					// Autotile corner offsets indexed by neighbour state as [a][b][c]:
					//  - a: orthogonal neighbour along the corner's primary axis
					//       Top corners -> a = Up,  Bottom corners -> a = Down
					//  - b: orthogonal neighbour perpendicular to the corner's primary axis
					//       Left corners -> b = Left, Right corners -> b = Right
					//  - c: diagonal neighbour at the corner
					//
					// For each corner, comments on each case show a 2x2 diagram of neighbours:
					// X = tile identical to the target, . = different tile, C = current cell
					//
					// Corner::TopLeft diagram layout:
					//  [c][a]
					//  [b][C]
					// Corner::TopRight diagram layout:
					//  [a][c]
					//  [C][b]
					// Corner::BottomRight diagram layout:
					//  [C][b]
					//  [a][c]
					// Corner::BottomLeft diagram layout:
					//  [b][C]
					//  [c][a]
					static const std::unordered_map<Corner, OffsetCube> offsetsByCornerAndNeightbour = {
						{Corner::TopLeft,
						 OffsetCube{
							 {OffsetMat{
								  {OffsetRow{// a=1, b=1, c=1
											 // XX
											 // XC
											 spk::Vector2UInt(1, 4),
											 // a=1, b=1, c=0
											 // .X
											 // XC
											 spk::Vector2UInt(2, 0)},
								   OffsetRow{// a=1, b=0, c=1
											 // XX
											 // .C
											 spk::Vector2UInt(0, 4),
											 // a=1, b=0, c=0
											 // .X
											 // .C
											 spk::Vector2UInt(0, 3)}}},
							  OffsetMat{
								  {OffsetRow{// a=0, b=1, c=1
											 // X.
											 // XC
											 spk::Vector2UInt(1, 2),
											 // a=0, b=1, c=0
											 // ..
											 // XC
											 spk::Vector2UInt(1, 2)},
								   OffsetRow{// a=0, b=0, c=1
											 // X.
											 // .C
											 spk::Vector2UInt(0, 2),
											 // a=0, b=0, c=0
											 // ..
											 // .C
											 spk::Vector2UInt(0, 0)}}}}}},

						{Corner::TopRight,
						 OffsetCube{
							 {OffsetMat{
								  {OffsetRow{// a=1, b=1, c=1
											 // XX
											 // CX
											 spk::Vector2UInt(2, 4),
											 // a=1, b=1, c=0
											 // X.
											 // CX
											 spk::Vector2UInt(3, 0)},
								   OffsetRow{// a=1, b=0, c=1
											 // XX
											 // C.
											 spk::Vector2UInt(3, 3),
											 // a=1, b=0, c=0
											 // X.
											 // C.
											 spk::Vector2UInt(3, 3)}}},
							  OffsetMat{
								  {OffsetRow{// a=0, b=1, c=1
											 // .X
											 // CX
											 spk::Vector2UInt(2, 2),
											 // a=0, b=1, c=0
											 // ..
											 // CX
											 spk::Vector2UInt(2, 2)},
								   OffsetRow{// a=0, b=0, c=1
											 // .X
											 // C.
											 spk::Vector2UInt(3, 2),
											 // a=0, b=0, c=0
											 // ..
											 // C.
											 spk::Vector2UInt(1, 0)}}}}}},

						{Corner::BottomRight,
						 OffsetCube{
							 {OffsetMat{
								  {OffsetRow{// a=1, b=1, c=1
											 // CX
											 // XX
											 spk::Vector2UInt(2, 4),
											 // a=1, b=1, c=0
											 // CX
											 // X.
											 spk::Vector2UInt(3, 1)},
								   OffsetRow{// a=1, b=0, c=1
											 // C.
											 // XX
											 spk::Vector2UInt(3, 4),
											 // a=1, b=0, c=0
											 // C.
											 // X.
											 spk::Vector2UInt(3, 4)}}},
							  OffsetMat{
								  {OffsetRow{// a=0, b=1, c=1
											 // CX
											 // .X
											 spk::Vector2UInt(2, 5),
											 // a=0, b=1, c=0
											 // CX
											 // ..
											 spk::Vector2UInt(2, 5)},
								   OffsetRow{// a=0, b=0, c=1
											 // C.
											 // .X
											 spk::Vector2UInt(3, 5),
											 // a=0, b=0, c=0
											 // C.
											 // ..
											 spk::Vector2UInt(1, 1)}}}}}},

						{Corner::BottomLeft,
						 OffsetCube{
							 {OffsetMat{
								  {OffsetRow{// a=1, b=1, c=1
											 // XC
											 // XX
											 spk::Vector2UInt(1, 4),
											 // a=1, b=1, c=0
											 // XC
											 // .X
											 spk::Vector2UInt(2, 1)},
								   OffsetRow{// a=1, b=0, c=1
											 // .C
											 // XX
											 spk::Vector2UInt(0, 4),
											 // a=1, b=0, c=0
											 // .C
											 // .X
											 spk::Vector2UInt(0, 4)}}},
							  OffsetMat{
								  {OffsetRow{// a=0, b=1, c=1
											 // XC
											 // X.
											 spk::Vector2UInt(1, 5),
											 // a=0, b=1, c=0
											 // XC
											 // ..
											 spk::Vector2UInt(1, 5)},
								   OffsetRow{// a=0, b=0, c=1
											 // .C
											 // X.
											 spk::Vector2UInt(0, 5),
											 // a=0, b=0, c=0
											 // .C
											 // ..
											 spk::Vector2UInt(0, 1)}}}}}}};

					int aState = static_cast<int>(p_neightbourState[0]);
					int bState = static_cast<int>(p_neightbourState[1]);
					int cState = static_cast<int>(p_neightbourState[2]);

					return (offsetsByCornerAndNeightbour.at(p_corner)[aState][bState][cState]);
				}

				void _bakeAutotile(int p_x, int p_y, int p_layer, typename TileType::ID p_currentId, const TileType &p_tile)
				{
					// With world Y up, the cell origin (p_x,p_y) is the bottom-left of the tile.
					// Place sub-quads so that Top corners have y offset +0.5, Bottom corners at y offset 0.
					static const std::unordered_map<Corner, spk::Vector2> cornerBaseOffset = {
						{Corner::TopLeft, spk::Vector2(0.0f, 0.5f)},
						{Corner::TopRight, spk::Vector2(0.5f, 0.5f)},
						{Corner::BottomRight, spk::Vector2(0.5f, 0.0f)},
						{Corner::BottomLeft, spk::Vector2(0.0f, 0.0f)}};

					const std::array<Corner, 4> corners = {Corner::TopLeft, Corner::TopRight, Corner::BottomRight, Corner::BottomLeft};

					const auto &sheet = *(_tileMap->spriteSheet());
					const spk::Vector2UInt &baseAnchor = p_tile.spriteAnchor();
					const spk::Vector2Int basePosition = _chunkCoordinates * size + spk::Vector2Int(p_x, p_y);

					for (const Corner &corner : corners)
					{
						const spk::Vector2 cornerPosition =
							spk::Vector2(static_cast<float>(p_x), static_cast<float>(p_y)) + cornerBaseOffset.at(corner);

						std::array<bool, 3> state = _getNeighbourState(basePosition, p_layer, p_currentId, corner);

						_insertData(
							cornerPosition.x,
							cornerPosition.y,
							0.5f,
							0.5f,
							p_layer,
							_tileMap->spriteSheet()->sprite(baseAnchor + _neightbourOffsetToApply(corner, state)));
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

									switch (tile->type())
									{
									case TileType::Type::Autotile:
									{
										_bakeAutotile(x, y, layer, current, *tile);
										break;
									}
									case TileType::Type::Monotile:
									{
										_bakeMonoTile(x, y, layer, *tile);
										break;
									}
									}
								}
							}
						}
						_isBaked = true;
					}
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
					_chunkCoordinates = p_coord;
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
					spk::SafePointer<Chunk> chunk = owner();
					if (chunk != nullptr)
					{
						chunk->_notifyEdition();
					}
				}

				void setContent(int p_x, int p_y, int p_layer, typename TileType::ID p_id)
				{
					_content[p_x][p_y][p_layer] = p_id;
				}

				void validate()
				{
					_isBaked = false;
					spk::SafePointer<Chunk> chunk = owner();
					if (chunk != nullptr)
					{
						chunk->_notifyEdition();
					}
				}

				// Mark this chunk's mesh as dirty to trigger a re-bake on next paint.
				void unbake()
				{
					_isBaked = false;
					spk::SafePointer<Chunk> chunk = owner();
					if (chunk != nullptr)
					{
						chunk->_notifyEdition();
					}
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

				void onUpdateEvent(spk::UpdateEvent &p_event) override
				{
					// No collision processing
					(void)p_event;
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

				spk::CollisionMesh2D bakeCollisionMesh(const spk::Flags<TFlagEnum> &p_flags) const
				{
					spk::CollisionMesh2D result;

					if (_tileMap == nullptr)
					{
						return result;
					}

					std::array<std::array<bool, ChunkSizeY>, ChunkSizeX> candidate;

					for (size_t x = 0; x < ChunkSizeX; ++x)
					{
						for (size_t y = 0; y < ChunkSizeY; ++y)
						{
							bool hasFlag = false;
							for (size_t layer = 0; layer < LayerCount; ++layer)
							{
								const typename TileType::ID &current = _content[x][y][layer];

								if (current != -1)
								{
									spk::SafePointer<const TileType> tile = _tileMap->tileById(current);

									if ((tile != nullptr) && (tile->flags().has(static_cast<TFlagEnum>(p_flags.bits)) == true))
									{
										hasFlag = true;
										break;
									}
								}
							}
							candidate[x][y] = hasFlag;
						}
					}

					while (true)
					{
						int bestArea = 0;
						int bestLeft = -1;
						int bestRight = -1;
						int bestBottom = -1;
						int bestTop = -1;
						int bestWidth = 0;
						int bestHeight = 0;

						std::array<int, ChunkSizeX> heights;
						heights.fill(0);

						for (size_t y = 0; y < ChunkSizeY; ++y)
						{
							for (size_t x = 0; x < ChunkSizeX; ++x)
							{
								if (candidate[x][y] == true)
								{
									heights[x] = heights[x] + 1;
								}
								else
								{
									heights[x] = 0;
								}
							}

							std::vector<size_t> stack;
							stack.reserve(ChunkSizeX + 1);

							for (size_t i = 0; i <= ChunkSizeX; ++i)
							{
								int currentHeight = (i < ChunkSizeX ? heights[i] : 0);
								while ((stack.empty() == false) && (heights[stack.back()] > currentHeight))
								{
									int h = heights[stack.back()];
									stack.pop_back();
									size_t leftIndex = (stack.empty() == true ? 0 : (stack.back() + 1));
									size_t rightIndex = (i == 0 ? 0 : (i - 1));
									int width = static_cast<int>(rightIndex - leftIndex + 1);
									int area = h * width;
									if (area > bestArea)
									{
										bestArea = area;
										bestLeft = static_cast<int>(leftIndex);
										bestRight = static_cast<int>(rightIndex);
										bestTop = static_cast<int>(y);
										bestBottom = static_cast<int>(y) - h + 1;
										bestWidth = width;
										bestHeight = h;
									}
									else if ((area == bestArea) == true)
									{
										bool currentIsSquare = (width == h);
										bool bestIsSquare = (bestWidth == bestHeight);
										if ((bestIsSquare == false) && (currentIsSquare == true))
										{
											bestLeft = static_cast<int>(leftIndex);
											bestRight = static_cast<int>(rightIndex);
											bestTop = static_cast<int>(y);
											bestBottom = static_cast<int>(y) - h + 1;
											bestWidth = width;
											bestHeight = h;
										}
										else if (
											((currentIsSquare == true) && (bestIsSquare == true)) ||
											((currentIsSquare == false) && (bestIsSquare == false)))
										{
											int currentDiff = std::abs(width - h);
											int bestDiff = std::abs(bestWidth - bestHeight);
											if (currentDiff < bestDiff)
											{
												bestLeft = static_cast<int>(leftIndex);
												bestRight = static_cast<int>(rightIndex);
												bestTop = static_cast<int>(y);
												bestBottom = static_cast<int>(y) - h + 1;
												bestWidth = width;
												bestHeight = h;
											}
										}
									}
								}
								stack.push_back(i);
							}
						}

						if (bestArea <= 0)
						{
							break;
						}

						for (int x = bestLeft; x <= bestRight; ++x)
						{
							for (int y = bestBottom; y <= bestTop; ++y)
							{
								candidate[static_cast<size_t>(x)][static_cast<size_t>(y)] = false;
							}
						}

						float fx0 = static_cast<float>(bestLeft);
						float fy0 = static_cast<float>(bestBottom);
						float fx1 = static_cast<float>(bestRight + 1);
						float fy1 = static_cast<float>(bestTop + 1);

						result.addUnit(
							spk::Polygon2D::fromLoop(
								{spk::Vector2(fx0, fy0), spk::Vector2(fx1, fy0), spk::Vector2(fx1, fy1), spk::Vector2(fx0, fy1)}));
					}

					return result;
				}
			};

			spk::SafePointer<TileMap<ChunkSizeX, ChunkSizeY, TLayerCount, TFlagEnum>> _tilemap;
			spk::SafePointer<Mesh2DRenderer> _renderer;
			spk::SafePointer<Data> _data;
			spk::ContractProvider _editionContractProvider;

			void _notifyEdition()
			{
				_editionContractProvider.trigger();
			}

			friend class Data;

		public:
			Chunk(const std::wstring &p_name, spk::SafePointer<TileMap> p_parent, const spk::Vector2Int &p_coord) :
				spk::Entity(p_name, p_parent)
			{
				_tilemap = p_parent;
				_renderer = this->template addComponent<Mesh2DRenderer>(p_name + L"/Mesh2DRenderer");
				_data = this->template addComponent<Data>(p_name + L"/Data");
				_data->setTileMap(_tilemap);
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

			void unbake()
			{
				_data->unbake();
			}

			spk::SafePointer<spk::Mesh2D> mesh()
			{
				return _data->mesh();
			}

			const spk::SafePointer<const spk::Mesh2D> mesh() const
			{
				return _data->mesh();
			}

			spk::CollisionMesh2D bakeCollisionMesh(const spk::Flags<TFlagEnum> &p_flags) const
			{
				return _data->bakeCollisionMesh(p_flags);
			}

			spk::SafePointer<Mesh2DRenderer> renderer()
			{
				return _renderer;
			}

			spk::SafePointer<const Mesh2DRenderer> renderer() const
			{
				return _renderer;
			}

			using EditionContract = spk::ContractProvider::Contract;
			EditionContract onEdition(const spk::ContractProvider::Job &p_job)
			{
				return _editionContractProvider.subscribe(p_job);
			}
		};

	private:
		spk::SafePointer<const spk::SpriteSheet> _spriteSheet;

		std::unordered_map<typename TileType::ID, std::unique_ptr<TileType>> _availableTiles;
		std::unordered_map<spk::Vector2Int, std::unique_ptr<Chunk>> _chunks;

		std::vector<spk::SafePointer<Chunk>> _activeChunks;

		// Mark 4-neighbour chunks dirty so they can re-bake autotiles touching the new chunk.
		void _unbakeNeightbours(const spk::Vector2Int &p_chunkCoordinate)
		{
			static const std::array<spk::Vector2Int, 4> neighbourOffsets = {
				spk::Vector2Int(1, 0), spk::Vector2Int(-1, 0), spk::Vector2Int(0, 1), spk::Vector2Int(0, -1)};

			for (const auto &offset : neighbourOffsets)
			{
				const spk::Vector2Int neighbourCoord = p_chunkCoordinate + offset;
				if (_chunks.contains(neighbourCoord) == true)
				{
					_chunks[neighbourCoord]->unbake();
				}
			}
		}

		std::unique_ptr<Chunk> _generateChunk(const spk::Vector2Int &p_chunkCoordinate)
		{
			const std::wstring chunkName = name() + L"/Chunk[" + p_chunkCoordinate.toWstring() + L"]";

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
			spk::Vector2Int chunkCoord = worldToChunkCoordinates(p_global);

			if (_chunks.contains(chunkCoord) == false)
			{
				return -1;
			}

			// Compute local coordinates with positive modulo so they are in [0, size)
			spk::Vector2Int local = {
				static_cast<int>(spk::positiveModulo(p_global.x, Chunk::size.x)), static_cast<int>(spk::positiveModulo(p_global.y, Chunk::size.y))};
			return _chunks.at(chunkCoord)->content(local, p_layer);
		}

		void setContent(const spk::Vector2Int &p_global, int p_layer, typename TileType::ID p_id)
		{
			const spk::Vector2Int chunkCoord = worldToChunkCoordinate(p_global);

			if (_chunks.contains(chunkCoord) == false)
			{
				_chunks.emplace(chunkCoord, std::move(_generateChunk(chunkCoord)));
				_unbakeNeightbours(chunkCoord);
			}

			spk::Vector2Int local = absoluteToRelativeCoordinate(p_global);

			_chunks[chunkCoord]->setContent(local, p_layer, p_id);
		}

		void setContent(int p_x, int p_y, int p_layer, typename TileType::ID p_id)
		{
			setContent(spk::Vector2Int(p_x, p_y), p_layer, p_id);
		}

		static spk::Vector2Int worldToChunkCoordinates(const spk::Vector2Int &p_global)
		{
			const int cx = static_cast<int>(std::floor(static_cast<float>(p_global.x) / static_cast<float>(Chunk::size.x)));
			const int cy = static_cast<int>(std::floor(static_cast<float>(p_global.y) / static_cast<float>(Chunk::size.y)));
			return {cx, cy};
		}

		static spk::Vector2Int chunkToWorldCoordinates(const spk::Vector2Int &p_chunkCoord)
		{
			return {p_chunkCoord.x * Chunk::size.x, p_chunkCoord.y * Chunk::size.y};
		}

		static spk::Vector2Int absoluteToRelativeCoordinates(const spk::Vector2Int &p_global)
		{
			return {
				static_cast<int>(spk::positiveModulo(p_global.x, Chunk::size.x)), static_cast<int>(spk::positiveModulo(p_global.y, Chunk::size.y))};
		}

		static spk::Vector2Int relativeToAbsoluteCoordinates(const spk::Vector2Int &p_relative, const spk::Vector2Int &p_chunkCoord)
		{
			return {p_chunkCoord.x * Chunk::size.x + p_relative.x, p_chunkCoord.y * Chunk::size.y + p_relative.y};
		}

		spk::SafePointer<Chunk> chunk(const spk::Vector2Int &p_chunkCoordinates)
		{
			if (_chunks.contains(p_chunkCoordinates) == false)
			{
				_chunks.emplace(p_chunkCoordinates, std::move(_generateChunk(p_chunkCoordinates)));

				_unbakeNeightbours(p_chunkCoordinates);
			}

			return (_chunks[p_chunkCoordinates].get());
		}

		spk::SafePointer<const Chunk> chunk(const spk::Vector2Int &p_chunkCoordinates) const
		{
			if (_chunks.contains(p_chunkCoordinates) == false)
			{
				return (nullptr);
			}

			return (_chunks.at(p_chunkCoordinates).get());
		}

		void activateChunks(const spk::Vector2Int &p_start, const spk::Vector2Int &p_end)
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
					_activeChunks.push_back(chunk({i, j}));
				}
			}

			for (auto &chunk : _activeChunks)
			{
				chunk->activate();
			}
		}
	};
}
