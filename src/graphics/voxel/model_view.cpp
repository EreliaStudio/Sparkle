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
		_topologyVersion(~std::uint64_t{0})
	{
		_sync();
	}

	Model::Skeleton::Part::View::View(const View &other) :
		_model(other._model),
		_part(other._part),
		_root(other._root),
		_topologyVersion(other._topologyVersion),
		_attachmentVersions(other._attachmentVersions),
		_geometry(other._geometry),
		_volumeVersion(other._volumeVersion),
		_palette(other._palette)
	{
		for (const auto &[name, child] : other._children)
		{
			_children.emplace(name, std::make_unique<View>(*child));
		}
		for (const auto &[name, child] : other._models)
		{
			_models.emplace(name, std::make_unique<Model::View>(*child));
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

	void Model::Skeleton::Part::View::_sync() const
	{
		if (_root != &_model->skeleton().root())
		{
			throw spk::Exception("Model Part View: root was replaced; obtain a new View");
		}
		const auto version = _model->version().topology;
		if (_topologyVersion == version)
		{
			return;
		}
		for (auto it = _attachmentVersions.begin(); it != _attachmentVersions.end();)
		{
			const auto found = _part->joints().find(it->first);
			if (found == _part->joints().end() || found->second.attachmentVersion() != it->second ||
				(found->second.child() && (!_children.contains(it->first) || _children.at(it->first)->_part != found->second.child())) ||
				(found->second.model() && (!_models.contains(it->first) || &_models.at(it->first)->model() != found->second.model())))
			{
				_children.erase(it->first);
				_models.erase(it->first);
				it = _attachmentVersions.erase(it);
			}
			else
			{
				++it;
			}
		}
		for (const auto &[name, joint] : _part->joints())
		{
			if (_attachmentVersions.contains(name))
			{
				continue;
			}
			if (const Part *child = joint.child())
			{
				_children.emplace(name, std::make_unique<View>(*_model, *child));
			}
			else if (Model *model = joint.model())
			{
				_models.emplace(name, std::make_unique<Model::View>(*model));
			}
			if (joint.child() || joint.model())
			{
				_attachmentVersions.emplace(name, joint.attachmentVersion());
			}
		}
		for (const auto &[name, child] : _children)
		{
			child->_sync();
		}
		_topologyVersion = version;
	}

	const Model::Skeleton::Part &Model::Skeleton::Part::View::part() const
	{
		_sync();
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
		_sync();
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
		_sync();
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
		_sync();
		const auto found = _children.find(jointName);
		if (found == _children.end())
		{
			throw spk::Exception("Model Part View: joint has no child Part: " + std::string(jointName));
		}
		return *found->second;
	}

	Model::View &Model::Skeleton::Part::View::attachedModel(std::string_view jointName)
	{
		return const_cast<Model::View &>(std::as_const(*this).attachedModel(jointName));
	}

	const Model::View &Model::Skeleton::Part::View::attachedModel(std::string_view jointName) const
	{
		_sync();
		const auto found = _models.find(jointName);
		if (found == _models.end())
		{
			throw spk::Exception("Model Part View: joint has no attached Model: " + std::string(jointName));
		}
		return *found->second;
	}

	void Model::Skeleton::Part::View::_collectMatches(std::string_view name, std::vector<View *> &matches)
	{
		_sync();
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
		for (const auto &[name, joint] : _part->joints())
		{
			auto attached = transform * joint.transform() * spk::Matrix4x4::rotation(joint.rotation());
			if (const Part *child = joint.child())
			{
				if (!joint.attachment().empty())
				{
					attached = attached * inverseAnchor(child->joint(joint.attachment()).transform());
				}
				_children.at(name)->_visit(callback, attached);
			}
			else if (const Model *model = joint.model())
			{
				if (!joint.attachment().empty())
				{
					attached = attached * inverseAnchor(model->skeleton().root().joint(joint.attachment()).transform());
				}
				_models.at(name)->_visit(callback, attached);
			}
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

	void Model::View::_sync() const
	{
		if (_root._root != &_model->skeleton().root())
		{
			_root = Skeleton::Part::View(*_model, _model->skeleton().root());
		}
		_root._sync();
	}

	Model::Skeleton::Part::View &Model::View::root()
	{
		_sync();
		return _root;
	}

	const Model::Skeleton::Part::View &Model::View::root() const
	{
		_sync();
		return _root;
	}

	Model::Skeleton::Part::View &Model::View::part(const Path &path)
	{
		return const_cast<Skeleton::Part::View &>(std::as_const(*this).part(path));
	}

	const Model::Skeleton::Part::View &Model::View::part(const Path &path) const
	{
		const auto *current = &root();
		for (const auto &name : path)
		{
			current = &current->child(name);
		}
		return *current;
	}

	Model::Skeleton::Part::View &Model::View::part(std::string_view uniqueJointName)
	{
		std::vector<Skeleton::Part::View *> matches;
		root()._collectMatches(uniqueJointName, matches);
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
		root().bind(std::move(palette), propagation);
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
				if (joint.model() && !view.attachedModel(name).isRenderable())
				{
					return false;
				}
			}
			return true;
		};
		return check(root());
	}

	void Model::View::_visit(const DrawCallback &callback, spk::Matrix4x4 parent) const
	{
		root()._visit(callback, parent * _model->transform());
	}

	void Model::View::visit(const DrawCallback &callback) const
	{
		_visit(callback, spk::Matrix4x4::identity());
	}
}
