
#include "voxel/occlusion/result.hpp"

#include <limits>
#include <string_view>
#include <unordered_map>

#include "voxel/definition.hpp"

#include <utility>

namespace spk::Voxel::Occlusion
{
	struct Result::Storage
	{
		Data fragments;
		TriangleContainer triangles;
	};

	Result::Result() :
		_data(std::make_shared<const Storage>())
	{
	}

	Result::Result(Data fragments) :
		Result(std::move(fragments), static_cast<const Definition *>(nullptr))
	{
	}

	Result::Result(Data fragments, const Definition &source) :
		Result(std::move(fragments), &source)
	{
	}

	Result::Result(Data fragments, const Definition *source)
	{
		auto result = std::make_shared<Storage>();
		result->fragments = std::move(fragments);

		// The enumeration order matches the bake-local ResolvedSlotContainer.
		std::unordered_map<std::string_view, std::size_t> slotIndexCollection;
		if (source != nullptr)
		{
			slotIndexCollection.reserve(source->slots().size());
			std::size_t index = 0;
			for (const auto &[geometricKey, slot] : source->slots())
			{
				(void)slot;
				slotIndexCollection.emplace(geometricKey, index++);
			}
		}

		std::size_t triangleCount = 0;
		for (const auto &polygon : result->fragments)
		{
			if (polygon.vertices.size() > 2)
			{
				triangleCount += polygon.vertices.size() - 2;
			}
		}
		result->triangles.reserve(triangleCount);

		for (const auto &polygon : result->fragments)
		{
			const std::size_t slotIndex = source != nullptr
											  ? slotIndexCollection.at(polygon.slotKey)
											  : std::numeric_limits<std::size_t>::max();
			const auto &vertices = polygon.vertices;
			for (std::size_t i = 1; i + 1 < vertices.size(); ++i)
			{
				const auto &a = vertices[0];
				const auto &b = vertices[i];
				const auto &c = vertices[i + 1];
				const auto cross = (b.position - a.position).cross(c.position - a.position);
				Triangle triangle;
				triangle.vertices[0] = {a.position, a.normal.value_or(polygon.geometricNormal), a.uv};
				triangle.vertices[1] = {b.position, b.normal.value_or(polygon.geometricNormal), b.uv};
				triangle.vertices[2] = {c.position, c.normal.value_or(polygon.geometricNormal), c.uv};
				triangle.slotKey = polygon.slotKey;
				triangle.slotIndex = slotIndex;
				triangle.crossLength = cross.length();
				result->triangles.push_back(std::move(triangle));
			}
		}
		_data = std::move(result);
	}

	const Result::Data &Result::fragments() const noexcept
	{
		return _data->fragments;
	}

	const Result::TriangleContainer &Result::triangles() const noexcept
	{
		return _data->triangles;
	}

	bool Result::empty() const noexcept
	{
		return _data->fragments.empty();
	}
}
