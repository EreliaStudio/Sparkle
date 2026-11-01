#include "voxel/mesh.hpp"

#include <iterator>
#include <limits>
#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	void SurfaceMesh::Builder::_setupAttributes(spk::VertexBuffer &buffer)
	{
		using Type = spk::VertexBuffer::Attribute::Type;
		buffer.addAttribute(0, Type::Float, 3);
		buffer.addAttribute(1, Type::Float, 3);
		buffer.addAttribute(2, Type::Float, 2);
		buffer.addAttribute(3, Type::UnsignedInt, 1, spk::VertexBuffer::Interpretation::Integer);
	}

	Mesh::Mesh() :
		_data(std::make_shared<const Data>())
	{
	}

	Mesh::Mesh(
		SurfaceMesh opaque,
		SurfaceMesh translucent,
		spk::ShadowMap::Mesh depth,
		std::vector<Definition::Slot> slots,
		std::uint32_t materialIndexOffset,
		bool harmonized) :
		_data(std::make_shared<const Data>(std::move(opaque), std::move(translucent), std::move(depth), std::move(slots), materialIndexOffset, harmonized))
	{
		if (!_data->slots.empty() &&
			_data->slots.size() - 1 > std::numeric_limits<std::uint32_t>::max() - materialIndexOffset)
		{
			throw spk::Exception("Voxel Mesh: material index offset overflow");
		}
	}

	const SurfaceMesh &Mesh::opaqueMesh() const noexcept
	{
		return _data->opaque;
	}

	const SurfaceMesh &Mesh::transparentMesh() const noexcept
	{
		return _data->translucent;
	}

	const spk::ShadowMap::Mesh &Mesh::depthMesh() const noexcept
	{
		return _data->depth;
	}

	const std::vector<Definition::Slot> &Mesh::materialSlots() const noexcept
	{
		return _data->slots;
	}

	std::uint32_t Mesh::materialIndexOffset() const noexcept
	{
		return _data->offset;
	}

	bool Mesh::isHarmonized() const noexcept
	{
		return _data->harmonized;
	}

	bool Mesh::accepts(const spk::Palette &palette) const noexcept
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

	const Definition::Slot &Mesh::materialSlot(std::uint32_t index) const
	{
		if (index < materialIndexOffset() || index - materialIndexOffset() >= materialSlots().size())
		{
			throw spk::Exception("Voxel Mesh: material index out of range");
		}
		return materialSlots()[index - materialIndexOffset()];
	}

	namespace
	{
		[[nodiscard]] SurfaceMesh remapSurface(
			const SurfaceMesh &source,
			std::uint32_t offset,
			const std::vector<std::uint32_t> &mapping)
		{
			if (source.vertexCount() == 0)
			{
				return source;
			}
			SurfaceMesh::Builder builder;
			const auto vertices = source.layout().vertexBuffer().cast<MeshVertex>();
			const auto indices = source.layout().indexBuffer().cast<std::uint32_t>();
			builder.reserve(vertices.size(), indices.size());
			for (auto vertex : vertices)
			{
				if (vertex.materialIndex < offset || vertex.materialIndex - offset >= mapping.size())
				{
					throw spk::Exception("Voxel Mesh: invalid material index");
				}
				vertex.materialIndex = mapping[vertex.materialIndex - offset];
				(void)builder.addVertex(vertex);
			}
			for (const auto index : indices)
			{
				builder.addIndex(index);
			}
			return std::move(builder).build();
		}
	}

	Mesh Mesh::remap(const std::vector<std::uint32_t> &indices, std::vector<Definition::Slot> slots) const
	{
		if (indices.size() != materialSlots().size())
		{
			throw spk::Exception("Voxel Mesh: incorrect material mapping size");
		}
		for (const auto index : indices)
		{
			if (index >= slots.size())
			{
				throw spk::Exception("Voxel Mesh: material mapping exceeds slots");
			}
		}
		return Mesh(
			remapSurface(opaqueMesh(), materialIndexOffset(), indices),
			remapSurface(transparentMesh(), materialIndexOffset(), indices),
			depthMesh(),
			std::move(slots),
			0,
			true);
	}
}
