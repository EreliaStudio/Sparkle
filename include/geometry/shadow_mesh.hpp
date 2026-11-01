#pragma once

#include <cmath>
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
		class Builder final : public spk::Mesh<spk::Vector3>::Builder<Mesh>
		{
		private:
			void _setupAttributes(spk::VertexBuffer &buffer) override;
		};

		template <typename TVertex, typename TExtractor>
			requires std::invocable<TExtractor, const TVertex &> &&
					 std::convertible_to<std::invoke_result_t<TExtractor, const TVertex &>, spk::Vector3>
		[[nodiscard]] static Mesh from(const spk::Mesh<TVertex> &source, TExtractor &&extractor)
		{
			Builder builder;
			if (source.indexCount() == 0)
			{
				return std::move(builder).build();
			}

			const auto vertices = source.layout().vertexBuffer().template cast<TVertex>();
			const auto indices = source.layout().indexBuffer().template cast<std::uint32_t>();
			if (indices.size() % 3 != 0)
			{
				throw spk::Exception("ShadowMap mesh requires triangle indices");
			}
			for (const auto &vertex : vertices)
			{
				const spk::Vector3 position = std::invoke(extractor, vertex);
				if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
				{
					throw spk::Exception("ShadowMap mesh position must be finite");
				}
				(void)builder.addVertex(position);
			}
			for (const auto index : indices)
			{
				if (index >= vertices.size())
				{
					throw spk::Exception("ShadowMap mesh index is out of bounds");
				}
				builder.addIndex(index);
			}
			return std::move(builder).build();
		}
	};
}
