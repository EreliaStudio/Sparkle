#include "model3d/object_headless.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
	void validateRotation(const spk::Quaternion &rotation)
	{
		const float lengthSquared = rotation.dot(rotation);
		if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0f)
		{
			throw spk::Exception("AttachmentNode rotation must be a finite non-zero quaternion");
		}
	}

	spk::Quaternion eulerRotation(const spk::Vector3 &euler)
	{
		if (!std::isfinite(euler.x) || !std::isfinite(euler.y) || !std::isfinite(euler.z))
		{
			throw spk::Exception("AttachmentNode Euler angles must be finite");
		}
		return spk::Quaternion::fromEuler(euler);
	}
}

namespace spk::Model3D
{
	AttachmentNode::AttachmentNode(
		ObjectHeadless::ElementNode &owner, const Blueprint::Element::AttachmentPoint &definition) :
		_owner(&owner),
		_definition(&definition),
		_transform(definition.transform())
	{
	}

	AttachmentNode::~AttachmentNode() = default;

	const Blueprint::Element::AttachmentPoint &AttachmentNode::definition() const noexcept
	{
		return *_definition;
	}

	ObjectHeadless::ElementNode &AttachmentNode::owner() noexcept
	{
		return *_owner;
	}

	const ObjectHeadless::ElementNode &AttachmentNode::owner() const noexcept
	{
		return *_owner;
	}

	const Matrix4x4 &AttachmentNode::rotationMatrix() const noexcept
	{
		return _rotationMatrix;
	}

	const Matrix4x4 &AttachmentNode::transform() const noexcept
	{
		return _transform;
	}

	void AttachmentNode::_applyRotation(const Matrix4x4 &rotation)
	{
		_rotationMatrix = rotation;
		_transform = _definition->transform() * _rotationMatrix;
		for (const Attachable *child : _owner->children())
		{
			if (child->parentAttachment() == this)
			{
				child->_invalidateTransform();
			}
		}
	}

	void AttachmentNode::setRotation(const Quaternion &rotation)
	{
		validateRotation(rotation);
		_applyRotation(Matrix4x4::rotation(rotation));
	}

	void AttachmentNode::setRotation(const Vector3 &euler)
	{
		setRotation(eulerRotation(euler));
	}

	void AttachmentNode::rotate(const Quaternion &rotation)
	{
		validateRotation(rotation);
		_applyRotation(_rotationMatrix * Matrix4x4::rotation(rotation));
	}

	void AttachmentNode::rotate(const Vector3 &euler)
	{
		rotate(eulerRotation(euler));
	}

	std::size_t AttachmentNode::attachedCount() const noexcept
	{
		return _bound.size();
	}

	ObjectHeadless &AttachmentNode::_attachOwned(std::unique_ptr<ObjectHeadless> instance)
	{
		if (!instance || instance->parentAttachment() != nullptr)
		{
			throw spk::Exception("AttachmentNode cannot attach an invalid or already attached instance");
		}
		_bound.push_back(std::move(instance));
		try
		{
			_owner->_addChild(*_bound.back());
		} catch (...)
		{
			_bound.pop_back();
			throw;
		}
		ObjectHeadless &result = *_bound.back();
		result._parentAttachment = this;
		result._invalidateTransform();
		return result;
	}

	ObjectHeadless &AttachmentNode::attach(const Blueprint &assembly)
	{
		return _attachOwned(std::make_unique<ObjectHeadless>(assembly));
	}

	ObjectHeadless &AttachmentNode::attach(const ObjectHeadless &instance)
	{
		return _attachOwned(std::make_unique<ObjectHeadless>(instance));
	}

	ObjectHeadless &AttachmentNode::attach(ObjectHeadless &&instance)
	{
		for (const ObjectHeadless *ancestor = &_owner->instance(); ancestor != nullptr;)
		{
			if (ancestor == &instance)
			{
				throw spk::Exception("AttachmentNode cannot attach its own ancestor");
			}
			const AttachmentNode *parent = ancestor->parentAttachment();
			ancestor = parent != nullptr ? &parent->owner().instance() : nullptr;
		}
		return _attachOwned(std::make_unique<ObjectHeadless>(std::move(instance)));
	}

	ObjectHeadless AttachmentNode::detach(ObjectHeadless &instance)
	{
		const auto it = std::ranges::find_if(_bound, [&instance](const auto &entry) {
			return entry.get() == &instance;
		});
		if (it == _bound.end())
		{
			throw spk::Exception("AttachmentNode does not own the requested instance");
		}
		_owner->_removeChild(**it);
		(*it)->_parentAttachment = nullptr;
		(*it)->_invalidateTransform();
		ObjectHeadless detached(std::move(**it));
		_bound.erase(it);
		return detached;
	}
}
