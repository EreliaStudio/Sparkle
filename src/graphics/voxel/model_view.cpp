#include "voxel/model.hpp"

#include <cmath>
#include <utility>

#include "exception.hpp"
#include "voxel/palette.hpp"

namespace spk::Voxel
{
	namespace
	{
		[[nodiscard]] spk::Matrix4x4 inverseAnchor(const spk::Matrix4x4 &matrix)
		{
			auto result = spk::Matrix4x4::identity();
			for (std::size_t c = 0; c < 3; ++c)
			{
				for (std::size_t r = 0; r < 3; ++r)
				{
					result[c][r] = matrix[r][c];
				}
			}
			for (std::size_t r = 0; r < 3; ++r)
			{
				result[3][r] = 0;
				for (std::size_t c = 0; c < 3; ++c)
				{
					result[3][r] -= result[c][r] * matrix[3][c];
				}
			}
			return result;
		}
	}

	Model::View::View(Model model) :
		_model(std::move(model))
	{
	}

	Model::View::View(Model model, spk::Palette palette) :
		View(std::move(model))
	{
		setPalette(std::move(palette));
	}

	const Model &Model::View::model() const noexcept
	{
		return _model;
	}

	const spk::Palette *Model::View::palette() const noexcept
	{
		const auto found = _palettes.find({});
		return found == _palettes.end() ? nullptr : &found->second;
	}

	Model::View::Path Model::View::_findUnique(std::string_view name) const
	{
		std::vector<Path> matches;
		Path current;
		const std::function<void(const Model &)> scan = [&](const Model &model) {
			for (const auto &[key, anchor] : model.anchors())
			{
				current.push_back(key);
				if (key == name)
				{
					matches.push_back(current);
				}
				if (anchor.model())
				{
					scan(*anchor.model());
				}
				current.pop_back();
			}
		};
		scan(_model);
		if (matches.size() != 1)
		{
			throw spk::Exception(matches.empty() ? "Voxel View: missing anchor" : "Voxel View: ambiguous anchor");
		}
		return matches.front();
	}

	const Model &Model::View::_node(const Path &path) const
	{
		const Model *model = &_model;
		for (const auto &name : path)
		{
			model = model->anchor(name).model();
			if (!model)
			{
				throw spk::Exception("Voxel View: anchor has no Model");
			}
		}
		return *model;
	}

	const spk::Palette *Model::View::palette(std::string_view anchorName) const
	{
		const auto path = _findUnique(anchorName);
		const auto found = _palettes.find(path);
		return found == _palettes.end() ? nullptr : &found->second;
	}

	void Model::View::_applyPalette(
		Path path,
		const spk::Palette &palette,
		PalettePropagation propagation,
		std::map<Path, spk::Palette> &prepared) const
	{
		const Model &node = _node(path);
		if (!node.mesh().accepts(palette))
		{
			throw spk::Exception("Voxel View: palette is missing required material slots");
		}
		prepared.insert_or_assign(path, palette);
		if (propagation == PalettePropagation::Recursive)
		{
			for (const auto &[name, anchor] : node.anchors())
			{
				if (anchor.model())
				{
					auto child = path;
					child.push_back(name);
					_applyPalette(std::move(child), palette, propagation, prepared);
				}
			}
		}
	}

	void Model::View::setPalette(spk::Palette palette, PalettePropagation propagation)
	{
		(void)spk::Voxel::Palette::Builder(palette);
		auto prepared = _palettes;
		_applyPalette({}, palette, propagation, prepared);
		_palettes.swap(prepared);
	}

	void Model::View::setPalette(
		std::string_view anchorName,
		spk::Palette palette,
		PalettePropagation propagation)
	{
		(void)spk::Voxel::Palette::Builder(palette);
		auto prepared = _palettes;
		_applyPalette(_findUnique(anchorName), palette, propagation, prepared);
		_palettes.swap(prepared);
	}

	void Model::View::setAnchorRotation(std::string_view name, spk::Quaternion rotation)
	{
		const auto path = _findUnique(name);
		for (const float component : rotation.data)
		{
			if (!std::isfinite(component))
			{
				throw spk::Exception("Voxel View: non-finite rotation");
			}
		}
		if (rotation.dot(rotation) <= 0.0f)
		{
			throw spk::Exception("Voxel View: zero rotation");
		}
		_rotations.insert_or_assign(path, rotation.normalized());
	}

	spk::Quaternion Model::View::anchorRotation(std::string_view name) const
	{
		const auto found = _rotations.find(_findUnique(name));
		return found == _rotations.end() ? spk::Quaternion::identity() : found->second;
	}

	void Model::View::setTransform(spk::Matrix4x4 transform)
	{
		for (std::size_t c = 0; c < 4; ++c)
		{
			for (std::size_t r = 0; r < 4; ++r)
			{
				if (!std::isfinite(transform[c][r]))
				{
					throw spk::Exception("Voxel View: non-finite transform");
				}
			}
		}
		_transform = transform;
	}

	const spk::Matrix4x4 &Model::View::transform() const noexcept
	{
		return _transform;
	}

	void Model::View::_visit(
		const Model &model,
		Path &path,
		spk::Matrix4x4 transform,
		const std::function<void(const Model &, const spk::Palette &, const spk::Matrix4x4 &)> &callback) const
	{
		const auto palette = _palettes.find(path);
		if (palette == _palettes.end() && !model.materialSlots().empty())
		{
			throw spk::Exception("Voxel View: unbound Model palette");
		}
		const spk::Palette empty;
		callback(model, palette == _palettes.end() ? empty : palette->second, transform);
		for (const auto &[name, anchor] : model.anchors())
		{
			if (!anchor.model())
			{
				continue;
			}
			path.push_back(name);
			auto child = transform * anchor.transform();
			const auto rotation = _rotations.find(path);
			if (rotation != _rotations.end())
			{
				child = child * spk::Matrix4x4::rotation(rotation->second);
			}
			if (anchor.attachment())
			{
				child = child * inverseAnchor(anchor.model()->anchor(*anchor.attachment()).transform());
			}
			_visit(*anchor.model(), path, child, callback);
			path.pop_back();
		}
	}

	void Model::View::visit(
		const std::function<void(const Model &, const spk::Palette &, const spk::Matrix4x4 &)> &callback) const
	{
		Path path;
		_visit(_model, path, _transform, callback);
	}

	bool Model::View::isRenderable() const noexcept
	{
		try
		{
			visit([](const Model &, const spk::Palette &, const spk::Matrix4x4 &) {});
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
}
