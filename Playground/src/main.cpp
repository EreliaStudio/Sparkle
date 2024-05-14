#include "sparkle.hpp"

#define MESSAGE_A 0
#define MESSAGE_B 1
#define MESSAGE_C 2
#define MESSAGE_D 3
#define MESSAGE_E 4

class ClientWidget : public spk::Widget
{
private:
    spk::Client _client;

    void _onUpdate() override
    {
        while (_client.messages().empty() == false)
        {
            DEBUG_LINE();
            spk::Client::MessageObject message = _client.messages().pop_front();

            switch (message->type())
            {
                case MESSAGE_A:
                    std::cout << "Client - Receiving message A" << std::endl;
                    _client.send(spk::Message(MESSAGE_B));
                    break;
                case MESSAGE_B:
                    std::cout << "Client - Receiving message B" << std::endl;
                    _client.send(spk::Message(MESSAGE_C));
                    break;
                case MESSAGE_C:
                    std::cout << "Client - Receiving message C" << std::endl;
                    _client.send(spk::Message(MESSAGE_D));
                    break;
                case MESSAGE_D:
                    std::cout << "Client - Receiving message D" << std::endl;
                    _client.send(spk::Message(MESSAGE_E));
                    break;
                case MESSAGE_E:
                    std::cout << "Client - Receiving message E" << std::endl;
                    break;
            }
        }
    }

    void _onGeometryChange() override
    {

    }

    void _onRender() override
    {

    }

public:
    ClientWidget(spk::Widget* p_parent) :
        ClientWidget("Unnamed", p_parent)
    {

    }

    ClientWidget(const std::string& p_name, spk::Widget* p_parent) :
        spk::Widget(p_name, p_parent)
    {
        _client.connect("127.0.0.1", 26500);

        _client.send(spk::Message(MESSAGE_A));
    }
};

class LocalNodeWidget : public spk::Widget
{
private:
    spk::LocalNode _localNode;

    void _onUpdate() override
    {
        while (_localNode.messageReceived().empty() == false)
        {
            DEBUG_LINE();
            spk::Client::MessageObject message = _localNode.messageReceived().pop_front();

            std::cout << "Receiving message from central node : " << message->type() << std::endl;

            switch (message->type())
            {
                case MESSAGE_A:
                    std::cout << "Remote server - Receiving message A - ERROR" << std::endl;
                    break;
                case MESSAGE_B:
                    std::cout << "Remote server - Receiving message B - ERROR" << std::endl;
                    break;
                case MESSAGE_C:
                    std::cout << "Remote server - Receiving message C" << std::endl;
                    _localNode.insertMessageAwnser(_localNode.obtainAwnerMessage(message->header().emitterID, MESSAGE_C));
                    break;
                case MESSAGE_D:
                    std::cout << "Remote server - Receiving message D" << std::endl;
                    _localNode.insertMessageAwnser(_localNode.obtainAwnerMessage(message->header().emitterID, MESSAGE_D));
                    break;
                case MESSAGE_E:
                    std::cout << "Remote server - Receiving message E" << std::endl;
                    _localNode.insertMessageAwnser(_localNode.obtainAwnerMessage(message->header().emitterID, MESSAGE_E));
                    break;
            }
        }
    }

    void _onGeometryChange() override
    {

    }

    void _onRender() override
    {

    }

public:
    LocalNodeWidget(spk::Widget* p_parent) :
        spk::Widget(p_parent)
    {

    }

    spk::Node* node()
    {
        return (&_localNode);
    }
};

class RemoteServerWidget : public spk::Widget
{
private:
    spk::Server _server;

    void _onUpdate() override
    {
        while (_server.messages().empty() == false)
        {
            spk::Client::MessageObject message = _server.messages().pop_front();

            std::cout << "Receiving message from central node : " << message->type() << std::endl;

            switch (message->type())
            {
                case MESSAGE_A:
                    std::cout << "Remote server - Receiving message A" << std::endl;
                    _server.sendTo(message->header().emitterID, spk::Message(MESSAGE_A));
                    break;
                case MESSAGE_B:
                    std::cout << "Remote server - Receiving message B" << std::endl;
                    _server.sendTo(message->header().emitterID, spk::Message(MESSAGE_B));
                    break;
                case MESSAGE_C:
                    std::cout << "Remote server - Receiving message C - ERROR" << std::endl;
                    break;
                case MESSAGE_D:
                    std::cout << "Remote server - Receiving message D - ERROR" << std::endl;
                    break;
                case MESSAGE_E:
                    std::cout << "Remote server - Receiving message E - ERROR" << std::endl;
                    break;
            }
        }
    }

    void _onGeometryChange() override
    {

    }

    void _onRender() override
    {

    }

public:
    RemoteServerWidget(spk::Widget* p_parent) :
        spk::Widget(p_parent)
    {
        _server.start(26501);
    }
};

class CentralNodeWidget : public spk::Widget
{
private:
    spk::CentralNode _centralNode;

    spk::RemoteNode _remoteNode;

    void _onUpdate() override
    {
        _centralNode.redirectMessageToNode();
        _centralNode.redirectMessageToClients();
    }

    void _onGeometryChange() override
    {

    }

    void _onRender() override
    {

    }

public:
    CentralNodeWidget(spk::Node* p_localNode, spk::Widget* p_parent) :
        spk::Widget(p_parent)
    {
        _remoteNode.connect("127.0.0.1", 26501);

        _centralNode.addNode("RemoteNode", &_remoteNode);
        _centralNode.addNode("LocalNode", p_localNode);

        _centralNode.setupRedirection(MESSAGE_A, "RemoteNode");
        _centralNode.setupRedirection(MESSAGE_B, "RemoteNode");

        _centralNode.setupRedirection(MESSAGE_C, "LocalNode");
        _centralNode.setupRedirection(MESSAGE_D, "LocalNode");
        _centralNode.setupRedirection(MESSAGE_E, "LocalNode");

        _centralNode.start(26500);
    }
};

int main()
{
    spk::Application app("Playground", spk::Vector2UInt(800, 600), spk::Application::Mode::Monothread);

    RemoteServerWidget remoteServer(nullptr);
    remoteServer.activate();

    LocalNodeWidget localNode(nullptr);
    localNode.activate();
    
    CentralNodeWidget centralNode(localNode.node(), nullptr);
    centralNode.activate();
    
    ClientWidget clientWidget(nullptr);
    clientWidget.activate();

    return (app.run());
}