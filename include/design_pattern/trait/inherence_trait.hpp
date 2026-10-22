#pragma once

#include <algorithm>
#include <concepts>
#include <exception>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "design_pattern/contract_provider.hpp"
#include "diagnostics/logger.hpp"

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

		enum class ChildEdition
		{
			Added,
			Removed
		};

		using OnChildEditionContractProvider = ContractProvider<TType *, ChildEdition>;
		using OnChildEditionCallback = OnChildEditionContractProvider::callback_type;
		using OnChildEditionContract = OnChildEditionContractProvider::Contract;

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
		OnChildEditionContractProvider _onChildEditionContractProvider;
		std::exception_ptr _parentFailure;
		std::exception_ptr _childFailure;

		void _sortChildren()
		{
			std::ranges::sort(_children, _childComparator);
		}

		void _appendChild(TType *child)
		{
			_children.push_back(child);
			_sortChildren();
		}

		void _removeChild(TType *child)
		{
			const auto it = std::ranges::find(_children, child);
			if (it == _children.end())
			{
				return;
			}

			_children.erase(it);
		}

		[[nodiscard]] std::exception_ptr _notifyChild(TType *child, ChildEdition edition)
		{
			std::exception_ptr firstFailure;
			try
			{
				if (edition == ChildEdition::Added)
				{
					_onChildAdded(child);
				}
				else
				{
					_onChildRemoved(child);
				}
			} catch (...)
			{
				firstFailure = std::current_exception();
			}

			_childFailure = nullptr;
			try
			{
				_onChildEditionContractProvider.trigger(child, edition);
			} catch (...)
			{
				if (!firstFailure)
				{
					firstFailure = std::current_exception();
				}
			}
			if (!firstFailure)
			{
				firstFailure = _childFailure;
			}
			_childFailure = nullptr;
			return firstFailure;
		}

		[[nodiscard]] std::exception_ptr _notifyParent()
		{
			_parentFailure = nullptr;
			std::exception_ptr firstFailure;
			try
			{
				_onParentEditionContractProvider.trigger(_parent);
			} catch (...)
			{
				firstFailure = std::current_exception();
			}
			if (!firstFailure)
			{
				firstFailure = _parentFailure;
			}
			_parentFailure = nullptr;
			return firstFailure;
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

			TType *previous = _parent;
			if (previous != nullptr)
			{
				previous->_removeChild(self);
			}
			_parent = parent;
			if (parent != nullptr)
			{
				parent->_appendChild(self);
			}

			std::exception_ptr firstFailure;
			if (previous != nullptr)
			{
				firstFailure = previous->_notifyChild(self, ChildEdition::Removed);
			}
			if (parent != nullptr)
			{
				auto failure = parent->_notifyChild(self, ChildEdition::Added);
				if (!firstFailure)
				{
					firstFailure = failure;
				}
			}
			auto parentFailure = _notifyParent();
			if (!firstFailure)
			{
				firstFailure = parentFailure;
			}
			if (firstFailure)
			{
				std::rethrow_exception(firstFailure);
			}
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
			auto logFailure = [](std::exception_ptr failure) noexcept {
				if (!failure)
				{
					return;
				}
				try
				{
					std::rethrow_exception(failure);
				} catch (const std::exception &exception)
				{
					SPK_LOG(Error) << "InherenceTrait notification failed during destruction: " << exception.what() << std::endl;
				} catch (...)
				{
					SPK_LOG(Error) << "InherenceTrait notification failed during destruction." << std::endl;
				}
			};

			if (TType *parent = std::exchange(_parent, nullptr))
			{
				parent->_removeChild(static_cast<TType *>(this));
				logFailure(parent->_notifyChild(static_cast<TType *>(this), ChildEdition::Removed));
			}
			while (!_children.empty())
			{
				TType *child = _children.back();
				_children.pop_back();
				child->_parent = nullptr;
				logFailure(child->_notifyParent());
			}
		}

		OnParentEditionContract subscribeToParentEdition(OnParentEditionCallback callback)
		{
			return _onParentEditionContractProvider.subscribe([this, callback = std::move(callback)](const TType *parent) {
				try
				{
					callback(parent);
				} catch (...)
				{
					if (!_parentFailure)
					{
						_parentFailure = std::current_exception();
					}
				}
			});
		}

		OnChildEditionContract subscribeToChildEdition(OnChildEditionCallback callback)
		{
			return _onChildEditionContractProvider.subscribe([this, callback = std::move(callback)](TType *child, ChildEdition edition) {
				try
				{
					callback(child, edition);
				} catch (...)
				{
					if (!_childFailure)
					{
						_childFailure = std::current_exception();
					}
				}
			});
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
