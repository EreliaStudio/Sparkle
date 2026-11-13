#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "geometry/mesh.hpp"
#include "geometry/shadow_mesh.hpp"
#include "graphics/palette.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "voxel/definition.hpp"

namespace spk::Voxel
{
	class Geometry final
	{
	public:
		using MaterialIndices = std::unordered_map<std::string, std::uint32_t>;

		struct Vertex
		{
			spk::Vector3 position;
			spk::Vector3 normal;
			spk::Vector2 uv;
			std::uint32_t materialIndex;
		};

		class SurfaceMesh final : public spk::Mesh<Vertex>
		{
		public:
			class Builder final : public spk::Mesh<Vertex>::Builder<SurfaceMesh>
			{
			private:
				void _setupAttributes(spk::VertexBuffer &buffer) override;
			};
		};

	private:
		struct Data
		{
			SurfaceMesh opaque;
			SurfaceMesh translucent;
			spk::ShadowMap::Mesh depth;
			std::vector<Definition::Slot> slots;
			MaterialIndices materialIndices;
		};

		std::shared_ptr<const Data> _data;

	public:
		Geometry();
		Geometry(
			SurfaceMesh opaque,
			SurfaceMesh translucent,
			spk::ShadowMap::Mesh depth,
			std::vector<Definition::Slot> slots);

		[[nodiscard]] const SurfaceMesh &opaqueMesh() const noexcept;
		[[nodiscard]] const SurfaceMesh &transparentMesh() const noexcept;
		[[nodiscard]] const spk::ShadowMap::Mesh &depthMesh() const noexcept;
		[[nodiscard]] const std::vector<Definition::Slot> &materialSlots() const noexcept;
		[[nodiscard]] const MaterialIndices &materialIndices() const noexcept;
		[[nodiscard]] bool isCompatible(const spk::Palette &palette) const noexcept;
		[[nodiscard]] const Definition::Slot &materialSlot(std::uint32_t index) const;
	};
}
