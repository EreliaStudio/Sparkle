#include "voxel/mesher.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "exception.hpp"

namespace spk::Voxel
{
	namespace
	{
		using Face = Shape::Face;
		using Polygon = Shape::Polygon;
		using Position = spk::Vector3UInt;
		using SlotIndexCollection = std::map<std::string, std::uint32_t>;
		using MaterialOpacityCollection = std::map<std::string, float>;
		struct ResolvedSlot
		{
			std::string geometricKey;
			std::uint32_t materialIndex;
			bool opaque;
		};
		using ResolvedSlotContainer = std::vector<ResolvedSlot>;
		using MeshVertexContainer = std::vector<Geometry::Vertex>;

		constexpr std::array<Face, 6> Faces{
			Face::NegativeX,
			Face::PositiveX,
			Face::NegativeY,
			Face::PositiveY,
			Face::NegativeZ,
			Face::PositiveZ};

		[[nodiscard]] Shape::Orientation orientation(const Cell &cell)
		{
			return {cell.horizontalOrientation(), cell.verticalOrientation()};
		}

		[[nodiscard]] std::optional<std::size_t> neighborIndex(Position position, Position dimensions, std::size_t index, const std::array<std::size_t, 3> &strides, Face face)
		{
			const std::size_t axis = static_cast<std::size_t>(face) / 2;
			const bool positive = (static_cast<unsigned>(face) & 1U) != 0;
			if (positive)
			{
				if (position.data[axis] == dimensions.data[axis] - 1)
				{
					return std::nullopt;
				}
				return index + strides[axis];
			}
			if (position.data[axis] == 0)
			{
				return std::nullopt;
			}
			return index - strides[axis];
		}

		[[nodiscard]] const Definition &boundaryCube()
		{
			static const Definition result = [] {
				Shape::Builder builder;
				builder.addSlot("boundary");
				builder.addPolygon("boundary", {{{0, 0, 1}}, {{0, 1, 1}}, {{0, 1, 0}}, {{0, 0, 0}}});
				builder.addPolygon("boundary", {{{1, 0, 0}}, {{1, 1, 0}}, {{1, 1, 1}}, {{1, 0, 1}}});
				builder.addPolygon("boundary", {{{0, 0, 0}}, {{1, 0, 0}}, {{1, 0, 1}}, {{0, 0, 1}}});
				builder.addPolygon("boundary", {{{0, 1, 1}}, {{1, 1, 1}}, {{1, 1, 0}}, {{0, 1, 0}}});
				builder.addPolygon("boundary", {{{0, 0, 0}}, {{0, 1, 0}}, {{1, 1, 0}}, {{1, 0, 0}}});
				builder.addPolygon("boundary", {{{1, 0, 1}}, {{1, 1, 1}}, {{0, 1, 1}}, {{0, 0, 1}}});
				return Definition(builder.build(), {{"boundary", {"boundary"}}});
			}();
			return result;
		}

		[[nodiscard]] Occlusion::Result visible(
			const Cell &source,
			const Cell &blocker,
			Face direction,
			float tolerance,
			Occlusion::Cache *cache)
		{
			Occlusion::Cache::Key key;
			key.source = source.definition();
			key.sourceOrientation = orientation(source);
			key.blocker = blocker.definition();
			key.blockerOrientation = orientation(blocker);
			key.direction = direction;
			key.tolerance = tolerance;
			if (cache != nullptr)
			{
				return cache->get(key);
			}
			return Occlusion::Algorithm::compute(
				key.source,
				key.sourceOrientation,
				key.blocker,
				key.blockerOrientation,
				key.direction,
				key.tolerance);
		}

		void emit(
			MeshVertexContainer &verticesOut,
			const Polygon &polygon,
			Position position,
			std::uint32_t materialIndex,
			float tolerance)
		{
			const auto &vertices = polygon.vertices;
			const spk::Vector3 translation(position);
			for (std::size_t i = 1; i + 1 < vertices.size(); ++i)
			{
				const auto &a = vertices[0];
				const auto &b = vertices[i];
				const auto &c = vertices[i + 1];
				const auto cross = (b.position - a.position).cross(c.position - a.position);
				if (cross.length() <= 2.0 * tolerance * tolerance)
				{
					continue;
				}
				verticesOut.push_back({a.position + translation, a.normal.value_or(polygon.geometricNormal), a.uv, materialIndex});
				verticesOut.push_back({b.position + translation, b.normal.value_or(polygon.geometricNormal), b.uv, materialIndex});
				verticesOut.push_back({c.position + translation, c.normal.value_or(polygon.geometricNormal), c.uv, materialIndex});
			}
		}

