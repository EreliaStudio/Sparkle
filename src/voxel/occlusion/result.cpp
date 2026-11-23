
#include "voxel/occlusion/result.hpp"

#include <utility>

namespace spk::Voxel::Occlusion
{
	struct Result::Storage
	{
		Data fragments;
		std::vector<Triangle> triangles;
	};

	Result::Result() :
		_data(std::make_shared<const Storage>())
	{
	}

	Result::Result(Data fragments)
	{
		auto result = std::make_shared<Storage>();
		result->fragments = std::move(fragments);
		for (const auto &polygon : result->fragments)
		{
			const auto &vertices = polygon.vertices;
			for (std::size_t i = 1; i + 1 < vertices.size(); ++i)
			{
				const auto &a = vertices[0];
				const auto &b = vertices[i];
				const auto &c = vertices[i + 1];
				const auto cross = (b.position - a.position).cross(c.position - a.position);
				result->triangles.push_back({
					{{
						{a.position, a.normal.value_or(polygon.geometricNormal), a.uv},
						{b.position, b.normal.value_or(polygon.geometricNormal), b.uv},
						{c.position, c.normal.value_or(polygon.geometricNormal), c.uv},
					}},
					polygon.slotKey,
					cross.length()});
			}
		}
		_data = std::move(result);
	}

	const Result::Data &Result::fragments() const noexcept
	{
		return _data->fragments;
	}

	const std::vector<Result::Triangle> &Result::triangles() const noexcept
	{
		return _data->triangles;
	}

	bool Result::empty() const noexcept
	{
		return _data->fragments.empty();
	}
}
