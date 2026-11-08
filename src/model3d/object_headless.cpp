#include "model3d/object_headless.hpp"

#include <algorithm>
#include <utility>

namespace spk::Model3D
{
	struct ObjectHeadless::Data
	{
		ObjectHeadless *owner;
		Blueprint assembly;
		std::unique_ptr<ElementNode> root;
		std::unordered_map<std::string, ElementNode *> elements;

		explicit Data(ObjectHeadless &instance, Blueprint definition) :
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

	Attachable::Attachable(Attachable *structuralParent, AttachmentNode *parentAttachment) :
		_structuralParent(structuralParent),
		_parentAttachment(parentAttachment),
		_globalTransform([this]() {
			return _calculateGlobalTransform();
		})
	{
	}

	Matrix4x4 Attachable::_calculateGlobalTransform() const
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

	void Attachable::_addChild(Attachable &child)
	{
		_children.push_back(&child);
	}

	void Attachable::_removeChild(Attachable &child)
	{
		std::erase(_children, &child);
	}

	void Attachable::_invalidateTransform() const
	{
		_globalTransform.invalidate();
		for (const Attachable *child : _children)
		{
			child->_invalidateTransform();
		}
	}

	AttachmentNode *Attachable::parentAttachment() noexcept
	{
		return _parentAttachment;
	}

	const AttachmentNode *Attachable::parentAttachment() const noexcept
	{
		return _parentAttachment;
	}

	const std::vector<const Attachable *> &Attachable::children() const noexcept
	{
		return _children;
	}

	const Matrix4x4 &Attachable::globalTransform() const
	{
		return _globalTransform.get();
	}

	ObjectHeadless::ObjectHeadless(Blueprint assembly) :
		_data(std::make_unique<Data>(*this, std::move(assembly)))
	{
		_addChild(*_data->root);
	}

	ObjectHeadless::~ObjectHeadless() = default;

	void ObjectHeadless::_copyPose(ElementNode &target, const ElementNode &source)
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
			if (const auto *bound = dynamic_cast<const ObjectHeadless *>(child))
			{
				const std::string &name = bound->parentAttachment()->definition().name();
				target.attachment(name).attach(*bound);
			}
		}
		for (std::size_t i = 0; i < source._ownedChildren.size(); ++i)
		{
			_copyPose(*target._ownedChildren[i], *source._ownedChildren[i]);
		}
	}

	ObjectHeadless::ObjectHeadless(const ObjectHeadless &other) :
		ObjectHeadless(other.blueprint())
	{
		_copyPose(root(), other.root());
	}

	void ObjectHeadless::_adopt(ObjectHeadless &source) noexcept
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

	ObjectHeadless &ObjectHeadless::operator=(const ObjectHeadless &other)
	{
		if (this != &other)
		{
			ObjectHeadless copy(other);
			_adopt(copy);
			if (parentAttachment() != nullptr)
			{
				_invalidateTransform();
			}
		}
		return *this;
	}

	ObjectHeadless::ObjectHeadless(ObjectHeadless &&other)
	{
		if (other._data == nullptr)
		{
			throw spk::Exception("Cannot move an already moved-from ObjectHeadless");
		}
		if (other.parentAttachment() != nullptr)
		{
			throw spk::Exception("Cannot move an attached ObjectHeadless; detach it first");
		}
		_adopt(other);
	}

	ObjectHeadless &ObjectHeadless::operator=(ObjectHeadless &&other)
	{
		if (this == &other)
		{
			return *this;
		}
		if (other._data == nullptr)
		{
			throw spk::Exception("Cannot move an already moved-from ObjectHeadless");
		}
		if (other.parentAttachment() != nullptr)
		{
			throw spk::Exception("Cannot move an attached ObjectHeadless; detach it first");
		}
		for (const ObjectHeadless *ancestor = this; ancestor->parentAttachment() != nullptr;)
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

	const Matrix4x4 &ObjectHeadless::_localTransform() const noexcept
	{
		static const Matrix4x4 identity = Matrix4x4::identity();
		return identity;
	}

	const Blueprint &ObjectHeadless::blueprint() const
	{
		if (_data == nullptr)
		{
			throw spk::Exception("ObjectHeadless has been moved from");
		}
		return _data->assembly;
	}

	ObjectHeadless::ElementNode &ObjectHeadless::root()
	{
		if (_data == nullptr)
		{
			throw spk::Exception("ObjectHeadless has been moved from");
		}
		return *_data->root;
	}

	const ObjectHeadless::ElementNode &ObjectHeadless::root() const
	{
		return const_cast<ObjectHeadless *>(this)->root();
	}

	ObjectHeadless::ElementNode &ObjectHeadless::element(const std::string &name)
	{
		if (_data == nullptr)
		{
			throw spk::Exception("ObjectHeadless has been moved from");
		}
		const auto it = _data->elements.find(name);
		if (it == _data->elements.end())
		{
			throw spk::Exception("Unknown ObjectHeadless Element '" + name + "'");
		}
		return *it->second;
	}

	const ObjectHeadless::ElementNode &ObjectHeadless::element(const std::string &name) const
	{
		return const_cast<ObjectHeadless *>(this)->element(name);
	}

	AttachmentNode &ObjectHeadless::attachment(
		const std::string &elementName, const std::string &attachmentName)
	{
		return element(elementName).attachment(attachmentName);
	}

	const AttachmentNode &ObjectHeadless::attachment(
		const std::string &elementName, const std::string &attachmentName) const
	{
		return element(elementName).attachment(attachmentName);
	}

	ObjectHeadless::ElementNode::ElementNode(
		ObjectHeadless::Data &instanceData, const Blueprint::Element &definition, Attachable *structuralParent, AttachmentNode *parentAttachment) :
		Attachable(structuralParent, parentAttachment),
		_definition(&definition),
		_instanceData(&instanceData)
	{
		_attachments.reserve(definition.attachmentPoints().size());
		_ownedChildren.reserve(definition.attachmentPoints().size());
		for (const auto &point : definition.attachmentPoints())
		{
			auto attachment = std::make_unique<AttachmentNode>(*this, point);
			if (const Blueprint::Element *child = point.child())
			{
				auto node = std::make_unique<ElementNode>(instanceData, *child, nullptr, attachment.get());
				_addChild(*node);
				_ownedChildren.push_back(std::move(node));
			}
			_attachments.push_back(std::move(attachment));
		}
	}

	ObjectHeadless::ElementNode::~ElementNode() = default;

	const Matrix4x4 &ObjectHeadless::ElementNode::_localTransform() const noexcept
	{
		return _definition->anchor();
	}

	const Blueprint::Element &ObjectHeadless::ElementNode::definition() const noexcept
	{
		return *_definition;
	}

	ObjectHeadless &ObjectHeadless::ElementNode::instance() noexcept
	{
		return *_instanceData->owner;
	}

	const ObjectHeadless &ObjectHeadless::ElementNode::instance() const noexcept
	{
		return *_instanceData->owner;
	}

	AttachmentNode &ObjectHeadless::ElementNode::attachment(const std::string &name)
	{
		const auto it = std::ranges::find_if(_attachments, [&name](const auto &node) {
			return node->definition().name() == name;
		});
		if (it == _attachments.end())
		{
			throw spk::Exception("Unknown ObjectHeadless AttachmentNode '" + name + "'");
		}
		return **it;
	}

	const AttachmentNode &ObjectHeadless::ElementNode::attachment(const std::string &name) const
	{
		return const_cast<ElementNode *>(this)->attachment(name);
	}
}
