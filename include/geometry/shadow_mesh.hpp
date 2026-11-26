#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <concepts>
#include <cstdint>
#include <functional>
#include <utility>

#include "exception.hpp"
#include "geometry/mesh.hpp"
#include "math/vector3.hpp"

namespace spk::ShadowMap
{
	class Mesh final : public spk::Mesh<spk::Vector3>
	{
	public:
		struct Bounds
		{
			Vector3 minimum{};
			Vector3 maximum{};
		};

	private:
		struct BoundsCache
		{
			std::once_flag once;
			Bounds value;
		};
		std::shared_ptr<BoundsCache> _bounds;

	public:
		// Shared, lazy bounds: no additional vertex scan on the meshing path,
		// and all copies of an immutable shadow mesh reuse the same result.
		[[nodiscard]] const Bounds &bounds() const
		{
			static const Bounds empty{};
			if (!_bounds)
			{
				return empty;
			}
			std::call_once(_bounds->once, [this] {
				const auto vertices = layout().vertexBuffer().cast<Vector3>();
				if (vertices.empty())
				{
					return;
				}
				auto &value = _bounds->value;
				value.minimum = value.maximum = vertices.front();
				for (const auto &v : vertices)
				{
					value.minimum = {std::min(value.minimum.x, v.x), std::min(value.minimum.y, v.y), std::min(value.minimum.z, v.z)};
					value.maximum = {std::max(value.maximum.x, v.x), std::max(value.maximum.y, v.y), std::max(value.maximum.z, v.z)};
				}
			});
			return _bounds->value;
		}

		class Builder final : public spk::Mesh<spk::Vector3>::Builder<Mesh>
		{
		private:
			void _setupAttributes(spk::VertexBuffer &buffer) override;
			void _setupMesh(Mesh &mesh) override
			{
				mesh._bounds = std::make_shared<BoundsCache>();
			}
		};

		template <typename TVertex, typename TExtractor>
			requires std::invocable<TExtractor, const TVertex &> &&
					 std::convertible_to<std::invoke_result_t<TExtractor, const TVertex &>, spk::Vector3>
		[[nodiscard]] static Mesh from(const spk::Mesh<TVertex> &source, TExtractor &&extractor)
		{
			Builder builder;
			if (source.vertexCount() == 0 && source.indexCount() == 0)
			{
				return std::move(builder).build();
			}

			const auto vertices = source.layout().vertexBuffer().template cast<TVertex>();
			const auto indices = source.layout().indexBuffer().template cast<std::uint32_t>();
			if (indices.size() % 3 != 0)
			{
				throw spk::Exception("ShadowMap mesh requires triangle indices");
			}
			(void)builder.addGeneratedVertices(vertices.size(), [&](std::size_t index) {
				const spk::Vector3 position = std::invoke(extractor, vertices[index]);
				if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
				{
					throw spk::Exception("ShadowMap mesh position must be finite");
				}
				return position;
			});
			for (const auto index : indices)
			{
				if (index >= vertices.size())
				{
					throw spk::Exception("ShadowMap mesh index is out of bounds");
				}
			}
			builder.addIndices(indices);
			return std::move(builder).build();
		}
	};
}
