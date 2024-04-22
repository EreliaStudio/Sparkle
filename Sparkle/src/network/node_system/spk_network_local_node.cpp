#include "network/node_system/spk_network_local_node.hpp"

namespace spk::Network
{
	LocalNode::LocalNode()
	{
	}

	void LocalNode::setOnMessageReceptionCallback(const spk::Network::Message::Type& p_id, std::function<void(const spk::Network::Message&)> p_funct)
	{
		if (_onMessageReceptionCallbacks.contains(p_id) == true)
			spk::throwException("Callback already defined for message type [" + std::to_string(p_id) + "]");
		_onMessageReceptionCallbacks[p_id] = std::bind(p_funct, std::placeholders::_1);
	}

	void LocalNode::setUnknowMessageReceptionCallback(std::function<void(const spk::Network::Message&)> p_funct)
	{
		_onUnknowMessageReception = std::bind(p_funct, std::placeholders::_1);
	}


	void LocalNode::send(const spk::Network::Message &p_msg)
	{
		_onMessageReceptionCallbacks[p_msg.header().type()](p_msg);
	}
}