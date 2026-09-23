#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "exception.hpp"

namespace spk
{
	template <typename TElement>
	class Pool
	{
	public:
		using Element = TElement;
		using Factory = std::function<Element *()>;

	private:
		class State
		{
		private:
			std::vector<Element *> _availableElements;
			Factory _factory;

		public:
			explicit State(Factory factory) :
				_factory(std::move(factory))
			{
				if (!_factory)
				{
					throw spk::Exception("Pool requires a valid factory");
				}
			}

			~State()
			{
				clear();
			}

			[[nodiscard]] Element *obtain()
			{
				if (!_availableElements.empty())
				{
					Element *element = _availableElements.back();
					_availableElements.pop_back();

					return element;
				}

				Element *element = _factory();

				if (element == nullptr)
				{
					throw spk::Exception("Pool factory returned a null element");
				}

				return element;
			}

			void recycle(Element *element) noexcept
			{
				try
				{
					_availableElements.push_back(element);
				} catch (...)
				{
					delete element;
				}
			}

			void setFactory(Factory factory)
			{
				if (!factory)
				{
					throw spk::Exception("Pool requires a valid factory");
				}

				_factory = std::move(factory);
			}

			[[nodiscard]] std::shared_ptr<State> cloneEmpty() const
			{
				return std::make_shared<State>(_factory);
			}

			[[nodiscard]] std::size_t available() const
			{
				return _availableElements.size();
			}

			void clear()
			{
				for (Element *element : _availableElements)
				{
					delete element;
				}

				_availableElements.clear();
			}
		};

	public:
		class Lease
		{
		private:
			Element *_element = nullptr;
			std::weak_ptr<State> _state;

			friend class Pool;

			Lease(Element *element, std::weak_ptr<State> state) :
				_element(element),
				_state(std::move(state))
			{
			}

			void _recycle() noexcept
			{
				Element *element = std::exchange(_element, nullptr);

				if (element == nullptr)
				{
					return;
				}

				if (auto state = _state.lock())
				{
					state->recycle(element);
					return;
				}

				delete element;
			}

		public:
			Lease() = default;

			Lease(const Lease &other)
				requires std::is_copy_assignable_v<Element>
			{
				if (other._element == nullptr)
				{
					return;
				}

				auto state = other._state.lock();

				if (state == nullptr)
				{
					throw spk::Exception("Cannot copy a Pool::Lease after its Pool has been destroyed");
				}

				Element *element = state->obtain();

				try
				{
					*element = *other._element;
				} catch (...)
				{
					delete element;
					throw;
				}

				_element = element;
				_state = std::move(state);
			}

			Lease(Lease &&other) noexcept :
				_element(std::exchange(other._element, nullptr)),
				_state(std::move(other._state))
			{
			}

			~Lease()
			{
				_recycle();
			}

			Lease &operator=(const Lease &other)
				requires std::is_copy_assignable_v<Element>
			{
				if (this == &other)
			{
					return *this;
				}

				Lease copy(other);
				*this = std::move(copy);

				return *this;
			}

			Lease &operator=(Lease &&other) noexcept
			{
				if (this == &other)
				{
					return *this;
				}

				_recycle();

				_element = std::exchange(other._element, nullptr);
				_state = std::move(other._state);

				return *this;
			}

			[[nodiscard]] Element *get() noexcept
			{
				return _element;
			}

			[[nodiscard]] const Element *get() const noexcept
			{
				return _element;
			}

			[[nodiscard]] Element &operator*() noexcept
			{
				return *_element;
			}

			[[nodiscard]] const Element &operator*() const noexcept
			{
				return *_element;
			}

			[[nodiscard]] Element *operator->() noexcept
			{
				return _element;
			}

			[[nodiscard]] const Element *operator->() const noexcept
			{
				return _element;
			}

			[[nodiscard]] explicit operator bool() const noexcept
			{
				return _element != nullptr;
			}
		};

	private:
		std::shared_ptr<State> _state;

		[[nodiscard]] static Factory _defaultFactory()
			requires std::default_initializable<Element>
		{
			return []() {
				return new Element();
			};
		}

	public:
		Pool()
			requires std::default_initializable<Element>
			:
			Pool(_defaultFactory())
		{
		}

		explicit Pool(Factory factory) :
			_state(std::make_shared<State>(std::move(factory)))
		{
		}

		Pool(const Pool &other) :
			_state(other._state != nullptr ? other._state->cloneEmpty() : nullptr)
		{
		}

		Pool(Pool &&) noexcept = default;
		~Pool() = default;

		Pool &operator=(const Pool &other)
		{
			if (this == &other)
			{
				return *this;
			}

			std::shared_ptr<State> state =
				other._state != nullptr ? other._state->cloneEmpty() : nullptr;

			_state = std::move(state);

			return *this;
		}

		Pool &operator=(Pool &&) noexcept = default;

		void setFactory(Factory factory)
		{
			_state->setFactory(std::move(factory));
		}

		template <typename TOnObtain, typename... TArguments>
			requires std::invocable<TOnObtain &&, Element &, TArguments &&...>
		[[nodiscard]] Lease obtain(TOnObtain &&onObtain, TArguments &&...arguments)
		{
			Element *element = _state->obtain();

			try
			{
				std::invoke(
					std::forward<TOnObtain>(onObtain),
					*element,
					std::forward<TArguments>(arguments)...);
			} catch (...)
			{
				delete element;
				throw;
			}

			return Lease(element, _state);
		}

		[[nodiscard]] Lease obtain()
		{
			return obtain([](Element &) {
			});
		}

		[[nodiscard]] std::size_t available() const
		{
			return _state->available();
		}

		void clear()
		{
			_state->clear();
		}
	};
}