		void emit(
			const Polygon &polygon,
			Position position,
			const ResolvedSlotContainer &slots,
			MeshVertexContainer &opaque,
			MeshVertexContainer &translucent,
			float tolerance)
		{
			const auto entry = std::find_if(slots.begin(), slots.end(), [&](const ResolvedSlot &slot) {
				return slot.geometricKey == polygon.slotKey;
			});
			if (entry == slots.end())
			{
				throw spk::Exception("Voxel Definition references an unmapped geometric slot");
			}
			emit(entry->opaque ? opaque : translucent, polygon, position, entry->materialIndex, tolerance);
		}

		struct SurfaceTriangles
		{
			MeshVertexContainer opaque;
			MeshVertexContainer translucent;
		};

		// Only two correctly wound triangles covering an exact unit square qualify.
		// This fast path handles complete voxel faces; smaller rectangles are classified separately.
		[[nodiscard]] bool isFullOpaqueFace(const MeshVertexContainer &vertices, Face face)
		{
			if (vertices.size() != 6)
			{
				return false;
			}
			const auto axis = static_cast<std::size_t>(face) / 2;
			const auto u = (axis + 1) % 3;
			const auto v = (axis + 2) % 3;
			const float plane = static_cast<unsigned>(face) & 1U ? 1.0f : 0.0f;
			std::array<unsigned, 2> masks{};
			for (std::size_t i = 0; i < vertices.size(); ++i)
			{
				const auto &p = vertices[i].position;
				if (p.data[axis] != plane || (p.data[u] != 0 && p.data[u] != 1) || (p.data[v] != 0 && p.data[v] != 1))
				{
					return false;
				}
				masks[i / 3] |= 1U << (static_cast<unsigned>(p.data[u]) + 2U * static_cast<unsigned>(p.data[v]));
			}
			for (std::size_t i = 0; i < 6; i += 3)
			{
				const auto a = vertices[i + 1].position - vertices[i].position;
				const auto b = vertices[i + 2].position - vertices[i].position;
				if (a.data[u] * b.data[v] - a.data[v] * b.data[u] != (plane == 1.0f ? 1.0f : -1.0f))
				{
					return false;
				}
			}
			const unsigned shared = masks[0] & masks[1];
			return (masks[0] | masks[1]) == 15U && (shared == 6U || shared == 9U);
		}

		// Exact half-grid rectangles can be joined without resampling or
		// moving a vertex. Sloped, clipped and arbitrary polygons fall back.
		struct ShadowRectangle
		{
			Face face;
			float plane, left, right, bottom, top;
		};

		[[nodiscard]] bool halfGrid(float value)
		{
			return value == 0.0f || value == 0.5f || value == 1.0f;
		}

		[[nodiscard]] std::optional<ShadowRectangle> rectangleOnAxis(const MeshVertexContainer &vertices, std::size_t axis)
		{
			const auto u = (axis + 1) % 3;
			const auto v = (axis + 2) % 3;
			const float plane = vertices.front().position.data[axis];
			float left = 1, right = 0, bottom = 1, top = 0;
			for (const auto &vertex : vertices)
			{
				const auto &p = vertex.position;
				if (p.data[axis] != plane || !halfGrid(plane) || !halfGrid(p.data[u]) || !halfGrid(p.data[v]))
				{
					return std::nullopt;
				}
				left = std::min(left, p.data[u]);
				right = std::max(right, p.data[u]);
				bottom = std::min(bottom, p.data[v]);
				top = std::max(top, p.data[v]);
			}
			const float area = (right - left) * (top - bottom);
			if (area == 0.0f)
			{
				return std::nullopt;
			}
			std::array<unsigned, 2> masks{};
			for (std::size_t i = 0; i < vertices.size(); ++i)
			{
				const auto &p = vertices[i].position;
				if ((p.data[u] != left && p.data[u] != right) || (p.data[v] != bottom && p.data[v] != top))
				{
					return std::nullopt;
				}
				masks[i / 3] |= 1U << ((p.data[u] == right ? 1U : 0U) + (p.data[v] == top ? 2U : 0U));
			}
			const auto winding = [&](std::size_t i) {
				const auto a = vertices[i + 1].position - vertices[i].position;
				const auto b = vertices[i + 2].position - vertices[i].position;
				return a.data[u] * b.data[v] - a.data[v] * b.data[u];
			};
			const float direction = winding(0);
			const unsigned shared = masks[0] & masks[1];
			if (std::abs(direction) != area || winding(3) != direction || (masks[0] | masks[1]) != 15U || (shared != 6U && shared != 9U))
			{
				return std::nullopt;
			}
			return ShadowRectangle{static_cast<Face>(axis * 2 + (direction > 0 ? 1 : 0)), plane, left, right, bottom, top};
		}

