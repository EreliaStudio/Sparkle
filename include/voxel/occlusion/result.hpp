#pragma once

#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "voxel/shape.hpp"

namespace spk::Voxel
{
	class Definition;
}

namespace spk::Voxel::Occlusion
{
	class Result final
	{
	public:
		using Data = std::vector<Shape::Polygon>;

		struct Vertex
		{
			spk::Vector3 position;
			spk::Vector3 normal;
			spk::Vector2 uv;
		};

		struct Triangle
		{
			std::array<Vertex, 3> vertices;
			std::string slotKey;
			std::size_t slotIndex = std::numeric_limits<std::size_t>::max();
			double crossLength;
		};

		using TriangleContainer = std::vector<Triangle>;

	private:
		struct Storage;
		std::shared_ptr<const Storage> _data;

		Result(Data fragments, const Definition *source);

	public:
		Result();
		explicit Result(Data fragments);
		Result(Data fragments, const Definition &source);

		[[nodiscard]] const Data &fragments() const noexcept;
		[[nodiscard]] const TriangleContainer &triangles() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
	};
}
