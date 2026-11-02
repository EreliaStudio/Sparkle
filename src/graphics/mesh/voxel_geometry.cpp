#include "voxel/geometry.hpp"

#include <iterator>
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
		std::vector<Definition::Slot> slots,
		std::uint32_t materialIndexOffset,
		bool harmonized) :
		_data(std::make_shared<const Data>(std::move(opaque), std::move(translucent), std::move(depth), std::move(slots), materialIndexOffset, harmonized))
	{
		if (!_data->slots.empty() &&
			_data->slots.size() - 1 > std::numeric_limits<std::uint32_t>::max() - materialIndexOffset)
		{
			throw spk::Exception("Voxel Geometry: material index offset overflow");
		}
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

	std::uint32_t Geometry::materialIndexOffset() const noexcept
	{
		return _data->offset;
	}

	bool Geometry::isHarmonized() const noexcept
	{
		return _data->harmonized;
	}

	bool Geometry::isCompatible(const spk::Palette &palette) const noexcept
	{
		if (isHarmonized() && palette.materials().size() != materialSlots().size())
		{
			return false;
		}
		std::size_t index = 0;
		for (const auto &slot : materialSlots())
		{
			if (!palette.find(slot.name))
			{
				return false;
			}
			if (isHarmonized() && std::next(palette.materials().begin(), static_cast<std::ptrdiff_t>(index))->first != slot.name)
			{
				return false;
			}
			++index;
		}
		return true;
	}

	const Definition::Slot &Geometry::materialSlot(std::uint32_t index) const
	{
		if (index < materialIndexOffset() || index - materialIndexOffset() >= materialSlots().size())
		{
			throw spk::Exception("Voxel Geometry: material index out of range");
		}
		return materialSlots()[index - materialIndexOffset()];
	}
}