		[[nodiscard]] std::optional<ShadowRectangle> shadowRectangle(const MeshVertexContainer &vertices)
		{
			if (vertices.size() == 6)
			{
				for (std::size_t axis = 0; axis < 3; ++axis)
				{
					if (auto rectangle = rectangleOnAxis(vertices, axis))
					{
						return rectangle;
					}
				}
			}
			return std::nullopt;
		}

		struct CellTemplate
		{
			Cell cell;
			std::array<SurfaceTriangles, 6> outer;
			SurfaceTriangles inner;
			std::optional<SurfaceTriangles> isolated;
			bool hasInner = false;
			std::array<bool, 6> hasOuter{};
			std::array<bool, 6> fullOpaqueFace{};
			std::array<std::optional<ShadowRectangle>, 6> partialOpaqueFace{};
			std::optional<ShadowRectangle> innerRectangle;
			std::array<std::optional<bool>, 6> fullyOccludes{};
		};

		using OrientedCellTemplateContainer = std::array<std::optional<CellTemplate>, 8>;
		using CellTemplateContainer = std::vector<OrientedCellTemplateContainer>;

		[[nodiscard]] CellTemplate buildCellTemplate(
			const Shape::Instance &instance,
			const ResolvedSlotContainer &slots,
			float tolerance)
		{
			CellTemplate result;
			for (const Polygon &polygon : instance.triangles())
			{
				SurfaceTriangles *surface;
				if (polygon.shell == Shape::Shell::Inner)
				{
					result.hasInner = true;
					surface = &result.inner;
				}
				else if (polygon.boundary)
				{
					const auto faceIndex = static_cast<std::size_t>(*polygon.boundary);
					result.hasOuter[faceIndex] = true;
					surface = &result.outer[faceIndex];
				}
				else
				{
					continue;
				}
				emit(polygon, {0, 0, 0}, slots, surface->opaque, surface->translucent, tolerance);
			}
			for (const Face face : Faces)
			{
				const auto i = static_cast<std::size_t>(face);
				result.fullOpaqueFace[i] = isFullOpaqueFace(result.outer[i].opaque, face);
				if (!result.fullOpaqueFace[i])
				{
					result.partialOpaqueFace[i] = shadowRectangle(result.outer[i].opaque);
				}
			}
			result.innerRectangle = shadowRectangle(result.inner.opaque);
			return result;
		}

		[[nodiscard]] const SurfaceTriangles &isolatedTriangles(CellTemplate &entry)
		{
			if (entry.isolated.has_value())
			{
				return *entry.isolated;
			}
			auto &result = entry.isolated.emplace();
			std::size_t opaqueCount = entry.inner.opaque.size();
			std::size_t translucentCount = entry.inner.translucent.size();
			for (const auto &face : entry.outer)
			{
				opaqueCount += face.opaque.size();
				translucentCount += face.translucent.size();
			}
			result.opaque.reserve(opaqueCount);
			result.translucent.reserve(translucentCount);
			for (const auto &face : entry.outer)
			{
				result.opaque.insert(result.opaque.end(), face.opaque.begin(), face.opaque.end());
				result.translucent.insert(result.translucent.end(), face.translucent.begin(), face.translucent.end());
			}
			result.opaque.insert(result.opaque.end(), entry.inner.opaque.begin(), entry.inner.opaque.end());
			result.translucent.insert(result.translucent.end(), entry.inner.translucent.begin(), entry.inner.translucent.end());
			return result;
		}

