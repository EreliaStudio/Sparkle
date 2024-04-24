#include "sparkle.hpp"

class ServerWidget : public spk::Widget
{
private:
    spk::Server _server;

    void _onUpdate() override
    {
        auto& messages = _server.messages();
        while (!messages.empty())
        {
            auto msg = messages.pop_front();
            if (msg->header().type == 0)
            {
                std::cout << "Received request from client." << std::endl;

                spk::Message response;
                response.header().type = 1;
                std::string responseData = "Hello from Server!";
                response << responseData;
                _server.sendToAll(response);
            }
        }
    }

public:
    ServerWidget(spk::Widget* p_parent) :
        spk::Widget(p_parent),
        _server()
    {
        _server.start(26500);
    }
};

class ClientWidget : public spk::Widget
{
private:
    spk::Client _client;

void _onUpdate() override
{
    static std::chrono::steady_clock::time_point lastSendTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastSendTime).count() >= 5)
    {
        spk::Message msg;
        msg.header().type = 0;
        _client.send(msg);
        lastSendTime = currentTime;
        std::cout << "Request sent to server." << std::endl;
    }

    while (!_client.messages().empty())
    {
        auto msg = _client.messages().pop_front();
        if (msg->header().type == 1)
        {
            std::string response = msg->get<std::string>();
            std::cout << "Response from server: " << response << std::endl;
        }
    }
}

public:
    ClientWidget(spk::Widget* p_parent) :
        spk::Widget(p_parent),
        _client()
    {
        _client.connect("127.0.0.1", 26500);
    }
};

int main()
{
    spk::Application app("Playground", spk::Vector2UInt(800, 600), spk::Application::Mode::Monothread);

    ServerWidget serverWidget = ServerWidget(nullptr);
    serverWidget.activate();

    ClientWidget clientWidget = ClientWidget(nullptr);
    clientWidget.activate();

    return (app.run());
}