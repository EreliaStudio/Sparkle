#include "structure/puppet.hpp"

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

namespace spk::Assembly
{
	AttachmentNode::AttachmentNode(
		Puppet::ElementNode &owner, const Blueprint::Element::AttachmentPoint &definition) :
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

	Puppet::ElementNode &AttachmentNode::owner() noexcept
	{
		return *_owner;
	}

	const Puppet::ElementNode &AttachmentNode::owner() const noexcept
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

	Puppet &AttachmentNode::_attachOwned(std::unique_ptr<Puppet> instance)
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
		Puppet &result = *_bound.back();
		result._parentAttachment = this;
		result._invalidateTransform();
		return result;
	}

	Puppet &AttachmentNode::attach(const Blueprint &assembly)
	{
		return _attachOwned(std::make_unique<Puppet>(assembly));
	}

	Puppet &AttachmentNode::attach(const Puppet &instance)
	{
		return _attachOwned(std::make_unique<Puppet>(instance));
	}

	Puppet &AttachmentNode::attach(Puppet &&instance)
	{
		for (const Puppet *ancestor = &_owner->instance(); ancestor != nullptr;)
		{
			if (ancestor == &instance)
			{
				throw spk::Exception("AttachmentNode cannot attach its own ancestor");
			}
			const AttachmentNode *parent = ancestor->parentAttachment();
			ancestor = parent != nullptr ? &parent->owner().instance() : nullptr;
		}
		return _attachOwned(std::make_unique<Puppet>(std::move(instance)));
	}

	Puppet AttachmentNode::detach(Puppet &instance)
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
		Puppet detached(std::move(**it));
		_bound.erase(it);
		return detached;
	}
}
