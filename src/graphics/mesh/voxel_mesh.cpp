#include "voxel/model.hpp"

#include <optional>

#include "graphics/voxel_palette.hpp"

#include <utility>

namespace spk::Voxel
{
	namespace
	{
		[[nodiscard]] bool sameAlbedo(
			std::optional<spk::GPUResource::Identifier> &atlas,
			const spk::Material &material) noexcept
		{
			const auto *albedo = material.find("albedo");
			if (albedo == nullptr || !albedo->texture.valid())
			{
				return false;
			}
			const auto identity = albedo->texture.identifier();
			if (!atlas)
			{
				atlas = identity;
			}
			return *atlas == identity;
		}
	}

	void Mesh::Builder::_setupAttributes(spk::VertexBuffer &buffer)
	{
		using Type = spk::VertexBuffer::Attribute::Type;
		buffer.addAttribute(0, Type::Float, 3);
		buffer.addAttribute(1, Type::Float, 3);
		buffer.addAttribute(2, Type::Float, 2);
		buffer.addAttribute(3, Type::UnsignedInt, 1, spk::VertexBuffer::Interpretation::Integer);
	}

	Model::Model(
		Mesh opaque,
		Mesh translucent,
		spk::ShadowMap::Mesh depth,
		std::vector<Definition::Slot> slots) :
		_data(std::make_shared<const Data>(std::move(opaque), std::move(translucent), std::move(depth), std::move(slots)))
	{
	}

	const Mesh &Model::opaqueMesh() const noexcept
	{
		return _data->opaque;
	}

	const Mesh &Model::transparentMesh() const noexcept
	{
		return _data->translucent;
	}

	const spk::ShadowMap::Mesh &Model::depthMesh() const noexcept
	{
		return _data->depth;
	}

	const std::vector<Definition::Slot> &Model::materialSlots() const noexcept
	{
		return _data->slots;
	}

	bool Model::isCompatible(const spk::VoxelPalette &palette) const noexcept
	{
		std::optional<spk::GPUResource::Identifier> opaqueAtlas;
		std::optional<spk::GPUResource::Identifier> translucentAtlas;
		for (const auto &slot : materialSlots())
		{
			const auto *entry = palette.find(slot.name);
			if (entry == nullptr || entry->opacity != slot.opacity)
			{
				return false;
			}
			auto &atlas = slot.opacity == 1.0f ? opaqueAtlas : translucentAtlas;
			if (!sameAlbedo(atlas, entry->material))
			{
				return false;
			}
		}
		return true;
	}
}
