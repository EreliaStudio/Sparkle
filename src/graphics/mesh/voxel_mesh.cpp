#include "voxel/model.hpp"

#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
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

	void Model::bind(spk::Palette palette)
	{
		if (!isCompatible(palette))
		{
			for (const auto &slot : materialSlots())
			{
				if (palette.find(slot.name) == nullptr)
				{
					throw spk::Exception("Voxel Model: palette is missing material slot '" + slot.name + "'");
				}
			}
		}
		_palette = std::move(palette);
	}

	const spk::Palette *Model::palette() const noexcept
	{
		return _palette ? &*_palette : nullptr;
	}

	bool Model::isCompatible(const spk::Palette &palette) const noexcept
	{
		for (const auto &slot : materialSlots())
		{
			if (palette.find(slot.name) == nullptr)
			{
				return false;
			}
		}
		return true;
	}
}
