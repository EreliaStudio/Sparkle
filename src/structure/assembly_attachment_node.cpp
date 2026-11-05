#include "structure/assembly.hpp"

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
			throw spk::Exception("Assembly::AttachmentNode rotation must be a finite non-zero quaternion");
		}
	}

	spk::Quaternion eulerRotation(const spk::Vector3 &euler)
	{
		if (!std::isfinite(euler.x) || !std::isfinite(euler.y) || !std::isfinite(euler.z))
		{
			throw spk::Exception("Assembly::AttachmentNode Euler angles must be finite");
		}
		return spk::Quaternion::fromEuler(euler);
	}
}

namespace spk
{
	Assembly::AttachmentNode::AttachmentNode(
		Instance::ElementNode &owner, const Element::AttachmentPoint &definition) :
		_owner(&owner),
		_definition(&definition),
		_transform(definition.transform())
	{
	}

	Assembly::AttachmentNode::~AttachmentNode() = default;

	const Assembly::Element::AttachmentPoint &Assembly::AttachmentNode::definition() const noexcept
	{
		return *_definition;
	}

	Assembly::Instance::ElementNode &Assembly::AttachmentNode::owner() noexcept
	{
		return *_owner;
	}

	const Assembly::Instance::ElementNode &Assembly::AttachmentNode::owner() const noexcept
	{
		return *_owner;
	}

	const Matrix4x4 &Assembly::AttachmentNode::rotationMatrix() const noexcept
	{
		return _rotationMatrix;
	}

	const Matrix4x4 &Assembly::AttachmentNode::transform() const noexcept
	{
		return _transform;
	}

	void Assembly::AttachmentNode::_applyRotation(const Matrix4x4 &rotation)
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

	void Assembly::AttachmentNode::setRotation(const Quaternion &rotation)
	{
		validateRotation(rotation);
		_applyRotation(Matrix4x4::rotation(rotation));
	}

	void Assembly::AttachmentNode::setRotation(const Vector3 &euler)
	{
		setRotation(eulerRotation(euler));
	}

	void Assembly::AttachmentNode::rotate(const Quaternion &rotation)
	{
		validateRotation(rotation);
		_applyRotation(_rotationMatrix * Matrix4x4::rotation(rotation));
	}

	void Assembly::AttachmentNode::rotate(const Vector3 &euler)
	{
		rotate(eulerRotation(euler));
	}

	std::size_t Assembly::AttachmentNode::boundCount() const noexcept
	{
		return _bound.size();
	}

	Assembly::Instance &Assembly::AttachmentNode::_bindOwned(std::unique_ptr<Instance> instance)
	{
		if (!instance || instance->parentAttachment() != nullptr)
		{
			throw spk::Exception("Assembly::AttachmentNode cannot bind an invalid or attached instance");
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
		Instance &result = *_bound.back();
		result._parentAttachment = this;
		result._invalidateTransform();
		return result;
	}

	Assembly::Instance &Assembly::AttachmentNode::bind(const Assembly &assembly)
	{
		return _bindOwned(std::make_unique<Instance>(assembly));
	}

	Assembly::Instance &Assembly::AttachmentNode::bind(const Instance &instance)
	{
		return _bindOwned(std::make_unique<Instance>(instance));
	}

	Assembly::Instance &Assembly::AttachmentNode::bind(Instance &&instance)
	{
		for (const Instance *ancestor = &_owner->instance(); ancestor != nullptr;)
		{
			if (ancestor == &instance)
			{
				throw spk::Exception("Assembly::AttachmentNode cannot bind its own ancestor");
			}
			const AttachmentNode *parent = ancestor->parentAttachment();
			ancestor = parent != nullptr ? &parent->owner().instance() : nullptr;
		}
		return _bindOwned(std::make_unique<Instance>(std::move(instance)));
	}

	Assembly::Instance Assembly::AttachmentNode::unbind(Instance &instance)
	{
		const auto it = std::ranges::find_if(_bound, [&instance](const auto &entry) {
			return entry.get() == &instance;
		});
		if (it == _bound.end())
		{
			throw spk::Exception("Assembly::AttachmentNode does not own the requested instance");
		}
		_owner->_removeChild(**it);
		(*it)->_parentAttachment = nullptr;
		(*it)->_invalidateTransform();
		Instance detached(std::move(**it));
		_bound.erase(it);
		return detached;
	}
}
