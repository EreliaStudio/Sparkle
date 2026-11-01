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

	Model::View::View(const View &other) :
		_model(other._model),
		_palettes(other._palettes),
		_rotations(other._rotations),
		_transform(other._transform)
	{
	}

	Model::View::View(View &&other) :
		_model(std::move(other._model)),
		_palettes(std::move(other._palettes)),
		_rotations(std::move(other._rotations)),
		_transform(other._transform)
	{
		other._lifetime.reset();
	}

	Model::View &Model::View::operator=(const View &other)
	{
		if (this != &other)
		{
			View replacement(other);
			*this = std::move(replacement);
		}
		return *this;
	}

	Model::View &Model::View::operator=(View &&other)
	{
		if (this != &other)
		{
			auto lifetime = std::make_shared<std::uint8_t>(0);
			_model = std::move(other._model);
			_palettes = std::move(other._palettes);
			_rotations = std::move(other._rotations);
			_transform = other._transform;
			_lifetime.swap(lifetime);
			other._lifetime.reset();
		}
		return *this;
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

	const Model::Anchor &Model::View::_anchor(const Path &path) const
	{
		if (path.empty())
		{
			throw spk::Exception("Voxel View: empty AnchorPath");
		}
		const Model *node = &_model;
		for (std::size_t index = 0; index < path.size(); ++index)
		{
			const auto &joint = node->anchor(path.segments()[index]);
			if (index + 1 == path.size())
			{
				return joint;
			}
			node = joint.model();
			if (!node)
			{
				throw spk::Exception("Voxel View: unbound intermediate Anchor");
			}
		}
		throw spk::Exception("Voxel View: empty AnchorPath");
	}

	Model::View::AnchorHandle Model::View::anchor(const AnchorPath &path)
	{
		(void)_anchor(path);
		return AnchorHandle(*this, path);
	}

	Model::View::AnchorHandle Model::View::anchor(std::string_view name)
	{
		return anchor(_findUnique(name));
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
		return palette(_findUnique(anchorName));
	}

	const spk::Palette *Model::View::palette(const AnchorPath &path) const
	{
		(void)_node(path);
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
		setPalette(_findUnique(anchorName), std::move(palette), propagation);
	}

	void Model::View::setPalette(
		const AnchorPath &path,
		spk::Palette palette,
		PalettePropagation propagation)
	{
		(void)spk::Voxel::Palette::Builder(palette);
		auto prepared = _palettes;
		_applyPalette(path, palette, propagation, prepared);
		_palettes.swap(prepared);
	}

	void Model::View::setAnchorRotation(std::string_view name, spk::Quaternion rotation)
	{
		setAnchorRotation(_findUnique(name), rotation);
	}

	void Model::View::setAnchorRotation(const AnchorPath &path, spk::Quaternion rotation)
	{
		(void)_anchor(path);
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
		return anchorRotation(_findUnique(name));
	}

	spk::Quaternion Model::View::anchorRotation(const AnchorPath &path) const
	{
		(void)_anchor(path);
		const auto found = _rotations.find(path);
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

	spk::Matrix4x4 Model::View::_childTransform(
		const Model::Anchor &anchor,
		const Path &path,
		spk::Matrix4x4 parent) const
	{
		auto child = parent * anchor.transform();
		const auto rotation = _rotations.find(path);
		if (rotation != _rotations.end())
		{
			child = child * spk::Matrix4x4::rotation(rotation->second);
		}
		if (anchor.attachment())
		{
			child = child * inverseAnchor(anchor.model()->anchor(*anchor.attachment()).transform());
		}
		return child;
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
			const auto child = _childTransform(anchor, path, transform);
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

	Model::View::AnchorHandle::AnchorHandle(View &view, AnchorPath path) :
		_view(&view),
		_path(std::move(path)),
		_lifetime(view._lifetime)
	{
	}

	const Model::AnchorPath &Model::View::AnchorHandle::path() const noexcept
	{
		return _path;
	}

	void Model::View::AnchorHandle::setRotation(spk::Quaternion rotation)
	{
		if (_lifetime.expired())
		{
			throw spk::Exception("Voxel AnchorHandle: View lifetime expired");
		}
		_view->setAnchorRotation(_path, rotation);
	}

	spk::Quaternion Model::View::AnchorHandle::rotation() const
	{
		if (_lifetime.expired())
		{
			throw spk::Exception("Voxel AnchorHandle: View lifetime expired");
		}
		return _view->anchorRotation(_path);
	}

	void Model::View::AnchorHandle::resetRotation()
	{
		setRotation(spk::Quaternion::identity());
	}

	bool Model::View::isRenderable() const noexcept
	{
		try
		{
			visit([](const Model &, const spk::Palette &, const spk::Matrix4x4 &) {
			});
			return true;
		} catch (...)
		{
			return false;
		}
	}
}
