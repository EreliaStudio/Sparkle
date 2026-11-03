#include "model.hpp"

#include <cmath>
#include <functional>
#include <utility>

#include "exception.hpp"

namespace spk
{
	namespace
	{
		void validateFinite(const spk::Matrix4x4 &matrix)
		{
			for (std::size_t column = 0; column < 4; ++column)
			{
				for (std::size_t row = 0; row < 4; ++row)
				{
					if (!std::isfinite(matrix[column][row]))
					{
						throw spk::Exception("Model: non-finite transformation");
					}
				}
			}
		}

		void validateJointTransform(const spk::Matrix4x4 &matrix)
		{
			validateFinite(matrix);
			constexpr float tolerance = 1.0e-4f;
			for (std::size_t column = 0; column < 3; ++column)
			{
				for (std::size_t other = 0; other < 3; ++other)
				{
					float dot = 0.0f;
					for (std::size_t row = 0; row < 3; ++row)
					{
						dot += matrix[column][row] * matrix[other][row];
					}
					if (std::abs(dot - (column == other ? 1.0f : 0.0f)) > tolerance)
					{
						throw spk::Exception("Model Joint: transformation must be rigid");
					}
				}
				if (std::abs(matrix[column][3]) > tolerance)
				{
					throw spk::Exception("Model Joint: invalid homogeneous transformation");
				}
			}
			if (std::abs(matrix[3][3] - 1.0f) > tolerance)
			{
				throw spk::Exception("Model Joint: invalid homogeneous transformation");
			}
		}
	}

	Model::Model() :
		_skeleton(std::make_unique<Skeleton>())
	{
	}

	Model::Model(Skeleton skeleton) :
		_skeleton(std::make_unique<Skeleton>(std::move(skeleton)))
	{
	}

	Model::Model(const Model &other) :
		_skeleton(std::make_unique<Skeleton>(*other._skeleton)),
		_transform(other._transform),
		_transformVersion(other._transformVersion)
	{
	}

	Model::Model(Model &&other) noexcept = default;
	Model &Model::operator=(Model &&other) noexcept = default;
	Model::~Model() = default;

	Model &Model::operator=(const Model &other)
	{
		if (this != &other)
		{
			Model replacement(other);
			*this = std::move(replacement);
		}
		return *this;
	}

	Model::Skeleton &Model::skeleton() noexcept
	{
		return *_skeleton;
	}

	const Model::Skeleton &Model::skeleton() const noexcept
	{
		return *_skeleton;
	}

	void Model::setTransform(spk::Matrix4x4 transform)
	{
		validateFinite(transform);
		_transform = transform;
		++_transformVersion;
	}

	const spk::Matrix4x4 &Model::transform() const noexcept
	{
		return _transform;
	}

	Model::Version Model::version() const noexcept
	{
		const auto revision = skeleton().revision();
		return {_transformVersion, revision.pose, revision.topology, revision.geometry};
	}

	Model::Skeleton::Skeleton() :
		_root(std::make_unique<Part>()),
		_revision(std::make_shared<Revision>())
	{
		_root->_bindRevision(_revision);
	}

	Model::Skeleton::Skeleton(const Skeleton &other) :
		_root(std::make_unique<Part>(*other._root)),
		_revision(std::make_shared<Revision>(*other._revision))
	{
		_root->_bindRevision(_revision);
	}

	Model::Skeleton::Skeleton(Skeleton &&other) noexcept = default;
	Model::Skeleton &Model::Skeleton::operator=(Skeleton &&other) noexcept = default;
	Model::Skeleton::~Skeleton() = default;

	Model::Skeleton &Model::Skeleton::operator=(const Skeleton &other)
	{
		if (this != &other)
		{
			Skeleton replacement(other);
			*this = std::move(replacement);
		}
		return *this;
	}

	Model::Skeleton::Part &Model::Skeleton::root() noexcept
	{
		return *_root;
	}

	const Model::Skeleton::Part &Model::Skeleton::root() const noexcept
	{
		return *_root;
	}

	Model::Skeleton::Joint &Model::Skeleton::joint(const Path &path)
	{
		if (path.empty())
		{
			throw spk::Exception("Model Skeleton: empty joint path");
		}
		Part *part = &root();
		for (std::size_t index = 0; index < path.size(); ++index)
		{
			Joint &result = part->joint(path[index]);
			if (index + 1 == path.size())
			{
				return result;
			}
			part = result.child();
			if (part == nullptr)
			{
				throw spk::Exception("Model Skeleton: intermediate joint has no Part");
			}
		}
		throw spk::Exception("Model Skeleton: empty joint path");
	}

