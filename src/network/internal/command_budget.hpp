#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <mutex>

namespace spk::NetworkInternal
{
	class CommandBudget final
	{
	public:
		enum class Stage
		{
			Command,
			Transfer,
			Frame,
			InFlight
		};

	private:
		struct Data final
		{
			mutable std::mutex mutex;
			std::size_t maximum;
			std::size_t used = 0;
			std::array<std::size_t, 4> byStage{};
		};

	public:
		class Lease final
		{
			friend class CommandBudget;

		private:
			std::shared_ptr<Data> _data;
			std::size_t _bytes = 0;
			Stage _stage = Stage::Command;

		public:
			Lease() = default;
			Lease(const Lease &) = delete;
			Lease &operator=(const Lease &) = delete;

			~Lease();

			[[nodiscard]] std::size_t bytes() const noexcept;

			void transition(Stage stage) noexcept;
		};

	private:
		std::shared_ptr<Data> _data;

	public:
		explicit CommandBudget(std::size_t maximum);

		[[nodiscard]] std::shared_ptr<Lease> reserve(std::size_t bytes, Stage stage = Stage::Command);
		[[nodiscard]] std::shared_ptr<Lease> tryReserve(std::size_t bytes, Stage stage = Stage::Command);

		void setMaximum(std::size_t maximum) noexcept;

		[[nodiscard]] std::size_t used() const noexcept;

		[[nodiscard]] std::size_t used(Stage stage) const noexcept;
	};
}
