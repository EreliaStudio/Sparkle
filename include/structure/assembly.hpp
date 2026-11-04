#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

#include "design_pattern/trait/name_trait.hpp"
#include "exception.hpp"
#include "math/matrix.hpp"

namespace spk
{
	class Assembly
	{
	public:
		class Element;
		class Builder;

	private:
		struct Data;
		std::shared_ptr<const Data> _data;

		explicit Assembly(std::shared_ptr<const Data> data) noexcept;

	public:
		[[nodiscard]] const Element &root() const;
		[[nodiscard]] const Matrix4x4 &attachmentPoint(
			const std::string &elementName,
			const std::string &attachmentName) const;

		template <typename T = Element>
			requires std::derived_from<T, Element>
		[[nodiscard]] const T &element(const std::string &name) const
		{
			const Element &result = _element(name);
			const T *typed = dynamic_cast<const T *>(&result);
			if (typed == nullptr)
			{
				throw spk::Exception("Assembly Element '" + name + "' has an incompatible type");
			}
			return *typed;
		}

	private:
		[[nodiscard]] const Element &_element(const std::string &name) const;
	};

	class Assembly::Element : public NameTrait
	{
	public:
		class AttachmentPoint : public NameTrait
		{
			friend class Element;

		private:
			Matrix4x4 _transform = Matrix4x4::identity();
			std::unique_ptr<Element> _child;

		public:
			explicit AttachmentPoint(std::string name);
			~AttachmentPoint();

			AttachmentPoint(const AttachmentPoint &) = delete;
			AttachmentPoint &operator=(const AttachmentPoint &) = delete;
			AttachmentPoint(AttachmentPoint &&) noexcept;
			AttachmentPoint &operator=(AttachmentPoint &&);

			[[nodiscard]] const Matrix4x4 &transform() const noexcept;
			[[nodiscard]] const Element *child() const noexcept;

			void setPosition(const Vector3 &position);
			void setOrientation(const Quaternion &orientation);
			void setTransform(const Vector3 &position, const Quaternion &orientation = {});
		};

	private:
		Matrix4x4 _anchor = Matrix4x4::identity();
		std::vector<AttachmentPoint> _attachmentPoints;

		[[nodiscard]] AttachmentPoint &_availableAttachmentPoint(const std::string &name);

		template <typename T, typename... TArgs>
			requires std::derived_from<T, Element> && std::constructible_from<T, TArgs...>
		T &_emplaceChild(const std::string &name, TArgs &&...args)
		{
			AttachmentPoint &entry = _availableAttachmentPoint(name);
			auto child = std::make_unique<T>(std::forward<TArgs>(args)...);
			T &result = *child;
			entry._child = std::move(child);
			return result;
		}

	public:
		explicit Element(std::string name);
		virtual ~Element();

		Element(const Element &) = delete;
		Element &operator=(const Element &) = delete;
		Element(Element &&) noexcept;
		Element &operator=(Element &&) = delete;

		[[nodiscard]] const Matrix4x4 &anchor() const noexcept;
		void setAnchor(const Vector3 &position, const Quaternion &orientation = {});

		AttachmentPoint &addAttachmentPoint(
			std::string name,
			const Vector3 &position = {},
			const Quaternion &orientation = {});
		void reserveAttachmentPoints(std::size_t count);
		[[nodiscard]] AttachmentPoint &attachmentPoint(const std::string &name);
		[[nodiscard]] const AttachmentPoint &attachmentPoint(const std::string &name) const;
		[[nodiscard]] const std::vector<AttachmentPoint> &attachmentPoints() const noexcept;

		template <typename T>
			requires std::derived_from<std::remove_cvref_t<T>, Element> &&
					 (!std::is_lvalue_reference_v<T>)
		std::remove_cvref_t<T> &attach(const std::string &name, T &&child)
		{
			using Concrete = std::remove_cvref_t<T>;
			if (typeid(child) != typeid(Concrete))
			{
				throw spk::Exception("Cannot move a derived Element through a base reference");
			}
			return _emplaceChild<Concrete>(name, std::forward<T>(child));
		}

		template <typename T, typename... TArgs>
			requires std::derived_from<T, Element> && std::constructible_from<T, TArgs...>
		T &attach(const std::string &name, TArgs &&...args)
		{
			return _emplaceChild<T>(name, std::forward<TArgs>(args)...);
		}

		void removeChild(const std::string &name);

		template <typename T = Element>
			requires std::derived_from<T, Element>
		[[nodiscard]] const T *child(const std::string &name) const
		{
			const Element *result = attachmentPoint(name).child();
			if (result == nullptr)
			{
				return nullptr;
			}
			const T *typed = dynamic_cast<const T *>(result);
			if (typed == nullptr)
			{
				throw spk::Exception("Element child at '" + name + "' has an incompatible type");
			}
			return typed;
		}
	};

	class Assembly::Builder
	{
	private:
		std::shared_ptr<Data> _data;

		void _ensureEmpty() const;
		void _setRoot(std::unique_ptr<Element> root);

	public:
		Builder();

		Builder(const Builder &) = delete;
		Builder &operator=(const Builder &) = delete;
		Builder(Builder &&) noexcept = default;
		Builder &operator=(Builder &&) noexcept = default;

		template <typename T>
			requires std::derived_from<std::remove_cvref_t<T>, Element> &&
					 (!std::is_lvalue_reference_v<T>)
		void setRoot(T &&root)
		{
			using Concrete = std::remove_cvref_t<T>;
			_ensureEmpty();
			if (typeid(root) != typeid(Concrete))
			{
				throw spk::Exception("Cannot move a derived root Element through a base reference");
			}
			_setRoot(std::make_unique<Concrete>(std::forward<T>(root)));
		}

		template <typename T, typename... TArgs>
			requires std::derived_from<T, Element> && std::constructible_from<T, TArgs...>
		T &setRoot(TArgs &&...args)
		{
			_ensureEmpty();
			auto root = std::make_unique<T>(std::forward<TArgs>(args)...);
			T &result = *root;
			_setRoot(std::move(root));
			return result;
		}

		[[nodiscard]] Element &root();
		[[nodiscard]] Assembly build();
	};
}