		void appendTranslated(MeshVertexContainer &output, const MeshVertexContainer &input, Position position)
		{
			if (input.empty())
			{
				return;
			}
			const auto offset = output.size();
			output.insert(output.end(), input.begin(), input.end());
			const spk::Vector3 translation(position);
			for (std::size_t i = offset; i < output.size(); ++i)
			{
				output[i].position += translation;
			}
		}

		void appendTranslated(MeshVertexContainer &opaque, MeshVertexContainer &translucent, const SurfaceTriangles &triangles, Position position)
		{
			appendTranslated(opaque, triangles.opaque, position);
			appendTranslated(translucent, triangles.translucent, position);
		}

		[[nodiscard]] std::size_t orientationIndex(Shape::Orientation value)
		{
			return (static_cast<unsigned>(value.vertical) * 4) + static_cast<unsigned>(value.horizontal) / 90;
		}

		void emit(
			const Occlusion::Result::Triangle &triangle,
			Position position,
			const ResolvedSlotContainer &slots,
			MeshVertexContainer &opaque,
			MeshVertexContainer &translucent,
			float tolerance)
		{
			if (triangle.crossLength <= 2.0 * tolerance * tolerance)
			{
				return;
			}
			const ResolvedSlot *entry = nullptr;
			if (triangle.slotIndex < slots.size())
			{
				entry = &slots[triangle.slotIndex];
			}
			else
			{
				const auto found = std::find_if(slots.begin(), slots.end(), [&](const ResolvedSlot &slot) {
					return slot.geometricKey == triangle.slotKey;
				});
				if (found == slots.end())
				{
					throw spk::Exception("Voxel Definition references an unmapped geometric slot");
				}
				entry = &*found;
			}
			auto &output = entry->opaque ? opaque : translucent;
			const spk::Vector3 translation(position);
			for (const auto &vertex : triangle.vertices)
			{
				output.push_back({vertex.position + translation, vertex.normal, vertex.uv, entry->materialIndex});
			}
		}

		[[nodiscard]] Geometry::SurfaceMesh buildSurface(const MeshVertexContainer &vertices)
		{
			Geometry::SurfaceMesh::Builder builder;
			if (!vertices.empty())
			{
				(void)builder.addVertices(vertices);
				builder.addSequentialIndices(vertices.size());
			}
			return std::move(builder).build();
		}

		class ShadowMeshSimplifier
		{
		private:
			static constexpr std::size_t Missing = std::numeric_limits<std::size_t>::max();
			Position _dimensions;
			std::size_t _cellCount;
			std::array<std::vector<std::size_t>, 6> _faces;
			std::vector<std::size_t> _removedOffsets;
			std::vector<spk::Vector3> _mergedVertices;

			struct RectangleEntry
			{
				ShadowRectangle rectangle;
				std::size_t offset;
				bool merged = false;
			};
			std::vector<RectangleEntry> _partialRectangles;

			void _remove(RectangleEntry &entry)
			{
				if (!entry.merged)
				{
					_removedOffsets.push_back(entry.offset);
					entry.merged = true;
				}
			}

			void _mergePartialAxis(bool horizontal)
			{
				auto key = [horizontal](const RectangleEntry &entry) {
					const auto &r = entry.rectangle;
					return horizontal ? std::tuple{r.face, r.plane, r.bottom, r.top, r.left, r.right} :
										std::tuple{r.face, r.plane, r.left, r.right, r.bottom, r.top};
				};
				std::sort(_partialRectangles.begin(), _partialRectangles.end(), [&](const auto &a, const auto &b) {
					return key(a) < key(b);
				});
				std::size_t count = 0;
				for (auto &entry : _partialRectangles)
				{
					if (count != 0)
					{
						auto &previous = _partialRectangles[count - 1];
						auto &a = previous.rectangle;
						const auto &b = entry.rectangle;
						const bool adjacent = horizontal ? a.bottom == b.bottom && a.top == b.top && a.right == b.left :
														 a.left == b.left && a.right == b.right && a.top == b.bottom;
						if (a.face == b.face && a.plane == b.plane && adjacent)
						{
							_remove(previous);
							_remove(entry);
							if (horizontal)
							{
								a.right = b.right;
							}
							else
							{
								a.top = b.top;
							}
							continue;
						}
					}
					_partialRectangles[count++] = entry;
				}
				_partialRectangles.resize(count);
			}

