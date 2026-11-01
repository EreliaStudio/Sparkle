#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "geometry/mesh.hpp"
#include "geometry/shadow_mesh.hpp"
#include "graphics/palette.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "voxel/definition.hpp"

namespace spk::Voxel
{
	struct MeshVertex
	{
		spk::Vector3 position;
		spk::Vector3 normal;
		spk::Vector2 uv;
		std::uint32_t materialIndex;
	};

	class SurfaceMesh final : public spk::Mesh<MeshVertex>
	{
	public:
		class Builder final : public spk::Mesh<MeshVertex>::Builder<SurfaceMesh>
		{
		private:
			void _setupAttributes(spk::VertexBuffer &buffer) override;
		};
	};

	class Mesh final
	{
	private:
		struct Data
		{
			SurfaceMesh opaque;
			SurfaceMesh translucent;
			spk::ShadowMap::Mesh depth;
			std::vector<Definition::Slot> slots;
			std::uint32_t offset = 0;
			bool harmonized = false;
		};

		std::shared_ptr<const Data> _data;

	public:
		Mesh();
		Mesh(
			SurfaceMesh opaque,
			SurfaceMesh translucent,
			spk::ShadowMap::Mesh depth,
			std::vector<Definition::Slot> slots,
			std::uint32_t materialIndexOffset = 0,
			bool harmonized = false);

		[[nodiscard]] const SurfaceMesh &opaqueMesh() const noexcept;
		[[nodiscard]] const SurfaceMesh &transparentMesh() const noexcept;
		[[nodiscard]] const spk::ShadowMap::Mesh &depthMesh() const noexcept;
		[[nodiscard]] const std::vector<Definition::Slot> &materialSlots() const noexcept;
		[[nodiscard]] std::uint32_t materialIndexOffset() const noexcept;
		[[nodiscard]] bool isHarmonized() const noexcept;
		[[nodiscard]] bool accepts(const spk::Palette &palette) const noexcept;
		[[nodiscard]] const Definition::Slot &materialSlot(std::uint32_t index) const;
		[[nodiscard]] Mesh remap(
			const std::vector<std::uint32_t> &indices,
			std::vector<Definition::Slot> slots) const;
	};
}
