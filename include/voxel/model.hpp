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

	class Mesh final : public spk::Mesh<MeshVertex>
	{
	public:
		class Builder final : public spk::Mesh<MeshVertex>::Builder<Mesh>
		{
		private:
			void _setupAttributes(spk::VertexBuffer &buffer) override;
		};
	};

	class Model final
	{
	private:
		struct Data
		{
			Mesh opaque;
			Mesh translucent;
			spk::ShadowMap::Mesh depth;
			std::vector<Definition::Slot> slots;
			std::uint32_t materialIndexOffset = 0;
		};

		std::shared_ptr<const Data> _data;

	public:
		Model(
			Mesh opaque,
			Mesh translucent,
			spk::ShadowMap::Mesh depth,
			std::vector<Definition::Slot> slots,
			std::uint32_t materialIndexOffset = 0);

		[[nodiscard]] const Mesh &opaqueMesh() const noexcept;
		[[nodiscard]] const Mesh &transparentMesh() const noexcept;
		[[nodiscard]] const spk::ShadowMap::Mesh &depthMesh() const noexcept;
		[[nodiscard]] const std::vector<Definition::Slot> &materialSlots() const noexcept;
		[[nodiscard]] std::uint32_t materialIndexOffset() const noexcept;
		[[nodiscard]] const Definition::Slot &materialSlot(std::uint32_t index) const;
		[[nodiscard]] bool isCompatible(const spk::Palette &palette) const noexcept;
	};
}