			void _mergePartialRectangles()
			{
				_mergePartialAxis(true);
				_mergePartialAxis(false);
				for (const auto &entry : _partialRectangles)
				{
					if (entry.merged)
					{
						const auto &r = entry.rectangle;
						_emitRectangle(r.face, r.plane, r.left, r.bottom, r.right - r.left, r.top - r.bottom);
					}
				}
			}

			void _emitRectangle(Face face, float plane, float x, float y, float width, float height)
			{
				const auto axis = static_cast<std::size_t>(face) / 2;
				const auto u = (axis + 1) % 3;
				const auto v = (axis + 2) % 3;
				const bool positive = (static_cast<unsigned>(face) & 1U) != 0;
				spk::Vector3 a{};
				a.data[axis] = plane;
				a.data[u] = x;
				a.data[v] = y;
				auto b = a, c = a, d = a;
				b.data[u] += static_cast<float>(width);
				c.data[u] += static_cast<float>(width);
				c.data[v] += static_cast<float>(height);
				d.data[v] += static_cast<float>(height);
				if (positive)
				{
					_mergedVertices.insert(_mergedVertices.end(), {a, b, c, a, c, d});
				}
				else
				{
					_mergedVertices.insert(_mergedVertices.end(), {a, c, b, a, d, c});
				}
			}

			void _mergePlane(Face face, std::size_t plane, std::size_t width, std::size_t height, std::span<std::size_t> mask)
			{
				for (std::size_t y = 0; y < height; ++y)
				{
					for (std::size_t x = 0; x < width; ++x)
					{
						if (mask[y * width + x] == Missing)
						{
							continue;
						}
						std::size_t w = 1, h = 1;
						while (x + w < width && mask[y * width + x + w] != Missing)
						{
							++w;
						}
						while (y + h < height)
						{
							const auto row = mask.subspan((y + h) * width + x, w);
							if (std::find(row.begin(), row.end(), Missing) != row.end())
							{
								break;
							}
							++h;
						}
						for (std::size_t dy = 0; dy < h; ++dy)
						{
							for (std::size_t dx = 0; dx < w; ++dx)
							{
								auto &offset = mask[(y + dy) * width + x + dx];
								if (w != 1 || h != 1)
								{
									_removedOffsets.push_back(offset);
								}
								offset = Missing;
							}
						}
						if (w != 1 || h != 1)
						{
							_emitRectangle(face, static_cast<float>(plane + ((static_cast<unsigned>(face) & 1U) ? 1 : 0)), static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h));
						}
					}
				}
			}

			void _mergeFaces()
			{
				for (const Face face : Faces)
				{
					auto &mask = _faces[static_cast<std::size_t>(face)];
					if (mask.empty())
					{
						continue;
					}
					const auto axis = static_cast<std::size_t>(face) / 2;
					const std::size_t width = _dimensions.data[(axis + 1) % 3];
					const std::size_t height = _dimensions.data[(axis + 2) % 3];
					for (std::size_t plane = 0; plane < _dimensions.data[axis]; ++plane)
					{
						_mergePlane(face, plane, width, height, std::span<std::size_t>(mask).subspan(plane * width * height, width * height));
					}
				}
				_mergePartialRectangles();
				std::sort(_removedOffsets.begin(), _removedOffsets.end());
			}

		public:
			ShadowMeshSimplifier(Position dimensions, std::size_t cellCount) :
				_dimensions(dimensions),
				_cellCount(cellCount)
			{
			}

			void record(Face face, Position position, std::size_t vertexOffset)
			{
				// Float positions must exactly represent both ends of a unit face.
				if (_dimensions.x > (1U << 24) || _dimensions.y > (1U << 24) || _dimensions.z > (1U << 24))
				{
					return;
				}
				auto &mask = _faces[static_cast<std::size_t>(face)];
				if (mask.empty())
				{
					mask.resize(_cellCount, Missing);
				}
				const auto axis = static_cast<std::size_t>(face) / 2;
				const auto u = (axis + 1) % 3;
				const auto v = (axis + 2) % 3;
				const auto index = (static_cast<std::size_t>(position.data[axis]) * _dimensions.data[v] + position.data[v]) * _dimensions.data[u] + position.data[u];
				mask[index] = vertexOffset;
			}

