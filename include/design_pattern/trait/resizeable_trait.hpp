#pragma once

#include "design_pattern/contract_provider.hpp"
#include "math/vector2.hpp"

namespace spk
{
	class ResizeableTrait
	{
	public:
		struct SizeHint
		{
			Vector2 minimal;
			Vector2 maximal;
			Vector2 preferred;

			[[nodiscard]] bool operator==(const SizeHint &other) const = default;
		};

		using Contract = ContractProvider<ResizeableTrait *>::Contract;
		using callback_type = ContractProvider<ResizeableTrait *>::callback_type;

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

	private:
		void _triggerEdition();

		SizeHint _sizeHint;
		ContractProvider<ResizeableTrait *> _onSizeHintEditionContractProvider;
	};
}
