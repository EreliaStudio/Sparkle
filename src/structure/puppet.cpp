#include "structure/puppet.hpp"

#include <algorithm>
#include <utility>

namespace spk::Assembly
{
	struct Puppet::Data
	{
		Puppet *owner;
		Blueprint assembly;
		std::unique_ptr<ElementNode> root;
		std::unordered_map<std::string, ElementNode *> elements;

		explicit Data(Puppet &instance, Blueprint definition) :
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

	Puppet::Puppet(Blueprint assembly) :
		_data(std::make_unique<Data>(*this, std::move(assembly)))
	{
		_addChild(*_data->root);
	}

	Puppet::~Puppet() = default;

	void Puppet::_copyPose(ElementNode &target, const ElementNode &source)
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
			if (const auto *bound = dynamic_cast<const Puppet *>(child))
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

	Puppet::Puppet(const Puppet &other) :
		Puppet(other.assembly())
	{
		_copyPose(root(), other.root());
	}

	void Puppet::_adopt(Puppet &source) noexcept
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

	Puppet &Puppet::operator=(const Puppet &other)
	{
		if (this != &other)
		{
			Puppet copy(other);
			_adopt(copy);
			if (parentAttachment() != nullptr)
			{
				_invalidateTransform();
			}
		}
		return *this;
	}

	Puppet::Puppet(Puppet &&other)
	{
		if (other._data == nullptr)
		{
			throw spk::Exception("Cannot move an already moved-from Puppet");
		}
		if (other.parentAttachment() != nullptr)
		{
			throw spk::Exception("Cannot move an attached Puppet; detach it first");
		}
		_adopt(other);
	}

	Puppet &Puppet::operator=(Puppet &&other)
	{
		if (this == &other)
		{
			return *this;
		}
		if (other._data == nullptr)
		{
			throw spk::Exception("Cannot move an already moved-from Puppet");
		}
		if (other.parentAttachment() != nullptr)
		{
			throw spk::Exception("Cannot move an attached Puppet; detach it first");
		}
		for (const Puppet *ancestor = this; ancestor->parentAttachment() != nullptr;)
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

	const Matrix4x4 &Puppet::_localTransform() const noexcept
	{
		static const Matrix4x4 identity = Matrix4x4::identity();
		return identity;
	}

	const Blueprint &Puppet::assembly() const
	{
		if (_data == nullptr)
		{
			throw spk::Exception("Puppet has been moved from");
		}
		return _data->assembly;
	}

	Puppet::ElementNode &Puppet::root()
	{
		if (_data == nullptr)
		{
			throw spk::Exception("Puppet has been moved from");
		}
		return *_data->root;
	}

	const Puppet::ElementNode &Puppet::root() const
	{
		return const_cast<Puppet *>(this)->root();
	}

	Puppet::ElementNode &Puppet::element(const std::string &name)
	{
		if (_data == nullptr)
		{
			throw spk::Exception("Puppet has been moved from");
		}
		const auto it = _data->elements.find(name);
		if (it == _data->elements.end())
		{
			throw spk::Exception("Unknown Puppet Element '" + name + "'");
		}
		return *it->second;
	}

	const Puppet::ElementNode &Puppet::element(const std::string &name) const
	{
		return const_cast<Puppet *>(this)->element(name);
	}

	AttachmentNode &Puppet::attachment(
		const std::string &elementName, const std::string &attachmentName)
	{
		return element(elementName).attachment(attachmentName);
	}

	const AttachmentNode &Puppet::attachment(
		const std::string &elementName, const std::string &attachmentName) const
	{
		return element(elementName).attachment(attachmentName);
	}

	Puppet::ElementNode::ElementNode(
		Puppet::Data &instanceData, const Blueprint::Element &definition, Attachable *structuralParent, AttachmentNode *parentAttachment) :
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

	Puppet::ElementNode::~ElementNode() = default;

	const Matrix4x4 &Puppet::ElementNode::_localTransform() const noexcept
	{
		return _definition->anchor();
	}

	const Blueprint::Element &Puppet::ElementNode::definition() const noexcept
	{
		return *_definition;
	}

	Puppet &Puppet::ElementNode::instance() noexcept
	{
		return *_instanceData->owner;
	}

	const Puppet &Puppet::ElementNode::instance() const noexcept
	{
		return *_instanceData->owner;
	}

	AttachmentNode &Puppet::ElementNode::attachment(const std::string &name)
	{
		const auto it = std::ranges::find_if(_attachments, [&name](const auto &node) {
			return node->definition().name() == name;
		});
		if (it == _attachments.end())
		{
			throw spk::Exception("Unknown Puppet AttachmentNode '" + name + "'");
		}
		return **it;
	}

	const AttachmentNode &Puppet::ElementNode::attachment(const std::string &name) const
	{
		return const_cast<ElementNode *>(this)->attachment(name);
	}
}
