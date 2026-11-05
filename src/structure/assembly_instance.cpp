#include "structure/assembly.hpp"

#include <algorithm>
#include <utility>

namespace spk
{
	Assembly::Attachable::Attachable(Attachable *structuralParent, AttachmentNode *parentAttachment) :
		_structuralParent(structuralParent),
		_parentAttachment(parentAttachment),
		_globalTransform([this]() { return _calculateGlobalTransform(); })
	{
	}

	Matrix4x4 Assembly::Attachable::_calculateGlobalTransform() const
	{
		if (_parentAttachment != nullptr)
		{
			return _parentAttachment->owner().globalTransform() *
				   _parentAttachment->transform() * _localTransform();
		}
		if (_structuralParent != nullptr)
		{
			return _structuralParent->globalTransform() * _localTransform();
		}
		return _localTransform();
	}

	void Assembly::Attachable::_addChild(Attachable &child)
	{
		_children.push_back(&child);
	}

	void Assembly::Attachable::_removeChild(Attachable &child)
	{
		std::erase(_children, &child);
	}

	void Assembly::Attachable::_invalidateTransform() const
	{
		_globalTransform.invalidate();
		for (const Attachable *child : _children)
		{
			child->_invalidateTransform();
		}
	}

	Assembly::AttachmentNode *Assembly::Attachable::parentAttachment() noexcept
	{
		return _parentAttachment;
	}

	const Assembly::AttachmentNode *Assembly::Attachable::parentAttachment() const noexcept
	{
		return _parentAttachment;
	}

	const std::vector<Assembly::Attachable *> &Assembly::Attachable::children() const noexcept
	{
		return _children;
	}

	const Matrix4x4 &Assembly::Attachable::globalTransform() const
	{
		return _globalTransform.get();
	}

	Assembly::Instance::Instance(Assembly assembly) :
		_assembly(std::move(assembly))
	{
		_root = std::make_unique<ElementNode>(*this, _assembly.root(), this, nullptr);
		_addChild(*_root);
		_register(*_root);
	}

	Assembly::Instance::~Instance() = default;

	void Assembly::Instance::_register(ElementNode &node)
	{
		_elements.emplace(node.definition().name(), &node);
		for (const auto &child : node._ownedChildren)
		{
			_register(*child);
		}
	}

	const Matrix4x4 &Assembly::Instance::_localTransform() const noexcept
	{
		static const Matrix4x4 identity = Matrix4x4::identity();
		return identity;
	}

	const Assembly &Assembly::Instance::assembly() const noexcept
	{
		return _assembly;
	}

	Assembly::Instance::ElementNode &Assembly::Instance::root() noexcept
	{
		return *_root;
	}

	const Assembly::Instance::ElementNode &Assembly::Instance::root() const noexcept
	{
		return *_root;
	}

	Assembly::Instance::ElementNode &Assembly::Instance::element(const std::string &name)
	{
		const auto it = _elements.find(name);
		if (it == _elements.end())
		{
			throw spk::Exception("Unknown Assembly::Instance Element '" + name + "'");
		}
		return *it->second;
	}

	const Assembly::Instance::ElementNode &Assembly::Instance::element(const std::string &name) const
	{
		return const_cast<Instance *>(this)->element(name);
	}

	Assembly::AttachmentNode &Assembly::Instance::attachment(
		const std::string &elementName, const std::string &attachmentName)
	{
		return element(elementName).attachment(attachmentName);
	}

	const Assembly::AttachmentNode &Assembly::Instance::attachment(
		const std::string &elementName, const std::string &attachmentName) const
	{
		return element(elementName).attachment(attachmentName);
	}

	Assembly::Instance::ElementNode::ElementNode(
		Instance &instance, const Element &definition, Attachable *structuralParent, AttachmentNode *parentAttachment) :
		Attachable(structuralParent, parentAttachment),
		_definition(&definition),
		_instance(&instance)
	{
		_attachments.reserve(definition.attachmentPoints().size());
		_ownedChildren.reserve(definition.attachmentPoints().size());
		for (const auto &point : definition.attachmentPoints())
		{
			auto attachment = std::make_unique<AttachmentNode>(*this, point);
			if (const Element *child = point.child())
			{
				auto node = std::make_unique<ElementNode>(instance, *child, nullptr, attachment.get());
				_addChild(*node);
				_ownedChildren.push_back(std::move(node));
			}
			_attachments.push_back(std::move(attachment));
		}
	}

	Assembly::Instance::ElementNode::~ElementNode() = default;

	const Matrix4x4 &Assembly::Instance::ElementNode::_localTransform() const noexcept
	{
		return _definition->anchor();
	}

	const Assembly::Element &Assembly::Instance::ElementNode::definition() const noexcept
	{
		return *_definition;
	}

	Assembly::Instance &Assembly::Instance::ElementNode::instance() noexcept
	{
		return *_instance;
	}

	const Assembly::Instance &Assembly::Instance::ElementNode::instance() const noexcept
	{
		return *_instance;
	}

	Assembly::AttachmentNode &Assembly::Instance::ElementNode::attachment(const std::string &name)
	{
		const auto it = std::ranges::find_if(_attachments, [&name](const auto &node) {
			return node->definition().name() == name;
		});
		if (it == _attachments.end())
		{
			throw spk::Exception("Unknown Assembly::Instance AttachmentNode '" + name + "'");
		}
		return **it;
	}

	const Assembly::AttachmentNode &Assembly::Instance::ElementNode::attachment(const std::string &name) const
	{
		return const_cast<ElementNode *>(this)->attachment(name);
	}
}
