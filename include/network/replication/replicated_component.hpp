#pragma once

#include "engine/component.hpp"
#include "design_pattern/trait/versioned_trait.hpp"
#include "network/message.hpp"
#include "type/uuid.hpp"

namespace spk::Network
{
	class ReplicatedComponent : public spk::Component, public spk::VersionedTrait
	{
	public:
		enum class Mode
		{
			Authoritative,
			Replica
		};

	private:
		spk::UUID _identifier;
		Mode _mode;

	protected:
		// Replicas implement this with MementoTrait::transaction<State>().
		virtual void _readNetworkState(const spk::Message::Reader &reader) = 0;
		virtual void _writeNetworkState(spk::Message::Writer &writer) const = 0;

	public:
		ReplicatedComponent(spk::UUID identifier, Mode mode);
		~ReplicatedComponent() override = default;

		[[nodiscard]] const spk::UUID &identifier() const noexcept;
		[[nodiscard]] Mode mode() const noexcept;
		void capture(spk::Message::Writer &writer) const;
		void apply(const spk::Message::Reader &reader);
	};
}
