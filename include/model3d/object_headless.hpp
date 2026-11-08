#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "model3d/blueprint.hpp"

namespace spk::Model3D
{
	class Attachable
	{
		friend class AttachmentNode;
		friend class ObjectHeadless;

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

	class ObjectHeadless : public Attachable
	{
	public:
		class ElementNode;
		using AttachmentNode = ::spk::Model3D::AttachmentNode;

	private:
		struct Data;
		std::unique_ptr<Data> _data;

		static void _copyPose(ElementNode &target, const ElementNode &source);
		void _adopt(ObjectHeadless &source) noexcept;
		[[nodiscard]] const Matrix4x4 &_localTransform() const noexcept override;

	public:
		explicit ObjectHeadless(Blueprint assembly);
		~ObjectHeadless();

		ObjectHeadless(const ObjectHeadless &other);
		ObjectHeadless &operator=(const ObjectHeadless &other);
		ObjectHeadless(ObjectHeadless &&other);
		ObjectHeadless &operator=(ObjectHeadless &&other);

		[[nodiscard]] const Blueprint &blueprint() const;
		[[nodiscard]] ElementNode &root();
		[[nodiscard]] const ElementNode &root() const;
		[[nodiscard]] ElementNode &element(const std::string &name);
		[[nodiscard]] const ElementNode &element(const std::string &name) const;
		[[nodiscard]] AttachmentNode &attachment(const std::string &elementName, const std::string &attachmentName);
		[[nodiscard]] const AttachmentNode &attachment(const std::string &elementName, const std::string &attachmentName) const;
	};

	class AttachmentNode
	{
		friend class ObjectHeadless;

	private:
		ObjectHeadless::ElementNode *_owner;
		const Blueprint::Element::AttachmentPoint *_definition;
		Matrix4x4 _rotationMatrix = Matrix4x4::identity();
		Matrix4x4 _transform = Matrix4x4::identity();
		std::vector<std::unique_ptr<ObjectHeadless>> _bound;

		void _applyRotation(const Matrix4x4 &rotation);
		ObjectHeadless &_attachOwned(std::unique_ptr<ObjectHeadless> instance);

	public:
		AttachmentNode(ObjectHeadless::ElementNode &owner, const Blueprint::Element::AttachmentPoint &definition);
		~AttachmentNode();

		AttachmentNode(const AttachmentNode &) = delete;
		AttachmentNode &operator=(const AttachmentNode &) = delete;
		AttachmentNode(AttachmentNode &&) = delete;
		AttachmentNode &operator=(AttachmentNode &&) = delete;

		[[nodiscard]] const Blueprint::Element::AttachmentPoint &definition() const noexcept;
		[[nodiscard]] ObjectHeadless::ElementNode &owner() noexcept;
		[[nodiscard]] const ObjectHeadless::ElementNode &owner() const noexcept;
		[[nodiscard]] const Matrix4x4 &rotationMatrix() const noexcept;
		[[nodiscard]] const Matrix4x4 &transform() const noexcept;

		void setRotation(const Quaternion &rotation);
		void setRotation(const Vector3 &euler);
		void rotate(const Quaternion &rotation);
		void rotate(const Vector3 &euler);

		[[nodiscard]] std::size_t attachedCount() const noexcept;
		ObjectHeadless &attach(const Blueprint &assembly);
		ObjectHeadless &attach(const ObjectHeadless &instance);
		ObjectHeadless &attach(ObjectHeadless &&instance);
		[[nodiscard]] ObjectHeadless detach(ObjectHeadless &instance);
	};

	class ObjectHeadless::ElementNode : public Attachable
	{
		friend class ObjectHeadless;
		friend class ::spk::Model3D::AttachmentNode;

	private:
		const Blueprint::Element *_definition;
		ObjectHeadless::Data *_instanceData;
		std::vector<std::unique_ptr<ElementNode>> _ownedChildren;
		std::vector<std::unique_ptr<AttachmentNode>> _attachments;

		[[nodiscard]] const Matrix4x4 &_localTransform() const noexcept override;

	public:
		ElementNode(ObjectHeadless::Data &instanceData, const Blueprint::Element &definition, Attachable *structuralParent, AttachmentNode *parentAttachment);
		~ElementNode();

		ElementNode(const ElementNode &) = delete;
		ElementNode &operator=(const ElementNode &) = delete;
		ElementNode(ElementNode &&) = delete;
		ElementNode &operator=(ElementNode &&) = delete;

		[[nodiscard]] const Blueprint::Element &definition() const noexcept;
		[[nodiscard]] ObjectHeadless &instance() noexcept;
		[[nodiscard]] const ObjectHeadless &instance() const noexcept;
		[[nodiscard]] AttachmentNode &attachment(const std::string &name);
		[[nodiscard]] const AttachmentNode &attachment(const std::string &name) const;
	};

}
