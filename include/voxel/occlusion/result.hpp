#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "voxel/shape.hpp"

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
			double crossLength;
		};

	private:
		struct Storage;
		std::shared_ptr<const Storage> _data;

	public:
		Result();
		explicit Result(Data fragments);

		[[nodiscard]] const Data &fragments() const noexcept;
		[[nodiscard]] const std::vector<Triangle> &triangles() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
	};
}