			void record(const std::optional<ShadowRectangle> &rectangle, Position position, std::size_t vertexOffset)
			{
				if (!rectangle || _dimensions.x > (1U << 23) || _dimensions.y > (1U << 23) || _dimensions.z > (1U << 23))
				{
					return;
				}
				auto r = *rectangle;
				const auto axis = static_cast<std::size_t>(r.face) / 2;
				const auto u = (axis + 1) % 3;
				const auto v = (axis + 2) % 3;
				r.plane += static_cast<float>(position.data[axis]);
				r.left += static_cast<float>(position.data[u]);
				r.right += static_cast<float>(position.data[u]);
				r.bottom += static_cast<float>(position.data[v]);
				r.top += static_cast<float>(position.data[v]);
				_partialRectangles.push_back({r, vertexOffset});
			}

			[[nodiscard]] spk::ShadowMap::Mesh build(const Geometry::SurfaceMesh &opaque, const MeshVertexContainer &vertices)
			{
				_mergeFaces();
				if (_removedOffsets.empty())
				{
					return spk::ShadowMap::Mesh::from(opaque, [](const Geometry::Vertex &vertex) {
						return vertex.position;
					});
				}
				spk::ShadowMap::Mesh::Builder builder;
				const std::size_t remaining = vertices.size() - _removedOffsets.size() * 6;
				const std::size_t count = remaining + _mergedVertices.size();
				std::size_t source = 0, removed = 0;
				(void)builder.addGeneratedVertices(count, [&](std::size_t index) {
					if (index >= remaining)
					{
						return _mergedVertices[index - remaining];
					}
					while (removed < _removedOffsets.size() && source == _removedOffsets[removed])
					{
						source += 6;
						++removed;
					}
					const auto position = vertices[source++].position;
					if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
					{
						throw spk::Exception("ShadowMap mesh position must be finite");
					}
					return position;
				});
				builder.addSequentialIndices(count);
				return std::move(builder).build();
			}
		};

		struct PreparedVolume
		{
			std::vector<Volume::PackedCell::Storage> cells;
			std::size_t occupied = 0;
			std::vector<std::size_t> definitionIndices;
			std::vector<Definition> definitions;
			std::vector<Definition::Slot> slots;
		};

		[[nodiscard]] PreparedVolume prepare(const Volume &volume)
		{
			PreparedVolume result;
			const auto dimensions = volume.dimensions();
			const auto count = static_cast<std::size_t>(dimensions.x) * dimensions.y * dimensions.z;
			result.cells.reserve(count);
			result.definitionIndices.reserve(count);
			MaterialOpacityCollection names;
			std::vector<std::pair<Definition::Registry::ID, std::size_t>> registeredDefinitions;
			std::vector<bool> hasGeometry;
			for (std::uint32_t z = 0; z < dimensions.z; ++z)
			{
				for (std::uint32_t y = 0; y < dimensions.y; ++y)
				{
					for (std::uint32_t x = 0; x < dimensions.x; ++x)
					{
						const auto packed = volume.atRaw({x, y, z});
						const auto id = Volume::PackedCell::definitionId(packed);
						const auto found = std::find_if(registeredDefinitions.begin(), registeredDefinitions.end(), [id](const auto &entry) {
							return entry.first == id;
						});
						std::size_t definitionIndex;
						if (found == registeredDefinitions.end())
						{
							const auto cell = volume.at({x, y, z});
							const auto &definition = cell.definition();
							const auto existing = std::find(result.definitions.begin(), result.definitions.end(), definition);
							definitionIndex = static_cast<std::size_t>(existing - result.definitions.begin());
							registeredDefinitions.emplace_back(id, definitionIndex);
							if (existing == result.definitions.end())
							{
								result.definitions.push_back(definition);
								hasGeometry.push_back(!definition.isAir() && !definition.shape().empty());
								for (const auto &[geometricKey, slot] : definition.slots())
								{
									(void)geometricKey;
									auto [it, inserted] = names.try_emplace(slot.name, slot.opacity);
									if (!inserted && it->second != slot.opacity)
									{
										throw spk::Exception("Voxel Geometry material slot has conflicting opacity: " + slot.name);
									}
								}
							}
						}
						else
						{
							definitionIndex = found->second;
						}
						result.occupied += hasGeometry[definitionIndex];
						result.definitionIndices.push_back(definitionIndex);
						result.cells.push_back(packed);
					}
				}
			}
			result.slots.reserve(names.size());
			for (const auto &[name, opacity] : names)
			{
				result.slots.push_back({name, opacity});
			}
			return result;
		}

