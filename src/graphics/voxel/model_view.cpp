#include "voxel/model_view.hpp"

#include <utility>

#include "exception.hpp"
#include "voxel/mesher.hpp"

namespace spk
{
	namespace
	{
		[[nodiscard]] spk::Matrix4x4 inverseAnchor(const spk::Matrix4x4 &matrix)
		{
			auto result = spk::Matrix4x4::identity();
			for (std::size_t column = 0; column < 3; ++column)
			{
				for (std::size_t row = 0; row < 3; ++row)
				{
					result[column][row] = matrix[row][column];
				}
			}
			for (std::size_t row = 0; row < 3; ++row)
			{
				result[3][row] = 0.0f;
				for (std::size_t column = 0; column < 3; ++column)
				{
					result[3][row] -= result[column][row] * matrix[3][column];
				}
			}
			return result;
		}
	}

	Model::Skeleton::Part::View::View(Model &model, const Part &part) :
		_model(&model),
		_part(&part),
		_root(&model.skeleton().root()),
		_topologyVersion(model.version().topology)
	{
		for (const auto &[name, joint] : part.joints())
		{
			if (const Part *child = joint.child())
			{
				_children.emplace(name, std::make_unique<View>(model, *child));
			}
		}
	}

	Model::Skeleton::Part::View::View(const View &other) :
		_model(other._model),
		_part(other._part),
		_root(other._root),
		_topologyVersion(other._topologyVersion),
		_geometry(other._geometry),
		_volumeVersion(other._volumeVersion),
		_palette(other._palette)
	{
		for (const auto &[name, child] : other._children)
		{
			_children.emplace(name, std::make_unique<View>(*child));
		}
	}

	Model::Skeleton::Part::View::View(View &&other) noexcept = default;
	Model::Skeleton::Part::View &Model::Skeleton::Part::View::operator=(View &&other) noexcept = default;
	Model::Skeleton::Part::View::~View() = default;

	Model::Skeleton::Part::View &Model::Skeleton::Part::View::operator=(const View &other)
	{
		if (this != &other)
		{
			View replacement(other);
			*this = std::move(replacement);
		}
		return *this;
	}

	void Model::Skeleton::Part::View::_checkTopology() const
	{
		if (_root != &_model->skeleton().root() || _topologyVersion != _model->version().topology)
		{
			throw spk::Exception("Model View: Skeleton structure changed; reconstruct the View");
		}
	}

	const Model::Skeleton::Part &Model::Skeleton::Part::View::part() const
	{
		_checkTopology();
		return *_part;
	}

	const spk::Voxel::Geometry *Model::Skeleton::Part::View::geometry() const
	{
		const Part &source = part();
		const auto *volume = source.volume();
		if (volume == nullptr)
		{
			_geometry.reset();
			return nullptr;
		}
		if (!_geometry || _volumeVersion != source.volumeVersion())
		{
			_geometry = spk::Voxel::Mesher{}.bake(*volume);
			_volumeVersion = source.volumeVersion();
		}
		return &*_geometry;
	}

	const spk::Palette *Model::Skeleton::Part::View::palette() const noexcept
	{
		return _palette ? &*_palette : nullptr;
	}

	void Model::Skeleton::Part::View::_validatePalette(const spk::Palette &palette, PalettePropagation propagation) const
	{
		const auto *mesh = geometry();
		if (mesh != nullptr && !mesh->isCompatible(palette))
		{
			throw spk::Exception("Model Part View: palette is missing required material slots");
		}
		if (propagation == PalettePropagation::Recursive)
		{
			for (const auto &[name, child] : _children)
			{
				child->_validatePalette(palette, propagation);
			}
		}
	}

	void Model::Skeleton::Part::View::_applyPalette(const spk::Palette &palette, PalettePropagation propagation)
	{
		_palette = palette;
		if (propagation == PalettePropagation::Recursive)
		{
			for (auto &[name, child] : _children)
			{
				child->_applyPalette(palette, propagation);
			}
		}
	}

