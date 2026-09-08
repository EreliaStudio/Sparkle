#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "design_pattern/contract_provider.hpp"

namespace spk
{
	template <typename TType, typename TChildComparator = std::less<TType *>>
	class InherenceTrait
	{
	public:
		using ChildrenContainer = std::vector<TType *>;
		using OnParentEditionContractProvider = ContractProvider<const TType *>;
		using OnParentEditionCallback = OnParentEditionContractProvider::callback_type;
		using OnParentEditionContract = OnParentEditionContractProvider::Contract;

		struct ContinueWhileTruthy
		{
			template <typename TValue>
			[[nodiscard]] bool operator()(const TValue &value) const
				noexcept(noexcept(static_cast<bool>(value)))
			{
				return static_cast<bool>(value);
			}
		};

	private:
		TType *_parent = nullptr;
		bool _destroying = false;
		ChildrenContainer _children;
		TChildComparator _childComparator;
		OnParentEditionContractProvider _onParentEditionContractProvider;

		void _sortChildren()
		{
			std::ranges::sort(_children, _childComparator);
		}

		void _appendChild(TType *child)
		{
			_children.push_back(child);
			_sortChildren();
			_onChildAdded(child);
		}

		void _removeChild(TType *child)
		{
			const auto it = std::ranges::find(_children, child);
			if (it == _children.end())
			{
				return;
			}

			_children.erase(it);
			_onChildRemoved(child);
		}

		void _setParent(TType *parent)
		{
			if (parent != nullptr && (_destroying || parent->_destroying))
			{
				throw std::logic_error("Can't attach an object during hierarchy destruction");
			}
			if (_parent == parent)
			{
				return;
			}

			TType *self = static_cast<TType *>(this);
			for (TType *ancestor = parent; ancestor != nullptr; ancestor = ancestor->_parent)
			{
				if (ancestor == self)
				{
					throw std::logic_error("Can't create a circular hierarchy");
				}
			}

			if (_parent != nullptr)
			{
				_parent->_removeChild(self);
			}

			_parent = parent;

			if (_parent != nullptr)
			{
				_parent->_appendChild(self);
			}

			_onParentEditionContractProvider.trigger(_parent);
		}

	protected:
		virtual void _onChildAdded(TType *)
		{
		}

		virtual void _onChildRemoved(TType *)
		{
		}

	public:
		InherenceTrait() :
			InherenceTrait(TChildComparator{})
		{
		}

		explicit InherenceTrait(TChildComparator comparator) :
			_childComparator(std::move(comparator))
		{
			static_assert(
				std::derived_from<TType, InherenceTrait<TType, TChildComparator>>,
				"TType must inherit from InherenceTrait<TType, TChildComparator>");
		}

		virtual ~InherenceTrait()
		{
			_destroying = true;
			// Only notify surviving objects. Derived members of this object may
			// already be gone; its own parent-edition subscribers must not run.
			if (TType *parent = std::exchange(_parent, nullptr))
			{
				parent->_removeChild(static_cast<TType *>(this));
			}
			while (!_children.empty())
			{
				TType *child = _children.back();
				_children.pop_back();
				child->_parent = nullptr;
				child->_onParentEditionContractProvider.trigger(nullptr);
			}
		}

		OnParentEditionContract subscribeToParentEdition(OnParentEditionCallback callback)
		{
			return _onParentEditionContractProvider.subscribe(std::move(callback));
		}

		void addChild(TType &child)
		{
			child.setParent(*static_cast<TType *>(this));
		}

		void setParent(TType &parent)
		{
			_setParent(&parent);
		}

		void clearParent()
		{
			_setParent(nullptr);
		}

		void removeChild(TType &child)
		{
			if (child._parent != static_cast<TType *>(this))
			{
				throw std::logic_error("The specified object isn't a child of this object");
			}

			child.clearParent();
		}

		void sortChildren()
		{
			_sortChildren();
		}

		void notifyOrderingChange()
		{
			if (_parent != nullptr)
			{
				_parent->_sortChildren();
			}
		}

		[[nodiscard]] const ChildrenContainer &children() const noexcept
		{
			return _children;
		}

		[[nodiscard]] bool hasParent() const noexcept
		{
			return _parent != nullptr;
		}

		[[nodiscard]] TType *parent() noexcept
		{
			return _parent;
		}

		[[nodiscard]] const TType *parent() const noexcept
		{
			return _parent;
		}

		template <typename TValueResolver, typename TContinuePredicate = ContinueWhileTruthy>
			requires std::invocable<const TValueResolver &, const TType &> &&
					 std::predicate<
						 const TContinuePredicate &,
						 const std::remove_cvref_t<std::invoke_result_t<const TValueResolver &, const TType &>> &>
		[[nodiscard]] auto resolveInHierarchy(
			const TValueResolver &resolver,
			const TContinuePredicate &shouldContinue = {}) const
			-> std::remove_cvref_t<std::invoke_result_t<const TValueResolver &, const TType &>>
		{
			using Value = std::remove_cvref_t<std::invoke_result_t<const TValueResolver &, const TType &>>;
			for (const TType *current = static_cast<const TType *>(this);
				 current != nullptr;
				 current = current->_parent)
			{
				Value value = std::invoke(resolver, *current);
				if (current->_parent == nullptr || !std::invoke(shouldContinue, value))
				{
					return value;
				}
			}

			throw std::logic_error("Invalid hierarchy traversal state");
		}
	};
}