		[[nodiscard]] SlotIndexCollection indexSlots(const std::vector<Definition::Slot> &slots)
		{
			SlotIndexCollection result;
			for (std::size_t i = 0; i < slots.size(); ++i)
			{
				if (i > std::numeric_limits<std::uint32_t>::max())
				{
					throw spk::Exception("Voxel Mesher: too many material slots");
				}
				result.emplace(slots[i].name, static_cast<std::uint32_t>(i));
			}
			return result;
		}
	}

	Mesher::OutsideCell Mesher::outsideCell(Position, Face) const
	{
		return {};
	}

	void Mesher::attachCache(Occlusion::Cache &cache) noexcept
	{
		_cache = &cache;
	}

	void Mesher::detachCache() noexcept
	{
		_cache = nullptr;
	}

	void Mesher::setTolerance(float tolerance)
	{
		if (!std::isfinite(tolerance) || tolerance <= 0.0f)
		{
			throw spk::Exception("Mesher tolerance must be positive and finite");
		}
		_tolerance = tolerance;
	}

	float Mesher::tolerance() const noexcept
	{
		return _tolerance;
	}

	Geometry Mesher::bake(const Volume &volume) const
	{
		auto prepared = prepare(volume);
		const auto indices = indexSlots(prepared.slots);
		std::vector<ResolvedSlotContainer> materials;
		materials.reserve(prepared.definitions.size());
		for (const auto &definition : prepared.definitions)
		{
			ResolvedSlotContainer resolved;
			resolved.reserve(definition.slots().size());
			for (const auto &[geometricKey, slot] : definition.slots())
			{
				resolved.push_back({geometricKey, indices.at(slot.name), slot.opacity == 1.0f});
			}
			materials.push_back(std::move(resolved));
		}
		MeshVertexContainer opaque;
		opaque.reserve(std::min<std::size_t>(prepared.occupied, 2500) * 40);
		MeshVertexContainer translucent;
		const auto dimensions = volume.dimensions();
		ShadowMeshSimplifier shadowSimplifier(dimensions, prepared.cells.size());
		CellTemplateContainer templates(prepared.definitions.size());
		const std::array<std::size_t, 3> strides{1, dimensions.x, static_cast<std::size_t>(dimensions.x) * dimensions.y};
		std::vector<CellTemplate *> cellTemplates(prepared.cells.size(), nullptr);
		for (std::size_t index = 0; index < prepared.cells.size(); ++index)
		{
			const auto id = prepared.definitionIndices[index];
			const auto &definition = prepared.definitions[id];
			if (definition.isAir() || definition.shape().empty())
			{
				continue;
			}
			const auto packed = prepared.cells[index];
			const Shape::Orientation orientationValue{Volume::PackedCell::horizontal(packed), Volume::PackedCell::vertical(packed)};
			auto &entry = templates[id][orientationIndex(orientationValue)];
			if (!entry)
			{
				entry.emplace(buildCellTemplate(definition.shape().instance(orientationValue), materials[id], _tolerance));
				entry->cell = Cell(definition, orientationValue.horizontal, orientationValue.vertical);
			}
			cellTemplates[index] = &*entry;
		}
		for (std::uint32_t z = 0; z < dimensions.z; ++z)
		{
			for (std::uint32_t y = 0; y < dimensions.y; ++y)
			{
				for (std::uint32_t x = 0; x < dimensions.x; ++x)
				{
					const Position position{x, y, z};
					const auto cellIndex = (static_cast<std::size_t>(z) * dimensions.y + y) * dimensions.x + x;
					if (cellTemplates[cellIndex] == nullptr)
					{
						continue;
					}
					auto &faces = *cellTemplates[cellIndex];
					// Interior cells without geometrically occupied neighbors cannot
					// have clipped faces or a closed inner shell. Emit their complete
					// surface in one batch, preserving face and material order.
					if (x > 0 && x + 1 < dimensions.x &&
						y > 0 && y + 1 < dimensions.y &&
						z > 0 && z + 1 < dimensions.z &&
						cellTemplates[cellIndex - strides[0]] == nullptr &&
						cellTemplates[cellIndex + strides[0]] == nullptr &&
						cellTemplates[cellIndex - strides[1]] == nullptr &&
						cellTemplates[cellIndex + strides[1]] == nullptr &&
						cellTemplates[cellIndex - strides[2]] == nullptr &&
						cellTemplates[cellIndex + strides[2]] == nullptr)
					{
						appendTranslated(opaque, translucent, isolatedTriangles(faces), position);
						continue;
					}
					const auto &resolved = materials[prepared.definitionIndices[cellIndex]];
					const Cell &cell = faces.cell;
					bool closed = faces.hasInner;
					for (const auto face : Faces)
					{
						const auto faceIndex = static_cast<std::size_t>(face);
						const auto &faceTriangles = faces.outer[faceIndex];
						const bool hasFace = !faceTriangles.opaque.empty() || !faceTriangles.translucent.empty();
						if (!closed && !hasFace)
						{
							continue;
						}
						const auto adjacent = neighborIndex(position, dimensions, cellIndex, strides, face);
						std::optional<OutsideCell> outside;
						const Cell *blocker;
						std::optional<std::size_t> blockerIndex;
						bool occupied;
						if (adjacent)
						{
							blockerIndex = *adjacent;
							occupied = cellTemplates[*blockerIndex] != nullptr;
							blocker = occupied ? &cellTemplates[*blockerIndex]->cell : nullptr;
						}
						else
						{
							outside.emplace(outsideCell(position, face));
							blocker = &outside->cell;
							occupied = outside->state == OutsideCell::State::Occupied && !blocker->isAir();
						}
						CellTemplate *blockerTemplates = occupied && blockerIndex ? cellTemplates[*blockerIndex] : nullptr;
						// Without an opposing boundary polygon, this neighbor cannot clip
						// the source face. Reuse the prepared triangles unchanged.
						const bool blockerCanOcclude = blockerTemplates == nullptr || blockerTemplates->hasOuter[faceIndex ^ 1U];
						bool fullyOccluded = false;
						if (occupied && blockerCanOcclude && (closed || hasFace))
						{
							if (blockerTemplates != nullptr)
							{
								auto &cached = blockerTemplates->fullyOccludes[faceIndex];
								if (!cached)
								{
									cached = visible(Cell(boundaryCube()), *blocker, face, _tolerance, _cache).empty();
								}
								fullyOccluded = *cached;
							}
							else
							{
								fullyOccluded = visible(Cell(boundaryCube()), *blocker, face, _tolerance, _cache).empty();
							}
						}
						if (closed && (!occupied || !fullyOccluded))
						{
							closed = false;
						}
						if (occupied)
						{
							if (hasFace && !fullyOccluded)
							{
								if (!blockerCanOcclude)
								{
									if (faces.fullOpaqueFace[faceIndex])
									{
										shadowSimplifier.record(face, position, opaque.size());
									}
									else
									{
										shadowSimplifier.record(faces.partialOpaqueFace[faceIndex], position, opaque.size());
									}
									appendTranslated(opaque, translucent, faceTriangles, position);
								}
								else
								{
									const auto visibility = visible(cell, *blocker, face, _tolerance, _cache);
									for (const auto &triangle : visibility.triangles())
									{
										emit(triangle, position, resolved, opaque, translucent, _tolerance);
									}
								}
							}
						}
						else
						{
							if (faces.fullOpaqueFace[faceIndex])
							{
								shadowSimplifier.record(face, position, opaque.size());
							}
							else
							{
								shadowSimplifier.record(faces.partialOpaqueFace[faceIndex], position, opaque.size());
							}
							appendTranslated(opaque, translucent, faceTriangles, position);
						}
					}
					if (!closed)
					{
						shadowSimplifier.record(faces.innerRectangle, position, opaque.size());
						appendTranslated(opaque, translucent, faces.inner, position);
					}
				}
			}
		}
		Geometry::SurfaceMesh opaqueMesh = buildSurface(opaque);
		auto depth = shadowSimplifier.build(opaqueMesh, opaque);
		return Geometry(std::move(opaqueMesh), buildSurface(translucent), std::move(depth), std::move(prepared.slots));
	}
}
