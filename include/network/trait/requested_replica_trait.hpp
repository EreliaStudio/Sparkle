#pragma once
#include "object_requester_trait.hpp"
#include "replica_trait.hpp"
namespace spk::Network
{
	// Coordinates replica application and acquisition completion without a wire protocol.
	class RequestedReplicaTrait : public ReplicaTrait, public ObjectRequesterTrait
	{
		bool &_requestOperation() override;
		SessionID _requestSession() const override;
		void _onReplicasCleared() override;
		void _onReplicaRemoved(ObjectID id) override;
		bool _receiveRequestedUpdate(const Update<spk::Message> &update, spk::Message::RequestID requestID);

	public:
		explicit RequestedReplicaTrait(std::size_t maximumTracked = 65536);
		~RequestedReplicaTrait() override = default;
		using ReplicaTrait::receiveUpdate;
		[[nodiscard]] bool receiveUpdate(const Update<spk::Message> &update, spk::Message::RequestID requestID);
		[[nodiscard]] bool receiveRejection(const Request &request);
	};
}
