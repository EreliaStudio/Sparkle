#pragma once
#include "container/json/value.hpp"

namespace spk
{
	// Explicit, application-owned snapshots. No automatic transaction or network coupling.
	class MementoTrait
	{
	public:
		using Snapshot = spk::JSON::Value;

	protected:
		[[nodiscard]] virtual Snapshot _save() const = 0;
		virtual void _restore(const Snapshot &snapshot) = 0;

	public:
		virtual ~MementoTrait() = default;
		[[nodiscard]] Snapshot save() const;
		void restore(const Snapshot &snapshot);
	};
}
