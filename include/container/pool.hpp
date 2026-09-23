#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
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
		using OnObtain = std::function<void(Element &)>;

	private:
		struct State
		{
			mutable std::mutex mutex;
			std::vector<Element *> availableElements;

			~State()
			{
				for (Element *element : availableElements)
				{
					delete element;
				}
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
				if (_element == nullptr)
				{
					return;
				}

				if (auto state = _state.lock())
				{
					try
					{
						const std::scoped_lock lock(state->mutex);
						state->availableElements.push_back(_element);
						_element = nullptr;
						return;
					}
					catch (...)
					{
					}
				}

				delete std::exchange(_element, nullptr);
			}

		public:
			Lease() = default;
			Lease(const Lease &) = delete;

			Lease(Lease &&other) noexcept :
				_element(std::exchange(other._element, nullptr)),
				_state(std::move(other._state))
			{
			}

			~Lease()
			{
				_recycle();
			}

			Lease &operator=(const Lease &) = delete;

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
		Factory _factory;
		OnObtain _onObtain;

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

		explicit Pool(OnObtain onObtain)
			requires std::default_initializable<Element>
			:
			Pool(_defaultFactory(), std::move(onObtain))
		{
		}

		explicit Pool(Factory factory, OnObtain onObtain = {}) :
			_state(std::make_shared<State>()),
			_factory(std::move(factory)),
			_onObtain(std::move(onObtain))
		{
			if (!_factory)
			{
				throw spk::Exception("Pool requires a valid factory");
			}
		}

		Pool(const Pool &) = delete;
		Pool(Pool &&) = delete;
		~Pool() = default;

		Pool &operator=(const Pool &) = delete;
		Pool &operator=(Pool &&) = delete;

		[[nodiscard]] Lease obtain()
		{
			Element *element = nullptr;

			{
				const std::scoped_lock lock(_state->mutex);

				if (!_state->availableElements.empty())
				{
					element = _state->availableElements.back();
					_state->availableElements.pop_back();
				}
			}

			if (element == nullptr)
			{
				element = _factory();

				if (element == nullptr)
				{
					throw spk::Exception("Pool factory returned a null element");
				}
			}

			try
			{
				if (_onObtain)
				{
					_onObtain(*element);
				}
			}
			catch (...)
			{
				delete element;
				throw;
			}

			return Lease(element, _state);
		}

		[[nodiscard]] std::size_t available() const
		{
			const std::scoped_lock lock(_state->mutex);
			return _state->availableElements.size();
		}

		void clear()
		{
			std::vector<Element *> elements;

			{
				const std::scoped_lock lock(_state->mutex);
				elements.swap(_state->availableElements);
			}

			for (Element *element : elements)
			{
				delete element;
			}
		}
	};
}
