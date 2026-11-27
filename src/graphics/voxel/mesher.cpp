#include "voxel/mesher.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
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
		// Partial, overlapping, sloped and non-grid faces keep their original triangles.
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

		struct CellTemplate
		{
			Cell cell;
			std::array<SurfaceTriangles, 6> outer;
			SurfaceTriangles inner;
			std::optional<SurfaceTriangles> isolated;
			bool hasInner = false;
			std::array<bool, 6> hasOuter{};
			std::array<bool, 6> fullOpaqueFace{};
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
			}
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

		class FaceMask
		{
		private:
			static constexpr std::size_t WordBits = 64;
			std::vector<std::uint64_t> _words;

			[[nodiscard]] static std::uint64_t _lowBits(std::size_t count)
			{
				return count == WordBits ? ~std::uint64_t{0} : (std::uint64_t{1} << count) - 1;
			}

		public:
			[[nodiscard]] bool empty() const
			{
				return _words.empty();
			}

			void resize(std::size_t count)
			{
				_words.resize(count / WordBits + (count % WordBits != 0));
			}

			void set(std::size_t index)
			{
				_words[index / WordBits] |= std::uint64_t{1} << (index % WordBits);
			}

			[[nodiscard]] std::uint64_t read(std::size_t index, std::size_t count) const
			{
				const auto word = index / WordBits;
				const auto shift = index % WordBits;
				auto bits = _words[word] >> shift;
				if (shift + count > WordBits)
				{
					bits |= _words[word + 1] << (WordBits - shift);
				}
				return bits & _lowBits(count);
			}

			[[nodiscard]] bool test(std::size_t index) const
			{
				return (_words[index / WordBits] & (std::uint64_t{1} << (index % WordBits))) != 0;
			}

			void clearBits(std::size_t index, std::uint64_t bits)
			{
				const auto word = index / WordBits;
				const auto shift = index % WordBits;
				_words[word] &= ~(bits << shift);
				if (shift != 0 && (bits >> (WordBits - shift)) != 0)
				{
					_words[word + 1] &= ~(bits >> (WordBits - shift));
				}
			}

			void clear(std::size_t index, std::size_t count)
			{
				while (count != 0)
				{
					const auto shift = index % WordBits;
					const auto length = std::min(count, WordBits - shift);
					_words[index / WordBits] &= ~(_lowBits(length) << shift);
					index += length;
					count -= length;
				}
			}

			[[nodiscard]] std::size_t first(std::size_t index, std::size_t count) const
			{
				std::size_t offset = 0;
				while (offset < count)
				{
					const auto length = std::min(WordBits, count - offset);
					const auto bits = read(index + offset, length);
					if (bits != 0)
					{
						return offset + static_cast<std::size_t>(std::countr_zero(bits));
					}
					offset += length;
				}
				return count;
			}

			[[nodiscard]] std::size_t run(std::size_t index, std::size_t count) const
			{
				std::size_t offset = 0;
				while (offset < count)
				{
					const auto length = std::min(WordBits, count - offset);
					const auto occupied = static_cast<std::size_t>(std::countr_one(read(index + offset, length)));
					offset += std::min(occupied, length);
					if (occupied < length)
					{
						break;
					}
				}
				return offset;
			}
		};

		class ShadowMeshSimplifier
		{
		private:
			struct Candidate
			{
				std::size_t key;
				std::size_t vertexOffset;
			};

			Position _dimensions;
			std::size_t _cellCount;
			bool _enabled;
			std::array<FaceMask, 6> _faces;
			std::vector<Candidate> _candidates;
			struct Rectangle
			{
				Face face;
				std::size_t plane;
				std::size_t x;
				std::size_t y;
				std::size_t width;
				std::size_t height;
			};

			std::vector<Rectangle> _rectangles;

			[[nodiscard]] static std::array<spk::Vector3, 6> _vertices(const Rectangle &rectangle)
			{
				const auto axis = static_cast<std::size_t>(rectangle.face) / 2;
				const auto u = (axis + 1) % 3;
				const auto v = (axis + 2) % 3;
				const bool positive = (static_cast<unsigned>(rectangle.face) & 1U) != 0;
				spk::Vector3 a{};
				a.data[axis] = static_cast<float>(rectangle.plane + (positive ? 1 : 0));
				a.data[u] = static_cast<float>(rectangle.x);
				a.data[v] = static_cast<float>(rectangle.y);
				auto b = a, c = a, d = a;
				b.data[u] += static_cast<float>(rectangle.width);
				c.data[u] += static_cast<float>(rectangle.width);
				c.data[v] += static_cast<float>(rectangle.height);
				d.data[v] += static_cast<float>(rectangle.height);
				if (positive)
				{
					return {a, b, c, a, c, d};
				}
				return {a, c, b, a, d, c};
			}

			// A complete row fits in one word for ordinary chunk sizes.
			// Remove isolated faces in bulk and keep the original greedy order.
			void _mergeNarrowPlane(Face face, std::size_t plane, std::size_t width, std::size_t height, FaceMask &pending, FaceMask &merged)
			{
				const auto base = plane * width * height;
				for (std::size_t y = 0; y < height; ++y)
				{
					const auto row = base + y * width;
					auto bits = pending.read(row, width);
					if (bits == 0)
					{
						continue;
					}
					const auto below = y + 1 < height ? pending.read(row + width, width) : std::uint64_t{0};
					const auto isolated = bits & ~(bits << 1) & ~(bits >> 1) & ~below;
					merged.clearBits(row, isolated);
					bits &= ~isolated;
					while (bits != 0)
					{
						const auto x = static_cast<std::size_t>(std::countr_zero(bits));
						const auto w = static_cast<std::size_t>(std::countr_one(bits >> x));
						const auto mask = w == 64 ? ~std::uint64_t{0} : ((std::uint64_t{1} << w) - 1) << x;
						std::size_t h = 1;
						while (y + h < height && (pending.read(row + h * width, width) & mask) == mask)
						{
							pending.clearBits(row + h * width, mask);
							++h;
						}
						if (w == 1 && h == 1)
						{
							merged.clearBits(row, mask);
						}
						else
						{
							_rectangles.push_back({face, plane, x, y, w, h});
						}
						bits &= ~mask;
					}
				}
			}

			void _mergePlane(Face face, std::size_t plane, std::size_t width, std::size_t height, FaceMask &pending, FaceMask &merged)
			{
				const auto base = plane * width * height;
				for (std::size_t y = 0; y < height; ++y)
				{
					const auto row = base + y * width;
					for (std::size_t x = 0; x < width;)
					{
						x += pending.first(row + x, width - x);
						if (x == width)
						{
							break;
						}
						const auto w = pending.run(row + x, width - x);
						std::size_t h = 1;
						while (y + h < height && pending.run(row + h * width + x, w) == w)
						{
							++h;
						}
						_consumeRectangle({face, plane, x, y, w, h}, row + x, width, pending, merged);
						x += w;
					}
				}
			}

			void _consumeRectangle(const Rectangle &rectangle, std::size_t start, std::size_t stride, FaceMask &pending, FaceMask &merged)
			{
				for (std::size_t dy = 0; dy < rectangle.height; ++dy)
				{
					pending.clear(start + dy * stride, rectangle.width);
				}
				if (rectangle.width == 1 && rectangle.height == 1)
				{
					merged.clear(start, 1);
				}
				else
				{
					_rectangles.push_back(rectangle);
				}
			}

			void _mergeFaces()
			{
				// Only one direction needs a temporary working mask at a time.
				// The original mask becomes the set of faces replaced by rectangles.
				for (const Face face : Faces)
				{
					auto &merged = _faces[static_cast<std::size_t>(face)];
					if (merged.empty())
					{
						continue;
					}
					auto pending = merged;
					const auto axis = static_cast<std::size_t>(face) / 2;
					const std::size_t width = _dimensions.data[(axis + 1) % 3];
					const std::size_t height = _dimensions.data[(axis + 2) % 3];
					for (std::size_t plane = 0; plane < _dimensions.data[axis]; ++plane)
					{
						if (width <= 64)
						{
							_mergeNarrowPlane(face, plane, width, height, pending, merged);
						}
						else
						{
							_mergePlane(face, plane, width, height, pending, merged);
						}
					}
				}
			}

			[[nodiscard]] bool _isMerged(const Candidate &candidate) const
			{
				return _faces[candidate.key & 7U].test(candidate.key >> 3U);
			}

		public:
			ShadowMeshSimplifier(Position dimensions, std::size_t cellCount) :
				_dimensions(dimensions),
				_cellCount(cellCount),
				_enabled(dimensions.x <= (1U << 24) && dimensions.y <= (1U << 24) && dimensions.z <= (1U << 24) &&
						 cellCount <= std::numeric_limits<std::size_t>::max() / 8U)
			{
			}

			void record(Face face, Position position, std::size_t vertexOffset)
			{
				// Float positions must exactly represent both ends of a unit face.
				if (!_enabled)
				{
					return;
				}
				const auto direction = static_cast<std::size_t>(face);
				auto &mask = _faces[direction];
				if (mask.empty())
				{
					mask.resize(_cellCount);
				}
				const auto axis = direction / 2;
				const auto u = (axis + 1) % 3;
				const auto v = (axis + 2) % 3;
				const auto index = (static_cast<std::size_t>(position.data[axis]) * _dimensions.data[v] + position.data[v]) * _dimensions.data[u] + position.data[u];
				mask.set(index);
				// The occlusion pass visits opaque vertices in output order. Keep
				// this order rather than sorting the offsets after greedy merging.
				if (_candidates.empty())
				{
					_candidates.reserve(std::min<std::size_t>(_cellCount, 4096));
				}
				_candidates.push_back({(index << 3U) | direction, vertexOffset});
			}

			[[nodiscard]] static spk::Vector3 checkedPosition(const Geometry::Vertex &vertex)
			{
				const auto position = vertex.position;
				if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
				{
					throw spk::Exception("ShadowMap mesh position must be finite");
				}
				return position;
			}

			[[nodiscard]] spk::ShadowMap::Mesh build(const MeshVertexContainer &vertices)
			{
				_mergeFaces();
				if (_rectangles.empty())
				{
					// The opaque output is already a sequential triangle list.
					// Generate its depth positions directly, without copying or validating an index buffer.
					spk::ShadowMap::Mesh::Builder builder;
					if (!vertices.empty())
					{
						(void)builder.addGeneratedVertices(vertices.size(), [&](std::size_t index) {
							return checkedPosition(vertices[index]);
						});
						builder.addSequentialIndices(vertices.size());
					}
					return std::move(builder).build();
				}
				std::erase_if(_candidates, [this](const Candidate &candidate) {
					return !_isMerged(candidate);
				});
				spk::ShadowMap::Mesh::Builder builder;
				const std::size_t remaining = vertices.size() - _candidates.size() * 6;
				const std::size_t count = remaining + _rectangles.size() * 6;
				std::size_t source = 0, removed = 0;
				std::array<spk::Vector3, 6> rectangleVertices{};
				// Write directly into the final depth mesh: keep exact opaque
				// positions in their original order and append greedy rectangles.
				(void)builder.addGeneratedVertices(count, [&](std::size_t index) {
					if (index >= remaining)
					{
						const auto rectangleIndex = (index - remaining) / 6;
						const auto vertexIndex = (index - remaining) % 6;
						if (vertexIndex == 0)
						{
							rectangleVertices = _vertices(_rectangles[rectangleIndex]);
						}
						return rectangleVertices[vertexIndex];
					}
					while (removed < _candidates.size() && source == _candidates[removed].vertexOffset)
					{
						source += 6;
						++removed;
					}
					return checkedPosition(vertices[source++]);
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
							appendTranslated(opaque, translucent, faceTriangles, position);
						}
					}
					if (!closed)
					{
						appendTranslated(opaque, translucent, faces.inner, position);
					}
				}
			}
		}
		auto depth = shadowSimplifier.build(opaque);
		Geometry::SurfaceMesh opaqueMesh = buildSurface(opaque);
		return Geometry(std::move(opaqueMesh), buildSurface(translucent), std::move(depth), std::move(prepared.slots));
	}
}
