#include "structure/assembly.hpp"

#include <cmath>
#include <utility>

namespace
{
	void validateRotation(const spk::Quaternion &rotation)
	{
		if (!std::isfinite(rotation.x) || !std::isfinite(rotation.y) ||
			!std::isfinite(rotation.z) || !std::isfinite(rotation.w) ||
			rotation.dot(rotation) == 0.0f)
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
		_owner->instance()._invalidateTransform();
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

	Assembly::Instance *Assembly::AttachmentNode::bound() noexcept
	{
		return _bound.get();
	}

	const Assembly::Instance *Assembly::AttachmentNode::bound() const noexcept
	{
		return _bound.get();
	}

	Assembly::Instance &Assembly::AttachmentNode::bind(const Assembly &assembly)
	{
		return bind(std::make_unique<Instance>(assembly));
	}

	Assembly::Instance &Assembly::AttachmentNode::bind(std::unique_ptr<Instance> instance)
	{
		if (_bound || _definition->child() != nullptr || !instance || instance->parentAttachment())
		{
			throw spk::Exception("Assembly::AttachmentNode cannot bind an occupied or invalid instance");
		}
		Instance &result = *instance;
		_owner->_addChild(result);
		result._parentAttachment = this;
		result._invalidateTransform();
		_bound = std::move(instance);
		return result;
	}

	std::unique_ptr<Assembly::Instance> Assembly::AttachmentNode::unbind()
	{
		if (!_bound)
		{
			throw spk::Exception("Assembly::AttachmentNode has no bound instance");
		}
		_owner->_removeChild(*_bound);
		_bound->_parentAttachment = nullptr;
		_bound->_invalidateTransform();
		return std::move(_bound);
	}
}
