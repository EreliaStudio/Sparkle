#pragma once

#include "contract_provider.hpp"
#include "vector2.hpp"

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
		};

		using Contract = ContractProvider<ResizeableTrait *>::Contract;
		using callback_type = ContractProvider<ResizeableTrait *>::callback_type;

		virtual ~ResizeableTrait();

		const SizeHint &sizeHint() const;
		const Vector2 &minimalSize() const;
		const Vector2 &maximalSize() const;
		const Vector2 &preferredSize() const;

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
