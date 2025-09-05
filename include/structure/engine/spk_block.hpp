#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "structure/engine/spk_collision_mesh.hpp"
#include "structure/engine/spk_obj_mesh.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"
#include "structure/math/spk_polygon.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/spk_safe_pointer.hpp"

namespace spk
{
	class Block
	{
	public:
		using ID = short;

		enum class HorizontalOrientation
		{
			XPositive,
			ZPositive,
			XNegative,
			ZNegative
		};

		enum class VerticalOrientation
		{
			YPositive,
			YNegative
		};

		struct Orientation
		{
			HorizontalOrientation horizontalOrientation;
			VerticalOrientation verticalOrientation;

			bool operator<(const Orientation &p_other) const;
		};

		static const std::vector<spk::Vector3> neightbourCoordinates;

		using Specifier = std::pair<Block::ID, Block::Orientation>;
		using Describer = std::pair<spk::SafePointer<const Block>, Block::Orientation>;
		using NeightbourDescriber = std::array<Block::Describer, 6>;
		using Footprint = spk::Polygon;

		static const spk::SpriteSheet &spriteSheet();

		static spk::SafePointer<const spk::Texture> texture();

	protected:
		using Type = std::wstring;

		virtual const spk::ObjMesh &_mesh() const = 0;

		static spk::SpriteSheet _spriteSheet;

		static void _applySprite(spk::ObjMesh::Shape &p_shape, const spk::SpriteSheet::Sprite &p_sprite);

	private:
		struct Face
		{
			Footprint footprint;
			spk::ObjMesh mesh;
			bool full = false;
		};

		class Cache
		{
		public:
			struct Entry;

			bool hasCase(const Orientation &p_orientation) const;

			void addCase(const Orientation &p_orientation, const spk::ObjMesh &p_mesh);

			const Entry &getCase(const Orientation &p_orientation) const;

			struct Entry
			{
				spk::ObjMesh innerMesh;
				std::unordered_map<spk::Vector3, Face> faces;

				void applyInnerMesh(spk::ObjMesh &p_target, const spk::Vector3 &p_offset) const;

				void applyFace(spk::ObjMesh &p_target, const spk::Vector3 &p_offset, const spk::Vector3 &p_normal) const;

			private:
				static void _appendMesh(spk::ObjMesh &p_target, const spk::ObjMesh &p_source, const spk::Vector3 &p_offset);
			};

		private:
			static spk::Vector3 _applyOrientation(const spk::Vector3 &p_position, const Orientation &p_orientation);

			static bool _isFullQuad(const std::vector<spk::ObjVertex> &p_vertices, const spk::Vector3 &p_normal);

			static std::vector<spk::ObjVertex> _extractVertices(const spk::ObjMesh::Shape &p_shape);

			static void _applyOrientationToVertices(std::vector<spk::ObjVertex> &p_vertices, const Orientation &p_orientation);

			static bool _isAxisAlignedFace(const std::vector<spk::ObjVertex> &p_vertices, spk::Vector3 &p_outNormal);

			static void _addVerticesToMesh(spk::ObjMesh &p_mesh, const std::vector<spk::ObjVertex> &p_vertices);

			static Entry _compute(const spk::ObjMesh &p_mesh, const Orientation &p_orientation);

			std::map<Orientation, Entry> _cache;
		};

		mutable Cache _cache;

		const Cache::Entry &_ensureCacheCase(const Orientation &p_orientation) const;

		using NeighFaces = std::array<std::vector<const Face *>, 6>;

		NeighFaces _gatherNeighbourFaces(const NeightbourDescriber &p_neightbourDescriber) const;

		static bool _allSixFull(const NeighFaces &p_neighFaces);

		static void _emitInnerIfNeeded(
			spk::ObjMesh &p_toFill, const Cache::Entry &p_data, const spk::Vector3 &p_position, const NeighFaces &p_neighFaces);

		static spk::Polygon _translated(const spk::Polygon &p_poly, const spk::Vector3 &p_delta);

		static void _emitVisibleFaces(
			spk::ObjMesh &p_toFill, const Cache::Entry &p_data, const spk::Vector3 &p_position, const NeighFaces &p_neighFaces);

	public:
		virtual ~Block() = default;

		void bake(
			spk::ObjMesh &p_toFill,
			const NeightbourDescriber &p_neightbourDescriber,
			const spk::Vector3 &p_position,
			const Orientation &p_orientation) const;
	};

} // namespace spk
