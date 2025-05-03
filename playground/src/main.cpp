#include <sparkle.hpp>

class Client
{
private:
	static inline std::string address;
	static inline const size_t port = 26500;
	static inline spk::Client client;

public:
	static spk::Client::Contract addOnConnectionCallback(const spk::Client::ConnectionCallback &p_connectionCallback)
	{
		return std::move(client.addOnConnectionCallback(p_connectionCallback));
	}
	static spk::Client::Contract addOnDisconnectionCallback(const spk::Client::DisconnectionCallback &p_disconnectionCallback)
	{
		return std::move(client.addOnDisconnectionCallback(p_disconnectionCallback));
	}

	static bool isConnected()
	{
		return (client.isConnected());
	}
	static void setup(const std::string &p_address)
	{
		address = p_address;
	}
	static void connect()
	{
		client.connect(address, port);
	}
	static void disconnect()
	{
		client.disconnect();
	}
	static void send(const spk::Message &p_message)
	{
		client.send(p_message);
	}
	static spk::ThreadSafeQueue<spk::Client::MessageObject> &messages()
	{
		return (client.messages());
	}
};

class Server
{
private:
	static inline const size_t port = 26500;
	static inline spk::Server server;

public:
	static void start()
	{
		server.start(port);
	}

	static void stop()
	{
		server.stop();
	}

	static spk::Server::Contract addOnConnectionCallback(const spk::Server::ConnectionCallback &p_connectionCallback)
	{
		return std::move(server.addOnConnectionCallback(p_connectionCallback));
	}
	
	static spk::Server::Contract addOnDisconnectionCallback(const spk::Server::DisconnectionCallback &p_disconnectionCallback)
	{
		return std::move(server.addOnDisconnectionCallback(p_disconnectionCallback));
	}

	static void sendTo(spk::Server::ClientID p_clientID, const spk::Message& p_msg)
	{
		server.sendTo(p_clientID, p_msg);
	}

	static void sendTo(const std::vector<spk::Server::ClientID> &p_clients, const spk::Message &p_message)
	{
		server.sendTo(p_clients, p_message);
	}

	static void sendToAll(const spk::Message& p_msg)
	{
		server.sendToAll(p_msg);
	}

	static spk::ThreadSafeQueue<spk::Server::MessageObject> &messages()
	{
		return server.messages();
	}
};

enum class Event
{
	EnterMenuHUD,
	EnterGameHUD
};

class EventDispatcher
{
public:
	using Observer = spk::Observer<Event>;
	using Contract = Observer::Contract;
	using Job = Observer::Job;

private:
	static inline Observer observer;

public:
	static void emit(Event p_event)
	{
		observer.notifyEvent(p_event);
	}

	static void invalidateContracts(Event p_event)
	{
		observer.invalidateContracts(p_event);
	}

	static Observer::Contract subscribe(Event p_event, const Job &p_job)
	{
		return (std::move(observer.subscribe(p_event, p_job)));
	}
};

class AssetAtlas
{
private:
	static inline std::unordered_map<std::wstring, spk::SafePointer<spk::Image>> images;
	static inline std::unordered_map<std::wstring, spk::SafePointer<spk::SpriteSheet>> spriteSheets;
	static inline std::unordered_map<std::wstring, spk::SafePointer<spk::Font>> fonts;

public:
	static void load(const spk::JSON::File& p_atlasFileConfiguration)
	{
		
	}

	static spk::SafePointer<spk::Image> image(const std::wstring& p_name)
	{
		if (images.contains(p_name) == false)
		{
			GENERATE_ERROR("Image not found: " + spk::StringUtils::wstringToString(p_name));
		}
		return images[p_name];
	}

	static spk::SafePointer<spk::SpriteSheet> spriteSheet(const std::wstring& p_name)
	{
		if (spriteSheets.contains(p_name) == false)
		{
			GENERATE_ERROR("SpriteSheet not found: " + spk::StringUtils::wstringToString(p_name));
		}
		return spriteSheets[p_name];
	} 

