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

	Model::Anchor::View::View(const Anchor &anchor)
	{
		if (const Model *model = anchor.model())
		{
			_child = std::make_unique<Model::View>(*model);
		}
	}

	Model::Anchor::View::View(const View &other) :
		_rotation(other._rotation)
	{
		if (other._child)
		{
			_child = std::make_unique<Model::View>(*other._child);
		}
	}

	Model::Anchor::View::View(View &&other) noexcept = default;
	Model::Anchor::View &Model::Anchor::View::operator=(View &&other) noexcept = default;
	Model::Anchor::View::~View() = default;

	Model::Anchor::View &Model::Anchor::View::operator=(const View &other)
	{
		if (this != &other)
		{
			View replacement(other);
			*this = std::move(replacement);
		}
		return *this;
	}

	bool Model::Anchor::View::hasChild() const noexcept
	{
		return _child != nullptr;
	}

	Model::View &Model::Anchor::View::child()
	{
		return const_cast<Model::View &>(std::as_const(*this).child());
	}

	const Model::View &Model::Anchor::View::child() const
	{
		if (!_child)
		{
			throw spk::Exception("Voxel Anchor View: no attached Model");
		}
		return *_child;
	}

	void Model::Anchor::View::setRotation(spk::Quaternion rotation)
	{
		for (const float component : rotation.data)
		{
			if (!std::isfinite(component))
			{
				throw spk::Exception("Voxel Anchor View: non-finite rotation");
			}
		}
		if (rotation.dot(rotation) <= 0.0f)
		{
			throw spk::Exception("Voxel Anchor View: zero rotation");
		}
		_rotation = rotation.normalized();
	}

	spk::Quaternion Model::Anchor::View::rotation() const noexcept
	{
		return _rotation;
	}

	void Model::Anchor::View::resetRotation() noexcept
	{
		_rotation = spk::Quaternion::identity();
	}

	Model::View::View(Model model) :
		_model(std::move(model))
	{
		for (const auto &[name, anchor] : _model.anchors())
		{
			_anchors.emplace(name, Anchor::View(anchor));
		}
	}

	Model::View::View(Model model, spk::Palette palette) :
		View(std::move(model))
	{
		bind(std::move(palette));
	}

	Model::View::View(const View &other) = default;
	Model::View::View(View &&other) noexcept = default;
	Model::View &Model::View::operator=(View &&other) noexcept = default;
	Model::View::~View() = default;

	Model::View &Model::View::operator=(const View &other)
	{
		if (this != &other)
		{
			View replacement(other);
			*this = std::move(replacement);
		}
		return *this;
	}

	const Model &Model::View::model() const noexcept
	{
		return _model;
	}

	const spk::Palette *Model::View::palette() const noexcept
	{
		return _palette ? &*_palette : nullptr;
	}

	void Model::View::_collectMatches(
		std::string_view name,
		Anchor::Path &path,
		std::vector<Anchor::Path> &matches) const
	{
		for (const auto &[key, anchor] : _anchors)
		{
			path.push_back(key);
			if (key == name)
			{
				matches.push_back(path);
			}
			if (anchor.hasChild())
			{
				anchor.child()._collectMatches(name, path, matches);
			}
			path.pop_back();
		}
	}

	Model::Anchor::Path Model::View::_findUnique(std::string_view name) const
	{
		std::vector<Anchor::Path> matches;
		Anchor::Path path;
		_collectMatches(name, path, matches);
		if (matches.size() != 1)
		{
			throw spk::Exception(matches.empty() ? "Voxel View: missing anchor" : "Voxel View: ambiguous anchor");
		}
		return matches.front();
	}

	Model::Anchor::View &Model::View::anchor(const Anchor::Path &path)
	{
		return const_cast<Anchor::View &>(std::as_const(*this).anchor(path));
	}

	const Model::Anchor::View &Model::View::anchor(const Anchor::Path &path) const
	{
		if (path.empty())
		{
			throw spk::Exception("Voxel View: empty Anchor::Path");
		}
		const Model::View *node = this;
		for (std::size_t index = 0; index < path.size(); ++index)
		{
			const auto found = node->_anchors.find(path[index]);
			if (found == node->_anchors.end())
			{
				throw spk::Exception("Voxel View: missing anchor: " + path[index]);
			}
			if (index + 1 == path.size())
			{
				return found->second;
			}
			node = &found->second.child();
		}
		throw spk::Exception("Voxel View: empty Anchor::Path");
	}

	Model::Anchor::View &Model::View::anchor(std::string_view uniqueName)
	{
		return anchor(_findUnique(uniqueName));
	}

	const Model::Anchor::View &Model::View::anchor(std::string_view uniqueName) const
	{
		return anchor(_findUnique(uniqueName));
	}

	void Model::View::_validatePalette(const spk::Palette &palette, PalettePropagation propagation) const
	{
		if (!_model.isCompatible(palette))
		{
			throw spk::Exception("Voxel View: palette is missing required material slots");
		}
		if (propagation == PalettePropagation::Recursive)
		{
			for (const auto &[name, anchor] : _anchors)
			{
				if (anchor.hasChild())
				{
					anchor.child()._validatePalette(palette, propagation);
				}
			}
		}
	}

	void Model::View::_applyPalette(const spk::Palette &palette, PalettePropagation propagation)
	{
		_palette = palette;
		if (propagation == PalettePropagation::Recursive)
		{
			for (auto &[name, anchor] : _anchors)
			{
				if (anchor.hasChild())
				{
					anchor.child()._applyPalette(palette, propagation);
				}
			}
		}
	}

	void Model::View::bind(spk::Palette palette, PalettePropagation propagation)
	{
		(void)spk::Voxel::Palette::Builder(palette);
		_validatePalette(palette, propagation);
		_applyPalette(palette, propagation);
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
		const Anchor &anchor,
		const Anchor::View &view,
		spk::Matrix4x4 parent) const
	{
		auto child = parent * anchor.transform() * spk::Matrix4x4::rotation(view.rotation());
		if (!anchor.attachment().empty())
		{
			child = child * inverseAnchor(view.child().model().anchor(anchor.attachment()).transform());
		}
		return child;
	}

	bool Model::View::isRenderable() const
	{
		if (!_model.materialSlots().empty() && (!_palette || !_model.isCompatible(*_palette)))
		{
			return false;
		}
		for (const auto &[name, anchor] : _anchors)
		{
			if (anchor.hasChild() && !anchor.child().isRenderable())
			{
				return false;
			}
		}
		return true;
	}

	void Model::View::_visit(
		const std::function<void(const Model &, const spk::Palette &, const spk::Matrix4x4 &)> &callback,
		spk::Matrix4x4 parent) const
	{
		const auto transform = parent * _transform;
		if (!_palette && !_model.materialSlots().empty())
		{
			throw spk::Exception("Voxel View: unbound Model palette");
		}
		const spk::Palette empty;
		callback(_model, _palette ? *_palette : empty, transform);
		for (const auto &[name, anchor] : _anchors)
		{
			if (anchor.hasChild())
			{
				anchor.child()._visit(callback, _childTransform(_model.anchor(name), anchor, transform));
			}
		}
	}

	void Model::View::visit(
		const std::function<void(const Model &, const spk::Palette &, const spk::Matrix4x4 &)> &callback) const
	{
		_visit(callback, spk::Matrix4x4::identity());
	}
}
