#pragma once

#include <concepts>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include "exception.hpp"
#include "structure/part.hpp"

namespace spk
{
	class Skeleton
	{
	public:
		class Builder;

	private:
		struct Data;
		std::shared_ptr<const Data> _data;

		explicit Skeleton(std::shared_ptr<const Data> data) noexcept;

	public:
		[[nodiscard]] const Part &root() const;
		[[nodiscard]] const Matrix4x4 &attachmentPoint(
			const std::string &partName,
			const std::string &attachmentName) const;

		template <typename T = Part>
			requires std::derived_from<T, Part>
		[[nodiscard]] const T &part(const std::string &name) const
		{
			const Part &result = _part(name);
			const T *typed = dynamic_cast<const T *>(&result);
			if (typed == nullptr)
			{
				throw spk::Exception("Skeleton Part '" + name + "' has an incompatible type");
			}
			return *typed;
		}

	private:
		[[nodiscard]] const Part &_part(const std::string &name) const;
	};

	class Skeleton::Builder
	{
	private:
		std::shared_ptr<Data> _data;

		void _ensureEmpty() const;
		void _setRoot(std::unique_ptr<Part> root);

	public:
		Builder();

		Builder(const Builder &) = delete;
		Builder &operator=(const Builder &) = delete;
		Builder(Builder &&) noexcept = default;
		Builder &operator=(Builder &&) noexcept = default;

		template <typename T>
			requires std::derived_from<std::remove_cvref_t<T>, Part> &&
					 (!std::is_lvalue_reference_v<T>)
		void setRoot(T &&root)
		{
			using Concrete = std::remove_cvref_t<T>;
			_ensureEmpty();
			if (typeid(root) != typeid(Concrete))
			{
				throw spk::Exception("Cannot move a derived root Part through a base reference");
			}
			_setRoot(std::make_unique<Concrete>(std::forward<T>(root)));
		}

		template <typename T, typename... TArgs>
			requires std::derived_from<T, Part> && std::constructible_from<T, TArgs...>
		T &setRoot(TArgs &&...args)
		{
			_ensureEmpty();
			auto root = std::make_unique<T>(std::forward<TArgs>(args)...);
			T &result = *root;
			_setRoot(std::move(root));
			return result;
		}

		[[nodiscard]] Part &root();
		[[nodiscard]] Skeleton build();
	};
}
