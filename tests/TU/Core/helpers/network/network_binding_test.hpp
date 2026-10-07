#pragma once

#include "network/network.hpp"
#include "network_test_utils.hpp"
#include <gtest/gtest.h>
#include <map>
#include <thread>

class NetworkBindingTest : public testing::Test
{
protected:
	using ID = spk::UUID;
	using Clock = spk::Network::Clock;
	class Server : public spk::Server
	{
	public:
		~Server()
		{
			stop();
		}
	};
	class Client : public spk::Client
	{
	public:
		~Client()
		{
			disconnect();
		}
	};
	static spk::Message payload(int value)
	{
		spk::Message::Writer writer;
		writer << value;
		return std::move(writer).build();
	}
	using Protocol = spk::Network::Protocol;
	class Object : public spk::Network::PublishableTrait, public spk::Network::ReplicableTrait
	{
		void _writeNetworkState(spk::Message::Writer &writer) const override
		{
			writer << value;
		}
		void _readNetworkState(const spk::Message::Reader &reader) override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			value = reader.get<int>();
			++applications;
		}

	public:
		const std::thread::id owner = std::this_thread::get_id();
		int value = 0, applications = 0;
		void change(int next)
		{
			value = next;
			invalidate();
		}
	};
	class Source : public spk::Network::PublicationSourceTrait
	{
		const std::thread::id _owner = std::this_thread::get_id();
		void _requestObject(ID peer, const spk::Network::Request &request) override
		{
			EXPECT_EQ(std::this_thread::get_id(), _owner);
			PublicationSourceTrait::_requestObject(peer, request);
		}

	public:
		explicit Source(spk::Message::Type type = 42) :
			PublicationSourceTrait(type, {.interval = Clock::duration::zero()})
		{
		}
	};
	class Replicas : public spk::Network::ReplicaCollectionTrait
	{
		spk::Network::ReplicableTrait *_findReplica(ID id) override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			auto found = objects.find(id);
			return found == objects.end() ? nullptr : &found->second;
		}
		spk::Network::ReplicableTrait &_createReplica(ID id) override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			++creations;
			return objects[id];
		}
		void _removeReplica(ID id) override
		{
			EXPECT_EQ(std::this_thread::get_id(), owner);
			objects.erase(id);
			++removals;
		}

	public:
		explicit Replicas(spk::Message::Type type = 42) :
			ReplicaCollectionTrait(type)
		{
		}
		const std::thread::id owner = std::this_thread::get_id();
		std::map<ID, Object> objects;
		int creations = 0, removals = 0;
	};
	ID object = ID::generate();
};