	const Model::Skeleton::Joint &Model::Skeleton::joint(const Path &path) const
	{
		return const_cast<Skeleton &>(*this).joint(path);
	}

	Model::Skeleton::Joint &Model::Skeleton::joint(std::string_view uniqueName)
	{
		std::vector<Joint *> found;
		std::function<void(Part &)> scan = [&](Part &part) {
			for (const auto &[name, joint] : part.joints())
			{
				if (name == uniqueName)
				{
					found.push_back(&part.joint(name));
				}
				if (auto *child = part.joint(name).child())
				{
					scan(*child);
				}
			}
		};
		scan(root());
		if (found.size() != 1)
		{
			throw spk::Exception(found.empty() ? "Model Skeleton: missing joint" : "Model Skeleton: ambiguous joint");
		}
		return *found.front();
	}

	const Model::Skeleton::Joint &Model::Skeleton::joint(std::string_view uniqueName) const
	{
		return const_cast<Skeleton &>(*this).joint(uniqueName);
	}

	Model::Skeleton::Revision Model::Skeleton::revision() const noexcept
	{
		return *_revision;
	}

	Model::Skeleton::Joint::Joint(spk::Matrix4x4 transform) :
		_transform(transform)
	{
		validateJointTransform(transform);
	}

	Model::Skeleton::Joint::Joint(const Joint &other) :
		_transform(other._transform),
		_rotation(other._rotation),
		_attachment(other._attachment),
		_child(other._child ? std::make_unique<Part>(*other._child) : nullptr),
		_model(other._model ? std::make_unique<Model>(*other._model) : nullptr),
		_attachmentVersion(other._attachmentVersion)
	{
	}

	Model::Skeleton::Joint::Joint(Joint &&other) noexcept = default;
	Model::Skeleton::Joint &Model::Skeleton::Joint::operator=(Joint &&other) noexcept
	{
		if (this != &other)
		{
			_transform = other._transform;
			_rotation = other._rotation;
			_attachment = std::move(other._attachment);
			_child = std::move(other._child);
			_model = std::move(other._model);
			++_attachmentVersion;
			_bindRevision(_revision);
			if (_revision)
			{
				++_revision->pose;
				++_revision->topology;
				++_revision->geometry;
			}
		}
		return *this;
	}
	Model::Skeleton::Joint::~Joint() = default;

	Model::Skeleton::Joint &Model::Skeleton::Joint::operator=(const Joint &other)
	{
		if (this != &other)
		{
			Joint replacement(other);
			replacement._bindRevision(_revision);
			*this = std::move(replacement);
		}
		return *this;
	}

	void Model::Skeleton::Joint::_bindRevision(std::shared_ptr<Revision> revision)
	{
		_revision = std::move(revision);
		if (_child)
		{
			_child->_bindRevision(_revision);
		}
	}

	void Model::Skeleton::Joint::_changed() noexcept
	{
		if (_revision)
		{
			++_revision->pose;
		}
	}

	void Model::Skeleton::Joint::setTransform(spk::Matrix4x4 transform)
	{
		validateJointTransform(transform);
		_transform = transform;
		_changed();
	}

	const spk::Matrix4x4 &Model::Skeleton::Joint::transform() const noexcept
	{
		return _transform;
	}

	void Model::Skeleton::Joint::setRotation(spk::Quaternion rotation)
	{
		for (const float component : rotation.data)
		{
			if (!std::isfinite(component))
			{
				throw spk::Exception("Model Joint: non-finite rotation");
			}
		}
		if (rotation.dot(rotation) <= 0.0f)
		{
			throw spk::Exception("Model Joint: zero rotation");
		}
		_rotation = rotation.normalized();
		_changed();
	}

	spk::Quaternion Model::Skeleton::Joint::rotation() const noexcept
	{
		return _rotation;
	}

	void Model::Skeleton::Joint::attach(Part child, std::string attachment)
	{
		if (!attachment.empty() && !child.joints().contains(attachment))
		{
			throw spk::Exception("Model Joint: missing child attachment: " + attachment);
		}
		auto prepared = std::make_unique<Part>(std::move(child));
		prepared->_bindRevision(_revision);
		_child = std::move(prepared);
		_model.reset();
		_attachment = std::move(attachment);
		++_attachmentVersion;
		if (_revision)
		{
			++_revision->topology;
		}
	}