	void Model::Skeleton::Part::View::bind(spk::Palette palette, PalettePropagation propagation)
	{
		(void)spk::Voxel::Palette::Builder(palette);
		_validatePalette(palette, propagation);
		_applyPalette(palette, propagation);
	}

	Model::Skeleton::Part::View &Model::Skeleton::Part::View::child(std::string_view jointName)
	{
		return const_cast<View &>(std::as_const(*this).child(jointName));
	}

	const Model::Skeleton::Part::View &Model::Skeleton::Part::View::child(std::string_view jointName) const
	{
		_checkTopology();
		const auto found = _children.find(jointName);
		if (found == _children.end())
		{
			throw spk::Exception("Model Part View: joint has no child Part: " + std::string(jointName));
		}
		return *found->second;
	}

	void Model::Skeleton::Part::View::_collectMatches(std::string_view name, std::vector<View *> &matches)
	{
		_checkTopology();
		for (auto &[key, child] : _children)
		{
			if (key == name)
			{
				matches.push_back(child.get());
			}
			child->_collectMatches(name, matches);
		}
	}

	void Model::Skeleton::Part::View::_visit(
		const std::function<void(const Part &, const spk::Voxel::Geometry &, const spk::Palette &, const spk::Matrix4x4 &)> &callback,
		spk::Matrix4x4 transform) const
	{
		const auto *mesh = geometry();
		if (mesh != nullptr)
		{
			if (!_palette && !mesh->materialSlots().empty())
			{
				throw spk::Exception("Model Part View: missing palette");
			}
			const spk::Palette empty;
			callback(*_part, *mesh, _palette ? *_palette : empty, transform);
		}
		for (const auto &[name, child] : _children)
		{
			const auto &joint = _part->joint(name);
			auto attached = transform * joint.transform() * spk::Matrix4x4::rotation(joint.rotation());
			if (!joint.attachment().empty())
			{
				attached = attached * inverseAnchor(joint.child()->joint(joint.attachment()).transform());
			}
			child->_visit(callback, attached);
		}
	}

	Model::View::View(Model &model) :
		_model(&model),
		_root(model, model.skeleton().root())
	{
	}

	const Model &Model::View::model() const noexcept
	{
		return *_model;
	}

	Model::Skeleton::Part::View &Model::View::root() noexcept
	{
		return _root;
	}

	const Model::Skeleton::Part::View &Model::View::root() const noexcept
	{
		return _root;
	}

	Model::Skeleton::Part::View &Model::View::part(const Path &path)
	{
		return const_cast<Skeleton::Part::View &>(std::as_const(*this).part(path));
	}

	const Model::Skeleton::Part::View &Model::View::part(const Path &path) const
	{
		const auto *current = &_root;
		for (const auto &name : path)
		{
			current = &current->child(name);
		}
		return *current;
	}

	Model::Skeleton::Part::View &Model::View::part(std::string_view uniqueJointName)
	{
		std::vector<Skeleton::Part::View *> matches;
		_root._collectMatches(uniqueJointName, matches);
		if (matches.size() != 1)
		{
			throw spk::Exception(matches.empty() ? "Model View: missing Part" : "Model View: ambiguous Part");
		}
		return *matches.front();
	}

	const Model::Skeleton::Part::View &Model::View::part(std::string_view uniqueJointName) const
	{
		return const_cast<Model::View &>(*this).part(uniqueJointName);
	}

	void Model::View::bind(spk::Palette palette, PalettePropagation propagation)
	{
		_root.bind(std::move(palette), propagation);
	}

	bool Model::View::isRenderable() const
	{
		std::function<bool(const Skeleton::Part::View &)> check = [&](const Skeleton::Part::View &view) {
			const auto *mesh = view.geometry();
			if (mesh && !mesh->materialSlots().empty() &&
				(view.palette() == nullptr || !mesh->isCompatible(*view.palette())))
			{
				return false;
			}
			for (const auto &[name, joint] : view.part().joints())
			{
				if (joint.child() && !check(view.child(name)))
				{
					return false;
				}
			}
			return true;
		};
		return check(_root);
	}

	void Model::View::visit(const DrawCallback &callback) const
	{
		_root._visit(callback, _model->transform());
	}
}
