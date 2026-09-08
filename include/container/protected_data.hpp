#pragma once

#include <mutex>
#include <shared_mutex>
#include <utility>

namespace spk
{
	template <typename TValue>
	class ProtectedData final
	{
		/**
		 * Reader and Writer are non-owning lock guards.
		 *
		 * They must not outlive the ProtectedData instance from which they were created.
		 * Destroying the ProtectedData while one of its guards still exists results in
		 * undefined behavior.
		 */
	public:
		using value_type = TValue;

		class Reader final
		{
			friend class ProtectedData;
		
		private:
			explicit Reader(const ProtectedData &owner) :
				_owner(owner),
				_lock(owner._mutex)
			{
			}

			const ProtectedData &_owner;
			std::shared_lock<std::shared_mutex> _lock;

		public:
			Reader(const Reader &) = delete;
			Reader &operator=(const Reader &) = delete;
			Reader(Reader &&) = delete;
			Reader &operator=(Reader &&) = delete;

			[[nodiscard]] const value_type &operator*() const noexcept
			{
				return _owner._value;
			}

			[[nodiscard]] const value_type *operator->() const noexcept
			{
				return &_owner._value;
			}
		};

		class Writer final
		{
			friend class ProtectedData;

		private:
			explicit Writer(ProtectedData &owner) :
				_owner(owner),
				_lock(owner._mutex)
			{
			}

			ProtectedData &_owner;
			std::unique_lock<std::shared_mutex> _lock;

		public:
			Writer(const Writer &) = delete;
			Writer &operator=(const Writer &) = delete;
			Writer(Writer &&) = delete;
			Writer &operator=(Writer &&) = delete;

			[[nodiscard]] value_type &operator*() noexcept
			{
				return _owner._value;
			}

			[[nodiscard]] value_type *operator->() noexcept
			{
				return &_owner._value;
			}
		};
	

	private:
		value_type _value{};
		mutable std::shared_mutex _mutex;

	public:
		ProtectedData() = default;

		explicit ProtectedData(value_type value) :
			_value(std::move(value))
		{
		}

		ProtectedData(const ProtectedData &) = delete;
		ProtectedData(ProtectedData &&) = delete;

		ProtectedData &operator=(const ProtectedData &) = delete;
		ProtectedData &operator=(ProtectedData &&) = delete;

		[[nodiscard]] Reader read() const
		{
			return Reader(*this);
		}

		[[nodiscard]] Writer write()
		{
			return Writer(*this);
		}
	};
}