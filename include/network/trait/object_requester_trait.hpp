#pragma once
#include "network/replication/request.hpp"
#include "network/replication/sequence.hpp"
#include <map>
#include <optional>
namespace spk::Network
{
	class ObjectRequesterTrait
	{
	public:
		enum class RequestStatus
		{
			Pending,
			Ready,
			Failed
		};

	private:
		struct Acquisition
		{
			spk::Message::RequestID id;
			RequestStatus status = RequestStatus::Pending;
		};
		std::map<ObjectID, Acquisition> _requests;
		Sequence _requestIDs;
		std::size_t _maximumRequests;
		bool _activeRequest = false;
		[[nodiscard]] bool _sendRequest(const Request &request);

	protected:
		// A composed behavior overrides this to share its application-hook guard.
		[[nodiscard]] virtual bool &_requestOperation();
		[[nodiscard]] virtual SessionID _requestSession() const = 0;
		[[nodiscard]] virtual bool _sendObjectRequest(const Request &request) = 0;
		[[nodiscard]] bool _completeAcquisition(ObjectID object, spk::Message::RequestID requestID);
		[[nodiscard]] bool _rejectAcquisition(const Request &request);
		void _eraseAcquisition(ObjectID object);
		void _clearAcquisitions();

	public:
		explicit ObjectRequesterTrait(std::size_t maximumRequests = 65536);
		ObjectRequesterTrait(const ObjectRequesterTrait &) = delete;
		ObjectRequesterTrait &operator=(const ObjectRequesterTrait &) = delete;
		virtual ~ObjectRequesterTrait() = default;
		[[nodiscard]] bool requestObject(ObjectID object);
		void cancelRequest(ObjectID object);
		[[nodiscard]] std::optional<RequestStatus> requestStatus(ObjectID object) const;
	};
}
