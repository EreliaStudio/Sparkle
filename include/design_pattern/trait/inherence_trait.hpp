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

	private:
		TType *_parent = nullptr;
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

		virtual ~InherenceTrait() = default;

		OnParentEditionContract subscribeToParentEdition(OnParentEditionCallback callback)
		{
			return _onParentEditionContractProvider.subscribe(std::move(callback));
		}

		void addChild(TType *child)
		{
			if (child == nullptr)
			{
				throw std::runtime_error("Can't assign an invalid child pointer");
			}

			child->setParent(static_cast<TType *>(this));
		}

		void setParent(TType *parent)
		{
			if (_parent == parent)
			{
				return;
			}

			TType *self = static_cast<TType *>(this);

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

		void removeChild(TType *child)
		{
			if (child == nullptr)
			{
				throw std::invalid_argument("Can't remove an invalid child pointer");
			}

			if (child->_parent != static_cast<TType *>(this))
			{
				throw std::logic_error("The specified object isn't a child of this object");
			}

			child->setParent(nullptr);
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

		[[nodiscard]] const ChildrenContainer &children() const
		{
			return _children;
		}

		[[nodiscard]] bool hasParent() const
		{
			return _parent != nullptr;
		}

		[[nodiscard]] TType *parent()
		{
			return _parent;
		}

		[[nodiscard]] const TType *parent() const
		{
			return _parent;
		}
	};
}
