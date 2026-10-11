#include <gtest/gtest.h>

#include "engine/replicated_component.hpp"
#include "engine/replication_interest.hpp"
#include "engine/client_replication.hpp"
#include "engine/server_replication.hpp"
#include "engine/engine.hpp"
#include "core/context/update_context.hpp"
#include "exception.hpp"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace
{
    class NumberComponent : public spk::ReplicatedComponent
    {
    public:
        std::uint32_t value = 0;
        explicit NumberComponent(spk::UUID id) : ReplicatedComponent(id) {}
        void set(std::uint32_t next) { value = next; invalidate(); }
    protected:
        void writeState(spk::ByteStream::Writer &writer) const override { writer << value; }
        void readState(const spk::ByteStream::Reader &reader) override { reader >> value; }
    };

    spk::ByteStream state(std::uint32_t number)
    {
        spk::ByteStream::Writer writer;
        writer << number;
        return std::move(writer).build();
    }
}

TEST(ReplicationV4Test, AppliesNewerRevisionsAndRejectsInvalidSections)
{
    NumberComponent component(spk::UUID::generate());
    component.applyState(2, state(42).reader());
    EXPECT_EQ(component.value, 42u);
    EXPECT_EQ(component.receivedRevision(), 2u);
    component.applyState(1, state(12).reader());
    EXPECT_EQ(component.value, 42u);
    EXPECT_THROW(component.applyState(3, spk::ByteStream().reader()), spk::Exception);
    EXPECT_EQ(component.receivedRevision(), 2u);
    component.applyState(3, state(51).reader());
    EXPECT_EQ(component.value, 51u);
}

TEST(ReplicationV4Test, InterestHandleCancelsAndSupportsUpdates)
{
    int cancellations = 0;
    int updates = 0;
    spk::ReplicationInterest handle([&] { ++cancellations; },
        [&](std::uint32_t, const spk::ByteStream &) { ++updates; });
    handle.update(10, {});
    EXPECT_EQ(updates, 1);
    handle.cancel();
    handle.cancel();
    EXPECT_EQ(cancellations, 1);
    EXPECT_THROW(handle.update(10, {}), spk::Exception);
}

TEST(ReplicationV4Test, SocketReplicationFollowsInterestAndRevision)
{
    spk::Server server;
    spk::Client client;
    spk::Engine serverEngine, clientEngine;
    spk::Entity authoritative("Authoritative"), replica("Replica");
    const auto id = spk::UUID::generate();
    auto &source = authoritative.addComponent<NumberComponent>(id);
    auto &target = replica.addComponent<NumberComponent>(id);
    serverEngine.addEntity(&authoritative);
    clientEngine.addEntity(&replica);
    serverEngine.addSystem<spk::ServerReplication>(server,
        [](const spk::ReplicatedComponent &, std::uint32_t type, const spk::ByteStream::Reader &) { return type == 7; },
        [](spk::ConnectionID, const spk::ReplicatedComponent &) { return true; }, 0ms);
    auto &receiver = clientEngine.addSystem<spk::ClientReplication>(client);
    server.start(0);
    client.connect("127.0.0.1", server.port());
    auto interest = receiver.subscribe(7, {});
    source.set(123);
    spk::UpdateContext context{};
    for (int i = 0; i < 200 && target.value != 123; ++i) {
        serverEngine.updateState(context);
        clientEngine.updateState(context);
        std::this_thread::sleep_for(5ms);
    }
    EXPECT_EQ(target.value, 123u);
    EXPECT_EQ(target.receivedRevision(), source.version() + 1);
    interest.cancel();
    client.disconnect();
    server.stop();
}
