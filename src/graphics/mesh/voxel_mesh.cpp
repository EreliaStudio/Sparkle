#include "voxel/model.hpp"

#include <limits>
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
		std::vector<Definition::Slot> slots,
		std::uint32_t materialIndexOffset) :
		_data(std::make_shared<const Data>(std::move(opaque), std::move(translucent), std::move(depth), std::move(slots), materialIndexOffset))
	{
		if (!_data->slots.empty() &&
			_data->slots.size() - 1 > std::numeric_limits<std::uint32_t>::max() - materialIndexOffset)
		{
			throw spk::Exception("Voxel Model: material index offset overflow");
		}
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

	std::uint32_t Model::materialIndexOffset() const noexcept
	{
		return _data->materialIndexOffset;
	}

	const Definition::Slot &Model::materialSlot(std::uint32_t index) const
	{
		if (index < materialIndexOffset() || index - materialIndexOffset() >= materialSlots().size())
		{
			throw spk::Exception("Voxel Model: material index out of range");
		}
		return materialSlots()[index - materialIndexOffset()];
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
