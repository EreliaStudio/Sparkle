#include "voxel/model_view.hpp"

#include <functional>
#include <utility>

#include "exception.hpp"
#include "voxel/mesher.hpp"
#include "voxel/volume.hpp"

namespace spk
{
	Skeleton::Part::View::View(Model &model, Part &part) :
		_model(&model),
		_part(&part),
		_root(&model.skeleton().root()),
		_topologyVersion(~std::uint64_t{0})
	{
		_sync();
	}

	Skeleton::Part::View::View(const View &other) :
		_model(other._model),
		_part(other._part),
		_root(other._root),
		_topologyVersion(other._topologyVersion),
		_geometry(other._geometry),
		_bodyVersion(other._bodyVersion),
		_attachmentVersions(other._attachmentVersions)
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

	Skeleton::Part::View::View(View &&other) noexcept = default;
	Skeleton::Part::View &Skeleton::Part::View::operator=(View &&other) noexcept = default;
	Skeleton::Part::View::~View() = default;

	Skeleton::Part::View &Skeleton::Part::View::operator=(const View &other)
	{
		if (this != &other)
		{
			View replacement(other);
			*this = std::move(replacement);
		}
		return *this;
	}

	bool Skeleton::Part::View::_attachmentChanged(const std::string &name, std::uint64_t version) const
	{
		const auto found = _part->joints().find(name);
		if (found == _part->joints().end() || found->second.attachmentVersion() != version)
		{
			return true;
		}
		if (const Part *child = found->second.child())
		{
			return !_children.contains(name) || _children.at(name)->_part != child;
		}
		if (const Model *model = found->second.model())
		{
			return !_models.contains(name) || &_models.at(name)->model() != model;
		}
		return true;
	}

	void Skeleton::Part::View::_dropOutdatedAttachments() const
	{
		for (auto it = _attachmentVersions.begin(); it != _attachmentVersions.end();)
		{
			if (!_attachmentChanged(it->first, it->second))
			{
				++it;
				continue;
			}
			_children.erase(it->first);
			_models.erase(it->first);
			it = _attachmentVersions.erase(it);
		}
	}

	void Skeleton::Part::View::_addMissingAttachments() const
	{
		for (const auto &[name, joint] : _part->joints())
		{
			if (_attachmentVersions.contains(name))
			{
				continue;
			}
			if (Part *child = _part->joint(name).child())
			{
				_children.emplace(name, std::make_unique<View>(*_model, *child));
			}
			else if (Model *model = _part->joint(name).model())
			{
				_models.emplace(name, std::make_unique<Model::View>(*model));
			}
			else
			{
				continue;
			}
			_attachmentVersions.emplace(name, joint.attachmentVersion());
		}
	}

	void Skeleton::Part::View::_sync() const
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
		_dropOutdatedAttachments();
		_addMissingAttachments();
		for (const auto &[name, child] : _children)
		{
			child->_sync();
		}
		_topologyVersion = version;
	}

	const Skeleton::Part &Skeleton::Part::View::part() const
	{
		_sync();
		return *_part;
	}

	const spk::Voxel::Geometry *Skeleton::Part::View::geometry() const
	{
		const Part &source = part();
		const spk::Body *body = source.body();
		if (body == nullptr)
		{
			_geometry.reset();
			return nullptr;
		}
		const auto *volume = dynamic_cast<const spk::Voxel::Volume *>(body);
		if (volume == nullptr)
		{
			throw spk::Exception("Model Part View: unsupported Body type for voxel meshing");
		}
		if (!_geometry || _bodyVersion != source.bodyVersion())
		{
			_geometry = spk::Voxel::Mesher{}.bake(*volume);
			_bodyVersion = source.bodyVersion();
		}
		return &*_geometry;
	}

	Skeleton::Part::View &Skeleton::Part::View::child(std::string_view jointName)
	{
		return const_cast<View &>(std::as_const(*this).child(jointName));
	}

	const Skeleton::Part::View &Skeleton::Part::View::child(std::string_view jointName) const
	{
		_sync();
		const auto found = _children.find(jointName);
		if (found == _children.end())
		{
			throw spk::Exception("Model Part View: joint has no child Part: " + std::string(jointName));
		}
		return *found->second;
	}

	Model::View &Skeleton::Part::View::attachedModel(std::string_view jointName)
	{
		return const_cast<Model::View &>(std::as_const(*this).attachedModel(jointName));
	}

	const Model::View &Skeleton::Part::View::attachedModel(std::string_view jointName) const
	{
		_sync();
		const auto found = _models.find(jointName);
		if (found == _models.end())
		{
			throw spk::Exception("Model Part View: joint has no attached Model: " + std::string(jointName));
		}
		return *found->second;
	}

	void Skeleton::Part::View::_collectMatches(std::string_view name, std::vector<View *> &matches)
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

	Skeleton::Part::View &Model::View::root()
	{
		_sync();
		return _root;
	}

	const Skeleton::Part::View &Model::View::root() const
	{
		_sync();
		return _root;
	}

	Skeleton::Part::View &Model::View::part(const Path &path)
	{
		return const_cast<Skeleton::Part::View &>(std::as_const(*this).part(path));
	}

	const Skeleton::Part::View &Model::View::part(const Path &path) const
	{
		const auto *current = &root();
		for (const auto &name : path)
		{
			current = &current->child(name);
		}
		return *current;
	}

	Skeleton::Part::View &Model::View::part(std::string_view uniqueJointName)
	{
		std::vector<Skeleton::Part::View *> matches;
		root()._collectMatches(uniqueJointName, matches);
		if (matches.size() != 1)
		{
			throw spk::Exception(matches.empty() ? "Model View: missing Part" : "Model View: ambiguous Part");
		}
		return *matches.front();
	}

	const Skeleton::Part::View &Model::View::part(std::string_view uniqueJointName) const
	{
		return const_cast<Model::View &>(*this).part(uniqueJointName);
	}

	void Model::View::bind(spk::Palette palette)
	{
		(void)spk::Voxel::Palette::Builder(palette);
		std::function<void(const Skeleton::Part::View &)> validate = [&](const Skeleton::Part::View &view) {
			if (const auto *geometry = view.geometry(); geometry && !geometry->isCompatible(palette))
			{
				throw spk::Exception("Model View: palette is missing required material slots");
			}
			for (const auto &[name, joint] : view.part().joints())
			{
				if (joint.child())
				{
					validate(view.child(name));
				}
			}
		};
		validate(root());
		_palette = std::move(palette);
	}

	const spk::Palette *Model::View::palette() const noexcept
	{
		return _palette ? &*_palette : nullptr;
	}

	bool Model::View::isRenderable() const
	{
		std::function<bool(const Skeleton::Part::View &)> check = [&](const Skeleton::Part::View &view) {
			const auto *mesh = view.geometry();
			if (mesh && !mesh->materialSlots().empty() &&
				(_palette == std::nullopt || !mesh->isCompatible(*_palette)))
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
}