	void Model::Skeleton::Joint::attach(Model model, std::string attachment)
	{
		if (!attachment.empty() && !model.skeleton().root().joints().contains(attachment))
		{
			throw spk::Exception("Model Joint: missing model attachment: " + attachment);
		}
		auto prepared = std::make_unique<Model>(std::move(model));
		_model = std::move(prepared);
		_child.reset();
		_attachment = std::move(attachment);
		++_attachmentVersion;
		if (_revision)
		{
			++_revision->topology;
		}
	}

	void Model::Skeleton::Joint::detach() noexcept
	{
		_child.reset();
		_model.reset();
		_attachment.clear();
		++_attachmentVersion;
		if (_revision)
		{
			++_revision->topology;
		}
	}

	Model::Skeleton::Part *Model::Skeleton::Joint::child() noexcept
	{
		return _child.get();
	}

	const Model::Skeleton::Part *Model::Skeleton::Joint::child() const noexcept
	{
		return _child.get();
	}

	Model *Model::Skeleton::Joint::model() noexcept
	{
		return _model.get();
	}

	const Model *Model::Skeleton::Joint::model() const noexcept
	{
		return _model.get();
	}

	const std::string &Model::Skeleton::Joint::attachment() const noexcept
	{
		return _attachment;
	}

	std::uint64_t Model::Skeleton::Joint::attachmentVersion() const noexcept
	{
		return _attachmentVersion;
	}

	Model::Skeleton::Part::Part(spk::Voxel::Volume volume) :
		_volume(std::move(volume))
	{
	}

	Model::Skeleton::Part::Part(const Part &other) :
		_volume(other._volume),
		_joints(other._joints),
		_volumeVersion(other._volumeVersion)
	{
	}

	Model::Skeleton::Part::Part(Part &&other) noexcept = default;
	Model::Skeleton::Part::~Part() = default;

	Model::Skeleton::Part &Model::Skeleton::Part::operator=(Part &&other) noexcept
	{
		if (this != &other)
		{
			_volume = std::move(other._volume);
			_joints = std::move(other._joints);
			++_volumeVersion;
			_bindRevision(_revision);
			if (_revision)
			{
				++_revision->topology;
				++_revision->geometry;
			}
		}
		return *this;
	}

	Model::Skeleton::Part &Model::Skeleton::Part::operator=(const Part &other)
	{
		if (this != &other)
		{
			Part replacement(other);
			*this = std::move(replacement);
		}
		return *this;
	}

	void Model::Skeleton::Part::_bindRevision(std::shared_ptr<Revision> revision)
	{
		_revision = std::move(revision);
		for (auto &[name, joint] : _joints)
		{
			joint._bindRevision(_revision);
		}
	}

	void Model::Skeleton::Part::setVolume(spk::Voxel::Volume volume)
	{
		_volume = std::move(volume);
		++_volumeVersion;
		if (_revision)
		{
			++_revision->geometry;
		}
	}

	void Model::Skeleton::Part::clearVolume() noexcept
	{
		_volume.reset();
		++_volumeVersion;
		if (_revision)
		{
			++_revision->geometry;
		}
	}

	const spk::Voxel::Volume *Model::Skeleton::Part::volume() const noexcept
	{
		return _volume ? &*_volume : nullptr;
	}

	std::uint64_t Model::Skeleton::Part::volumeVersion() const noexcept
	{
		return _volumeVersion;
	}

	void Model::Skeleton::Part::addJoint(std::string name, spk::Matrix4x4 transform)
	{
		if (name.empty() || _joints.contains(name))
		{
			throw spk::Exception("Model Part: duplicate or empty joint: " + name);
		}
		auto [it, inserted] = _joints.try_emplace(std::move(name), transform);
		(void)inserted;
		it->second._bindRevision(_revision);
		if (_revision)
		{
			++_revision->topology;
		}
	}

	Model::Skeleton::Joint &Model::Skeleton::Part::joint(std::string_view name)
	{
		return const_cast<Joint &>(std::as_const(*this).joint(name));
	}

	const Model::Skeleton::Joint &Model::Skeleton::Part::joint(std::string_view name) const
	{
		const auto found = _joints.find(name);
		if (found == _joints.end())
		{
			throw spk::Exception("Model Part: missing joint: " + std::string(name));
		}
		return found->second;
	}

	const std::map<std::string, Model::Skeleton::Joint, std::less<>> &Model::Skeleton::Part::joints() const noexcept
	{
		return _joints;
	}
}
