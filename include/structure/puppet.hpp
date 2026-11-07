#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "structure/blueprint.hpp"

namespace spk::Assembly
{
	class Attachable
	{
		friend class AttachmentNode;
		friend class Puppet;

	private:
		Attachable *_structuralParent;
		AttachmentNode *_parentAttachment;
		std::vector<const Attachable *> _children;
		mutable CachedData<Matrix4x4> _globalTransform;

		[[nodiscard]] Matrix4x4 _calculateGlobalTransform() const;

	protected:
		explicit Attachable(Attachable *structuralParent = nullptr, AttachmentNode *parentAttachment = nullptr);
		void _addChild(Attachable &child);
		void _removeChild(Attachable &child);
		void _invalidateTransform() const;
		[[nodiscard]] virtual const Matrix4x4 &_localTransform() const noexcept = 0;

	public:
		virtual ~Attachable() = default;

		Attachable(const Attachable &) = delete;
		Attachable &operator=(const Attachable &) = delete;
		Attachable(Attachable &&) = delete;
		Attachable &operator=(Attachable &&) = delete;

		[[nodiscard]] AttachmentNode *parentAttachment() noexcept;
		[[nodiscard]] const AttachmentNode *parentAttachment() const noexcept;
		[[nodiscard]] const std::vector<const Attachable *> &children() const noexcept;
		[[nodiscard]] const Matrix4x4 &globalTransform() const;
	};

	class Puppet : public Attachable
	{
	public:
		class ElementNode;
		using AttachmentNode = ::spk::Assembly::AttachmentNode;

	private:
		struct Data;
		std::unique_ptr<Data> _data;

		static void _copyPose(ElementNode &target, const ElementNode &source);
		void _adopt(Puppet &source) noexcept;
		[[nodiscard]] const Matrix4x4 &_localTransform() const noexcept override;

	public:
		explicit Puppet(Blueprint assembly);
		~Puppet();

		Puppet(const Puppet &other);
		Puppet &operator=(const Puppet &other);
		Puppet(Puppet &&other);
		Puppet &operator=(Puppet &&other);

		[[nodiscard]] const Blueprint &assembly() const;
		[[nodiscard]] ElementNode &root();
		[[nodiscard]] const ElementNode &root() const;
		[[nodiscard]] ElementNode &element(const std::string &name);
		[[nodiscard]] const ElementNode &element(const std::string &name) const;
		[[nodiscard]] AttachmentNode &attachment(const std::string &elementName, const std::string &attachmentName);
		[[nodiscard]] const AttachmentNode &attachment(const std::string &elementName, const std::string &attachmentName) const;
	};

	class AttachmentNode
	{
		friend class Puppet;

	private:
		Puppet::ElementNode *_owner;
		const Element::AttachmentPoint *_definition;
		Matrix4x4 _rotationMatrix = Matrix4x4::identity();
		Matrix4x4 _transform = Matrix4x4::identity();
		std::vector<std::unique_ptr<Puppet>> _bound;

		void _applyRotation(const Matrix4x4 &rotation);
		Puppet &_attachOwned(std::unique_ptr<Puppet> instance);

	public:
		AttachmentNode(Puppet::ElementNode &owner, const Element::AttachmentPoint &definition);
		~AttachmentNode();

		AttachmentNode(const AttachmentNode &) = delete;
		AttachmentNode &operator=(const AttachmentNode &) = delete;
		AttachmentNode(AttachmentNode &&) = delete;
		AttachmentNode &operator=(AttachmentNode &&) = delete;

		[[nodiscard]] const Element::AttachmentPoint &definition() const noexcept;
		[[nodiscard]] Puppet::ElementNode &owner() noexcept;
		[[nodiscard]] const Puppet::ElementNode &owner() const noexcept;
		[[nodiscard]] const Matrix4x4 &rotationMatrix() const noexcept;
		[[nodiscard]] const Matrix4x4 &transform() const noexcept;

		void setRotation(const Quaternion &rotation);
		void setRotation(const Vector3 &euler);
		void rotate(const Quaternion &rotation);
		void rotate(const Vector3 &euler);

		[[nodiscard]] std::size_t attachedCount() const noexcept;
		Puppet &attach(const Blueprint &assembly);
		Puppet &attach(const Puppet &instance);
		Puppet &attach(Puppet &&instance);
		[[nodiscard]] Puppet detach(Puppet &instance);
	};

	class Puppet::ElementNode : public Attachable
	{
		friend class Puppet;
		friend class AttachmentNode;

	private:
		const Element *_definition;
		Puppet::Data *_instanceData;
		std::vector<std::unique_ptr<ElementNode>> _ownedChildren;
		std::vector<std::unique_ptr<AttachmentNode>> _attachments;

		[[nodiscard]] const Matrix4x4 &_localTransform() const noexcept override;

	public:
		ElementNode(Puppet::Data &instanceData, const Element &definition, Attachable *structuralParent, AttachmentNode *parentAttachment);
		~ElementNode();

		ElementNode(const ElementNode &) = delete;
		ElementNode &operator=(const ElementNode &) = delete;
		ElementNode(ElementNode &&) = delete;
		ElementNode &operator=(ElementNode &&) = delete;

		[[nodiscard]] const Element &definition() const noexcept;
		[[nodiscard]] Puppet &instance() noexcept;
		[[nodiscard]] const Puppet &instance() const noexcept;
		[[nodiscard]] AttachmentNode &attachment(const std::string &name);
		[[nodiscard]] const AttachmentNode &attachment(const std::string &name) const;
	};

}
