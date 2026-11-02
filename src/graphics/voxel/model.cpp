#include "voxel/model.hpp"

#include <cmath>
#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	namespace
	{
		void validateFinite(const spk::Matrix4x4 &transform)
		{
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
		}

		void validateBasis(const spk::Matrix4x4 &transform)
		{
			constexpr float tolerance = 1.0e-4f;
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

		void validateAnchor(const spk::Matrix4x4 &transform)
		{
			validateFinite(transform);
			validateBasis(transform);
		}
	}

	struct Model::Data
	{
		spk::Voxel::Geometry geometry;
		std::map<std::string, Anchor, std::less<>> anchors;
	};

	Model::Anchor::Anchor(spk::Matrix4x4 transform) :
		_transform(transform)
	{
		validateAnchor(transform);
	}

	const spk::Matrix4x4 &Model::Anchor::transform() const noexcept
	{
		return _transform;
	}

	const Model *Model::Anchor::model() const noexcept
	{
		return _model.get();
	}

	const std::string &Model::Anchor::attachment() const noexcept
	{
		return _attachment;
	}

	Model::Model(std::shared_ptr<const Data> data) noexcept :
		_data(std::move(data))
	{
	}

	Model::Model() :
		_data(std::make_shared<const Data>())
	{
	}

	Model::Model(spk::Voxel::Geometry geometry) :
		_data(std::make_shared<const Data>(Data{std::move(geometry), {}}))
	{
	}

	const spk::Voxel::Geometry &Model::geometry() const noexcept
	{
		return _data->geometry;
	}

	const spk::Voxel::Geometry::SurfaceMesh &Model::opaqueMesh() const noexcept
	{
		return geometry().opaqueMesh();
	}

	const spk::Voxel::Geometry::SurfaceMesh &Model::transparentMesh() const noexcept
	{
		return geometry().transparentMesh();
	}

	const spk::ShadowMap::Mesh &Model::depthMesh() const noexcept
	{
		return geometry().depthMesh();
	}

	const std::vector<Definition::Slot> &Model::materialSlots() const noexcept
	{
		return geometry().materialSlots();
	}

	std::uint32_t Model::materialIndexOffset() const noexcept
	{
		return geometry().materialIndexOffset();
	}

	const Definition::Slot &Model::materialSlot(std::uint32_t index) const
	{
		return geometry().materialSlot(index);
	}

	bool Model::isCompatible(const spk::Palette &palette) const noexcept
	{
		return geometry().isCompatible(palette);
	}

	bool Model::hasAnchor(std::string_view name) const noexcept
	{
		return _data->anchors.find(name) != _data->anchors.end();
	}

	const Model::Anchor &Model::anchor(std::string_view name) const
	{
		const auto found = _data->anchors.find(name);
		if (found == _data->anchors.end())
		{
			throw spk::Exception("Voxel Model: missing anchor: " + std::string(name));
		}
		return found->second;
	}

	const std::map<std::string, Model::Anchor, std::less<>> &Model::anchors() const noexcept
	{
		return _data->anchors;
	}

	Model::Builder::Builder(const Model &source) :
		_geometry(source.geometry()),
		_anchors(source.anchors())
	{
	}

	void Model::Builder::setGeometry(spk::Voxel::Geometry geometry)
	{
		_geometry = std::move(geometry);
	}

	void Model::Builder::addAnchor(std::string name, spk::Matrix4x4 transform)
	{
		if (name.empty() || _anchors.contains(name))
		{
			throw spk::Exception("Voxel Model: duplicate or empty anchor: " + name);
		}
		_anchors.emplace(std::move(name), Anchor(transform));
	}

	void Model::Builder::removeAnchor(std::string_view name)
	{
		if (_anchors.erase(std::string(name)) == 0)
		{
			throw spk::Exception("Voxel Model: missing anchor: " + std::string(name));
		}
	}

	void Model::Builder::setAnchorTransform(std::string_view name, spk::Matrix4x4 transform)
	{
		validateAnchor(transform);
		auto found = _anchors.find(name);
		if (found == _anchors.end())
		{
			throw spk::Exception("Voxel Model: missing anchor: " + std::string(name));
		}
		found->second._transform = transform;
	}

	void Model::Builder::attach(std::string_view name, const Model &model, std::string attachment)
	{
		auto found = _anchors.find(name);
		if (found == _anchors.end())
		{
			throw spk::Exception("Voxel Model: missing anchor: " + std::string(name));
		}
		if (!attachment.empty() && !model.hasAnchor(attachment))
		{
			throw spk::Exception("Voxel Anchor: attachment does not exist: " + attachment);
		}
		auto child = std::make_shared<const Model>(model);
		found->second._model = std::move(child);
		found->second._attachment = std::move(attachment);
	}

	void Model::Builder::detach(std::string_view name)
	{
		auto found = _anchors.find(name);
		if (found == _anchors.end())
		{
			throw spk::Exception("Voxel Model: missing anchor: " + std::string(name));
		}
		found->second._model.reset();
		found->second._attachment.clear();
	}

	Model Model::Builder::build() const
	{
		return Model(std::make_shared<const Data>(Data{_geometry, _anchors}));
	}
}