	static spk::SafePointer<spk::Font> font(const std::wstring& p_name)
	{
		if (fonts.contains(p_name) == false)
		{
			GENERATE_ERROR("Font not found: " + spk::StringUtils::wstringToString(p_name));
		}
		return fonts[p_name];
	}
};

class PushButton : public spk::PushButton
{
private:

public:
	PushButton(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::PushButton(p_name, p_parent)
	{
		
	}
};

class TextLabel : public spk::TextLabel
{
private:

public:
	TextLabel(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::TextLabel(p_name, p_parent)
	{
		
	}
};

class TextEdit : public spk::TextEdit
{
private:

public:
	TextEdit(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::TextEdit(p_name, p_parent)
	{
		
	}
};

class GameHUD : public spk::Screen
{
private:

public:
	GameHUD(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Screen(p_name, p_parent)
	{
		
	}
};

class MenuHUD : public spk::Screen
{
private:
	TextEdit _addressTextEdit;
	PushButton _joinGameButton;
	PushButton _hostGameButton;
	PushButton _quitGameButton;

	void _onGeometryChange() override
	{
		spk::Vector2Int buttonAreaSize = spk::Vector2Int::min(geometry().size / 2, {400, 200});
		spk::Vector2Int buttonAreaOffset = (geometry().size - buttonAreaSize) / 2;

		float padding = 10;
		spk::Vector2Int lineSize = {buttonAreaSize.x, (buttonAreaSize.y - padding * 2) / 3};
		spk::Vector2Int editSize = {buttonAreaSize.x * 0.6f, lineSize.y};
		spk::Vector2Int joinGameButtonSize = {buttonAreaSize.x * 0.4f - padding, lineSize.y};

		_addressTextEdit.setGeometry(buttonAreaOffset, editSize);
		_joinGameButton.setGeometry({buttonAreaOffset.x + editSize.x + padding, buttonAreaOffset.y}, joinGameButtonSize);

		_hostGameButton.setGeometry({buttonAreaOffset.x, buttonAreaOffset.y + (lineSize.y + padding) * 1}, lineSize);
		_quitGameButton.setGeometry({buttonAreaOffset.x, buttonAreaOffset.y + (lineSize.y + padding) * 2}, lineSize);
	}

public:
	MenuHUD(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Screen(p_name, p_parent),
		_addressTextEdit(p_name + L"/_AddressTextEdit", this),
		_joinGameButton(p_name + L"/JoinGameButton", this),
		_hostGameButton(p_name + L"/HostGameButton", this),
		_quitGameButton(p_name + L"/QuitGameButton", this)
	{
		_addressTextEdit.activate();
		_addressTextEdit.setPlaceholder(L"Enter server address");

		_joinGameButton.setText(L"Join Game");
		_joinGameButton.activate();

		_hostGameButton.setText(L"Host Game");
		_hostGameButton.activate();

		_quitGameButton.setText(L"Quit");
		_quitGameButton.activate();
	}
};

class MainWidget : public spk::Widget
{
private:
	GameHUD _gameHUD;
	EventDispatcher::Contract _gameHUDContract;
	MenuHUD _menuHUD;
	EventDispatcher::Contract _menuHUDContract;

	void _onGeometryChange() override
	{
		spk::cout << std::endl << std::endl << std::endl;
		_gameHUD.setGeometry(geometry());
		_menuHUD.setGeometry(geometry());
	}

public:
	MainWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_gameHUD(p_name + L"/GameHUD", this),
		_menuHUD(p_name + L"/MenuHUD", this)
	{
		_gameHUDContract = EventDispatcher::subscribe(Event::EnterGameHUD, [&]() {
			_gameHUD.activate();
		});

		_menuHUDContract = EventDispatcher::subscribe(Event::EnterMenuHUD, [&]() {
			_menuHUD.activate();
		});
	}
};

int main()
{
	spk::GraphicalApplication app;

	spk::SafePointer<spk::Window> win = app.createWindow(L"TAAG", {{0, 0}, {800, 600}});

	MainWidget mainWidget(L"MainWidget", win->widget());
	mainWidget.setGeometry(win->geometry());
	mainWidget.activate();

	EventDispatcher::emit(Event::EnterMenuHUD);

	return (app.run());
}