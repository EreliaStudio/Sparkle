#include "voxel/geometry.hpp"

#include <limits>
#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	void Geometry::SurfaceMesh::Builder::_setupAttributes(spk::VertexBuffer &buffer)
	{
		using Type = spk::VertexBuffer::Attribute::Type;
		buffer.addAttribute(0, Type::Float, 3);
		buffer.addAttribute(1, Type::Float, 3);
		buffer.addAttribute(2, Type::Float, 2);
		buffer.addAttribute(3, Type::UnsignedInt, 1, spk::VertexBuffer::Interpretation::Integer);
	}

	Geometry::Geometry() :
		_data(std::make_shared<const Data>())
	{
	}

	Geometry::Geometry(
		Geometry::SurfaceMesh opaque,
		Geometry::SurfaceMesh translucent,
		spk::ShadowMap::Mesh depth,
		std::vector<Definition::Slot> slots)
	{
		MaterialIndices indices;
		for (std::size_t index = 0; index < slots.size(); ++index)
		{
			if (index > std::numeric_limits<std::uint32_t>::max() ||
				!indices.emplace(slots[index].name, static_cast<std::uint32_t>(index)).second)
			{
				throw spk::Exception("Voxel Geometry: duplicate or invalid material index");
			}
		}
		_data = std::make_shared<const Data>(
			Data{std::move(opaque), std::move(translucent), std::move(depth), std::move(slots), std::move(indices)});
	}

	const Geometry::SurfaceMesh &Geometry::opaqueMesh() const noexcept
	{
		return _data->opaque;
	}

	const Geometry::SurfaceMesh &Geometry::transparentMesh() const noexcept
	{
		return _data->translucent;
	}

	const spk::ShadowMap::Mesh &Geometry::depthMesh() const noexcept
	{
		return _data->depth;
	}

	const std::vector<Definition::Slot> &Geometry::materialSlots() const noexcept
	{
		return _data->slots;
	}

	const Geometry::MaterialIndices &Geometry::materialIndices() const noexcept
	{
		return _data->materialIndices;
	}

	bool Geometry::isCompatible(const spk::Palette &palette) const noexcept
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

	const Definition::Slot &Geometry::materialSlot(std::uint32_t index) const
	{
		if (index >= materialSlots().size())
		{
			throw spk::Exception("Voxel Geometry: material index out of range");
		}
		return materialSlots()[index];
	}
}
