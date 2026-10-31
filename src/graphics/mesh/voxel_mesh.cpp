#include "voxel/model.hpp"

#include <utility>

namespace spk::Voxel
{
	void VoxelMesh::Builder::_setupAttributes(spk::VertexBuffer &buffer)
	{
		using Type = spk::VertexBuffer::Attribute::Type;
		buffer.addAttribute(0, Type::Float, 3);
		buffer.addAttribute(1, Type::Float, 3);
		buffer.addAttribute(2, Type::Float, 2);
		buffer.addAttribute(3, Type::UnsignedInt, 1, spk::VertexBuffer::Interpretation::Integer);
	}

	Model::Model(
		VoxelMesh opaque,
		VoxelMesh translucent,
		spk::ShadowMap::Mesh depth,
		std::vector<Definition::Slot> slots) :
		_data(std::make_shared<const Data>(std::move(opaque), std::move(translucent), std::move(depth), std::move(slots)))
	{
	}

	const VoxelMesh &Model::opaqueMesh() const noexcept
	{
		return _data->opaque;
	}

	const VoxelMesh &Model::transparentMesh() const noexcept
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
}
