#include "structure/assembly.hpp"

#include <algorithm>
#include <utility>

namespace spk
{
	struct Assembly::Instance::Data
	{
		Instance *owner;
		Assembly assembly;
		std::unique_ptr<ElementNode> root;
		std::unordered_map<std::string, ElementNode *> elements;

		explicit Data(Instance &instance, Assembly definition) :
			owner(&instance),
			assembly(std::move(definition))
		{
			root = std::make_unique<ElementNode>(*this, assembly.root(), &instance, nullptr);
			_register(*root);
		}

		void _register(ElementNode &node)
		{
			elements.emplace(node.definition().name(), &node);
			for (const auto &child : node._ownedChildren)
			{
				_register(*child);
			}
		}
	};

	Assembly::Attachable::Attachable(Attachable *structuralParent, AttachmentNode *parentAttachment) :
		_structuralParent(structuralParent),
		_parentAttachment(parentAttachment),
		_globalTransform([this]() {
			return _calculateGlobalTransform();
		})
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

	const std::vector<const Assembly::Attachable *> &Assembly::Attachable::children() const noexcept
	{
		return _children;
	}

	const Matrix4x4 &Assembly::Attachable::globalTransform() const
	{
		return _globalTransform.get();
	}

	Assembly::Instance::Instance(Assembly assembly) :
		_data(std::make_unique<Data>(*this, std::move(assembly)))
	{
		_addChild(*_data->root);
	}

	Assembly::Instance::~Instance() = default;

	void Assembly::Instance::_copyPose(ElementNode &target, const ElementNode &source)
	{
		for (std::size_t i = 0; i < source._attachments.size(); ++i)
		{
			auto &destination = *target._attachments[i];
			const auto &origin = *source._attachments[i];
			destination._rotationMatrix = origin._rotationMatrix;
			destination._transform = origin._transform;
		}
		for (const Attachable *child : source.children())
		{
			if (const auto *bound = dynamic_cast<const Instance *>(child))
			{
				const std::string &name = bound->parentAttachment()->definition().name();
				target.attachment(name).bind(*bound);
			}
		}
		for (std::size_t i = 0; i < source._ownedChildren.size(); ++i)
		{
			_copyPose(*target._ownedChildren[i], *source._ownedChildren[i]);
		}
	}

	Assembly::Instance::Instance(const Instance &other) :
		Instance(other.assembly())
	{
		_copyPose(root(), other.root());
	}

	void Assembly::Instance::_adopt(Instance &source) noexcept
	{
		_data = std::move(source._data);
		_children.swap(source._children);
		source._children.clear();
		if (_data != nullptr)
		{
			_data->owner = this;
			_data->root->_structuralParent = this;
		}
	}

	Assembly::Instance &Assembly::Instance::operator=(const Instance &other)
	{
		if (this != &other)
		{
			Instance copy(other);
			_adopt(copy);
			if (parentAttachment() != nullptr)
			{
				_invalidateTransform();
			}
		}
		return *this;
	}

	Assembly::Instance::Instance(Instance &&other)
	{
		if (other._data == nullptr)
		{
			throw spk::Exception("Cannot move an already moved-from Assembly::Instance");
		}
		if (other.parentAttachment() != nullptr)
		{
			throw spk::Exception("Cannot move an attached Assembly::Instance; unbind it first");
		}
		_adopt(other);
	}

	Assembly::Instance &Assembly::Instance::operator=(Instance &&other)
	{
		if (this == &other)
		{
			return *this;
		}
		if (other._data == nullptr)
		{
			throw spk::Exception("Cannot move an already moved-from Assembly::Instance");
		}
		if (other.parentAttachment() != nullptr)
		{
			throw spk::Exception("Cannot move an attached Assembly::Instance; unbind it first");
		}
		for (const Instance *ancestor = this; ancestor->parentAttachment() != nullptr;)
		{
			ancestor = &ancestor->parentAttachment()->owner().instance();
			if (ancestor == &other)
			{
				throw spk::Exception("Cannot move an ancestor into its own attached descendant");
			}
		}
		_adopt(other);
		if (parentAttachment() != nullptr)
		{
			_invalidateTransform();
		}
		return *this;
	}

	const Matrix4x4 &Assembly::Instance::_localTransform() const noexcept
	{
		static const Matrix4x4 identity = Matrix4x4::identity();
		return identity;
	}

	const Assembly &Assembly::Instance::assembly() const
	{
		if (_data == nullptr)
		{
			throw spk::Exception("Assembly::Instance has been moved from");
		}
		return _data->assembly;
	}

	Assembly::Instance::ElementNode &Assembly::Instance::root()
	{
		if (_data == nullptr)
		{
			throw spk::Exception("Assembly::Instance has been moved from");
		}
		return *_data->root;
	}

	const Assembly::Instance::ElementNode &Assembly::Instance::root() const
	{
		return const_cast<Instance *>(this)->root();
	}

	Assembly::Instance::ElementNode &Assembly::Instance::element(const std::string &name)
	{
		if (_data == nullptr)
		{
			throw spk::Exception("Assembly::Instance has been moved from");
		}
		const auto it = _data->elements.find(name);
		if (it == _data->elements.end())
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
		Instance::Data &instanceData, const Element &definition, Attachable *structuralParent, AttachmentNode *parentAttachment) :
		Attachable(structuralParent, parentAttachment),
		_definition(&definition),
		_instanceData(&instanceData)
	{
		_attachments.reserve(definition.attachmentPoints().size());
		_ownedChildren.reserve(definition.attachmentPoints().size());
		for (const auto &point : definition.attachmentPoints())
		{
			auto attachment = std::make_unique<AttachmentNode>(*this, point);
			if (const Element *child = point.child())
			{
				auto node = std::make_unique<ElementNode>(instanceData, *child, nullptr, attachment.get());
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
		return *_instanceData->owner;
	}

	const Assembly::Instance &Assembly::Instance::ElementNode::instance() const noexcept
	{
		return *_instanceData->owner;
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
