#include "voxel/model.hpp"

#include <cmath>
#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	namespace
	{
		void validateAnchor(const spk::Matrix4x4 &transform)
		{
			constexpr float tolerance = 1.0e-4f;
			for (std::size_t c = 0; c < 4; ++c)
			{
				for (std::size_t r = 0; r < 4; ++r)
				{
					if (!std::isfinite(transform[c][r]))
					{
						throw spk::Exception("Voxel Anchor: non-finite transform");
					}
				}
			}
			for (std::size_t c = 0; c < 3; ++c)
			{
				for (std::size_t d = 0; d < 3; ++d)
				{
					float dot = 0;
					for (std::size_t r = 0; r < 3; ++r)
					{
						dot += transform[c][r] * transform[d][r];
					}
					if (std::abs(dot - (c == d ? 1.0f : 0.0f)) > tolerance)
					{
						throw spk::Exception("Voxel Anchor: transform must be rigid");
					}
				}
				if (std::abs(transform[c][3]) > tolerance)
				{
					throw spk::Exception("Voxel Anchor: invalid homogeneous transform");
				}
			}
			if (std::abs(transform[3][3] - 1.0f) > tolerance)
			{
				throw spk::Exception("Voxel Anchor: invalid homogeneous transform");
			}
		}
	}

	Model::Anchor::Anchor(spk::Matrix4x4 transform) :
		_transform(transform)
	{
		validateAnchor(transform);
	}

	Model::Anchor::Anchor(const Anchor &other) :
		_transform(other._transform),
		_attachment(other._attachment)
	{
		if (other._model)
		{
			_model = std::make_unique<Model>(*other._model);
		}
	}

	Model::Anchor::Anchor(Anchor &&other) noexcept = default;

	Model::Anchor &Model::Anchor::operator=(const Anchor &other)
	{
		if (this != &other)
		{
			Anchor copy(other);
			*this = std::move(copy);
		}
		return *this;
	}

	Model::Anchor &Model::Anchor::operator=(Anchor &&other) noexcept = default;

	Model::Anchor::~Anchor() = default;

	const spk::Matrix4x4 &Model::Anchor::transform() const noexcept
	{
		return _transform;
	}

	void Model::Anchor::setTransform(spk::Matrix4x4 transform)
	{
		validateAnchor(transform);
		_transform = transform;
	}

	void Model::Anchor::bind(const Model &model, std::optional<std::string> attachment)
	{
		if (attachment && !model.hasAnchor(*attachment))
		{
			throw spk::Exception("Voxel Anchor: attachment does not exist: " + *attachment);
		}
		auto copy = std::make_unique<Model>(model);
		_attachment.swap(attachment);
		_model.swap(copy);
	}

	void Model::Anchor::unbind() noexcept
	{
		_model.reset();
		_attachment.reset();
	}

	Model *Model::Anchor::model() noexcept
	{
		return _model.get();
	}

	const Model *Model::Anchor::model() const noexcept
	{
		return _model.get();
	}

	const std::optional<std::string> &Model::Anchor::attachment() const noexcept
	{
		return _attachment;
	}

	Model::Model(spk::Voxel::Mesh mesh) :
		_mesh(std::move(mesh))
	{
	}

	Model::Model(
		SurfaceMesh opaque,
		SurfaceMesh translucent,
		spk::ShadowMap::Mesh depth,
		std::vector<Definition::Slot> slots,
		std::uint32_t materialIndexOffset) :
		Model(Mesh(std::move(opaque), std::move(translucent), std::move(depth), std::move(slots), materialIndexOffset))
	{
	}

	const spk::Voxel::Mesh &Model::mesh() const noexcept
	{
		return _mesh;
	}

	Model Model::withMesh(spk::Voxel::Mesh mesh) const
	{
		Model copy(*this);
		copy._mesh = std::move(mesh);
		return copy;
	}

	const SurfaceMesh &Model::opaqueMesh() const noexcept
	{
		return mesh().opaqueMesh();
	}

	const SurfaceMesh &Model::transparentMesh() const noexcept
	{
		return mesh().transparentMesh();
	}

	const spk::ShadowMap::Mesh &Model::depthMesh() const noexcept
	{
		return mesh().depthMesh();
	}

	const std::vector<Definition::Slot> &Model::materialSlots() const noexcept
	{
		return mesh().materialSlots();
	}

	std::uint32_t Model::materialIndexOffset() const noexcept
	{
		return mesh().materialIndexOffset();
	}

	const Definition::Slot &Model::materialSlot(std::uint32_t index) const
	{
		return mesh().materialSlot(index);
	}

	bool Model::isCompatible(const spk::Palette &palette) const noexcept
	{
		for (const auto &slot : materialSlots())
		{
			if (!palette.find(slot.name))
			{
				return false;
			}
		}
		return true;
	}

	void Model::addAnchor(std::string name, spk::Matrix4x4 transform)
	{
		if (name.empty() || hasAnchor(name))
		{
			throw spk::Exception("Voxel Model: duplicate or empty anchor: " + name);
		}
		_anchors.emplace(std::move(name), Anchor(transform));
	}

	void Model::removeAnchor(std::string_view name)
	{
		if (_anchors.erase(std::string(name)) == 0)
		{
			throw spk::Exception("Voxel Model: missing anchor: " + std::string(name));
		}
	}

	bool Model::hasAnchor(std::string_view name) const noexcept
	{
		return _anchors.find(name) != _anchors.end();
	}

	Model::Anchor &Model::anchor(std::string_view name)
	{
		return const_cast<Anchor &>(std::as_const(*this).anchor(name));
	}

	const Model::Anchor &Model::anchor(std::string_view name) const
	{
		const auto found = _anchors.find(name);
		if (found == _anchors.end())
		{
			throw spk::Exception("Voxel Model: missing anchor: " + std::string(name));
		}
		return found->second;
	}

	const std::map<std::string, Model::Anchor, std::less<>> &Model::anchors() const noexcept
	{
		return _anchors;
	}
}
