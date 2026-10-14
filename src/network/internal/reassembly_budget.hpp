#pragma once

#include <cstddef>
#include <list>
#include <memory>

namespace spk::NetworkInternal
{
	class ReassemblyBudget final
	{
	private:
		struct Data;

	public:
		class Lease final
		{
			friend class ReassemblyBudget;

		private:
			std::shared_ptr<Data> _data;
			std::list<Lease *>::iterator _registration;
			std::size_t _maximum = 0;
			std::size_t _bytes = 0;

			Lease() = default;

		public:
			~Lease();
			Lease(const Lease &) = delete;
			Lease(Lease &&) = delete;
			Lease &operator=(const Lease &) = delete;
			Lease &operator=(Lease &&) = delete;
			[[nodiscard]] bool reserve(std::size_t bytes);
			void release(std::size_t bytes) noexcept;
		};

	private:
		std::shared_ptr<Data> _data;

	public:
		explicit ReassemblyBudget(std::size_t maximum = 2ull * 1024ull * 1024ull * 1024ull);
		ReassemblyBudget(const ReassemblyBudget &) = delete;
		ReassemblyBudget(ReassemblyBudget &&) = delete;
		ReassemblyBudget &operator=(const ReassemblyBudget &) = delete;
		ReassemblyBudget &operator=(ReassemblyBudget &&) = delete;
		[[nodiscard]] std::unique_ptr<Lease> reserve(std::size_t bytes);
		// Reductions constrain new leases; increases also extend existing allowances.
		void setMaximum(std::size_t bytes);
		[[nodiscard]] std::size_t used() const noexcept;
	};
}
