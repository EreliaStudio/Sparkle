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
	class Part : public NameTrait
	{
	public:
		class Attachment
		{
			friend class Part;

		private:
			std::string _name;
			Matrix4x4 _transform = Matrix4x4::identity();
			std::unique_ptr<Part> _child;

		public:
			explicit Attachment(std::string name);
			~Attachment();

			Attachment(const Attachment &) = delete;
			Attachment &operator=(const Attachment &) = delete;
			Attachment(Attachment &&) noexcept;
			Attachment &operator=(Attachment &&) noexcept;

			[[nodiscard]] const std::string &name() const noexcept;
			[[nodiscard]] const Matrix4x4 &transform() const noexcept;
			[[nodiscard]] const Part *child() const noexcept;

			void setPosition(const Vector3 &position);
			void setOrientation(const Quaternion &orientation);
			void setTransform(const Vector3 &position, const Quaternion &orientation = {});
		};

	private:
		Matrix4x4 _anchor = Matrix4x4::identity();
		std::vector<Attachment> _attachments;

		[[nodiscard]] Attachment &_availableAttachment(const std::string &name);

		template <typename T, typename... TArgs>
			requires std::derived_from<T, Part> && std::constructible_from<T, TArgs...>
		T &_emplaceChild(const std::string &name, TArgs &&...args)
		{
			Attachment &entry = _availableAttachment(name);
			auto child = std::make_unique<T>(std::forward<TArgs>(args)...);
			T &result = *child;
			entry._child = std::move(child);
			return result;
		}

	public:
		explicit Part(std::string name);
		virtual ~Part();

		Part(const Part &) = delete;
		Part &operator=(const Part &) = delete;
		Part(Part &&) noexcept;
		Part &operator=(Part &&) = delete;

		[[nodiscard]] const Matrix4x4 &anchor() const noexcept;
		void setAnchor(const Vector3 &position, const Quaternion &orientation = {});

		Attachment &addAttachmentPoint(
			std::string name,
			const Vector3 &position = {},
			const Quaternion &orientation = {});
		void reserveAttachments(std::size_t count);
		[[nodiscard]] Attachment &attachment(const std::string &name);
		[[nodiscard]] const Attachment &attachment(const std::string &name) const;
		[[nodiscard]] const std::vector<Attachment> &attachments() const noexcept;

		template <typename T>
			requires std::derived_from<std::remove_cvref_t<T>, Part> &&
					 (!std::is_lvalue_reference_v<T>)
		std::remove_cvref_t<T> &attach(const std::string &name, T &&child)
		{
			using Concrete = std::remove_cvref_t<T>;
			if (typeid(child) != typeid(Concrete))
			{
				throw spk::Exception("Cannot move a derived Part through a base reference");
			}
			return _emplaceChild<Concrete>(name, std::forward<T>(child));
		}

		template <typename T, typename... TArgs>
			requires std::derived_from<T, Part> && std::constructible_from<T, TArgs...>
		T &attach(const std::string &name, TArgs &&...args)
		{
			return _emplaceChild<T>(name, std::forward<TArgs>(args)...);
		}

		void removeChild(const std::string &name);

		template <typename T = Part>
			requires std::derived_from<T, Part>
		[[nodiscard]] const T *child(const std::string &name) const
		{
			const Part *result = attachment(name).child();
			if (result == nullptr)
			{
				return nullptr;
			}
			const T *typed = dynamic_cast<const T *>(result);
			if (typed == nullptr)
			{
				throw spk::Exception("Part child at '" + name + "' has an incompatible type");
			}
			return typed;
		}
	};
}
