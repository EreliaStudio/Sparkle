#pragma once

#include "design_pattern/contract_provider.hpp"
#include "math/vector2.hpp"

#include <limits>

namespace spk
{
	class ResizeableTrait
	{
	public:
		using Contract = ContractProvider<ResizeableTrait *>::Contract;
		using callback_type = ContractProvider<ResizeableTrait *>::callback_type;

		struct SizeHint
		{
			Vector2 minimal = {0, 0};
			Vector2 maximal = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
			Vector2 preferred = {0, 0};

			[[nodiscard]] bool operator==(const SizeHint &other) const = default;
		};

	private:
		void _triggerEdition();

		SizeHint _sizeHint;
		ContractProvider<ResizeableTrait *> _onSizeHintEditionContractProvider;

	public:
		virtual ~ResizeableTrait();

		[[nodiscard]] const SizeHint &sizeHint() const;
		[[nodiscard]] const Vector2 &minimalSize() const;
		[[nodiscard]] const Vector2 &maximalSize() const;
		[[nodiscard]] const Vector2 &preferredSize() const;

		void setSizeHint(const SizeHint &sizeHint);
		void setMinimalSize(const Vector2 &size);
		void setMaximalSize(const Vector2 &size);
		void setPreferredSize(const Vector2 &size);

		Contract subscribeToSizeHintEdition(callback_type job);
	};
}
