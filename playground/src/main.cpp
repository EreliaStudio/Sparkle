#include <sparkle.hpp>

enum class MessageType
{
	Register = 0,

    Connection = 100,
    Disconnection = 101,
    PublicNameEdition = 102,
	IconEdition = 103,

    FriendRequest = 200,
    FriendRequestResponse = 201,

    PrivateMessage = 300,

	SystemMessage = 400,
    Error = 401
};

class ConnectionWidget : public spk::Widget
{
private:
	spk::Frame _backgroundFrame;

	spk::TextLabel _usernameLabel;
	spk::TextEdit _usernameEdit;
	spk::TextLabel _passwordLabel;
	spk::TextEdit _passwordEdit;

	spk::PushButton _connectButton;
	spk::PushButton::Contract _connectButtonContract;
	spk::PushButton _registerButton;
	spk::PushButton::Contract _registerButtonContract;

	void _onGeometryChange() override
	{
		const int margin              = _backgroundFrame.cornerSize().x;
		constexpr int interLineSpace  = 15;
		constexpr int btnHPadding     = 20;
		constexpr int btnVPadding     = 8;
		constexpr int buttonGap       = 10;

		const spk::Geometry2D widgetRect = geometry();
		_backgroundFrame.setGeometry(0, widgetRect.size);

		const spk::Vector2Int areaPos  = spk::Vector2Int(margin, margin);
		const spk::Vector2Int areaSize = widgetRect.size - spk::Vector2Int(margin * 2, margin * 2);

		const int  lineHeight = (areaSize.y - 2 * interLineSpace) / 3;
		const spk::Vector2Int lineSize(areaSize.x, lineHeight);

		const spk::Vector2Int row0Pos = areaPos;

		const int labelWidth = static_cast<int>(lineSize.x * 0.30f);
		const int vInset     = static_cast<int>(lineHeight * 0.10f);

		_usernameLabel.setGeometry(row0Pos - spk::Vector2Int(0, 5), {labelWidth, lineHeight});

		_usernameEdit.setGeometry(
			{row0Pos.x, row0Pos.y + vInset},
			{lineSize.x, lineHeight - 2 * vInset});

		const spk::Vector2Int row1Pos = areaPos + spk::Vector2Int(0, lineHeight + interLineSpace);

		_passwordLabel.setGeometry(row1Pos - spk::Vector2Int(0, 5), {labelWidth, lineHeight});

		_passwordEdit.setGeometry(
			{row1Pos.x, row1Pos.y + vInset},
			{lineSize.x, lineHeight - 2 * vInset});

		const spk::Vector2Int row2Pos = areaPos + spk::Vector2Int(0, 2 * (lineHeight + interLineSpace));

		const spk::Vector2UInt buttonTextSize = spk::Vector2Int::max(
			_connectButton.computeTextSize(),
			_registerButton.computeTextSize());

		int buttonWidth  = static_cast<int>(buttonTextSize.x) + btnHPadding * 2;
		int buttonHeight = std::max<int>(buttonTextSize.y, buttonTextSize.y) + btnVPadding * 2;
		buttonHeight     = std::max(buttonHeight, lineHeight);

		const int rightEdgeX = row2Pos.x + lineSize.x;

		_registerButton.setGeometry(
			{rightEdgeX - buttonWidth, row2Pos.y + (lineHeight - buttonHeight) / 2},
			{buttonWidth, buttonHeight});

		_connectButton.setGeometry(
			{rightEdgeX - (buttonWidth + buttonGap + buttonWidth), row2Pos.y + (lineHeight - buttonHeight) / 2},
			{buttonWidth, buttonHeight});
	}

public:
	ConnectionWidget(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_backgroundFrame(p_name + L"/BackgroundFrame", this),
		_usernameLabel(p_name + L"/UsernameLabel", this),
		_usernameEdit(p_name + L"/UsernameEdit", this),
		_passwordLabel(p_name + L"/PasswordLabel", this),
		_passwordEdit(p_name + L"/PasswordEdit", this),
		_connectButton(p_name + L"/ConnectButton", this),
		_registerButton(p_name + L"/RegisterButton", this)
	{
		_backgroundFrame.setLayer(0);
		_backgroundFrame.setCornerSize(32);
		_backgroundFrame.activate();

		_usernameLabel.setLayer(3);
		_usernameLabel.setCornerSize(0);
		_usernameLabel.setNineSlice(nullptr);
		_usernameLabel.setText(L"Username :");
		_usernameLabel.setFontSize(spk::Font::Size(22, 2));
		_usernameLabel.setTextAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Top);
		_usernameLabel.activate();

		_usernameEdit.setLayer(1);
		_usernameEdit.setPlaceholder(L"Enter your username");
		_usernameEdit.setText(L"Hyarius");
		_usernameEdit.activate();

		_passwordLabel.setLayer(3);
		_passwordLabel.setCornerSize(0);
		_passwordLabel.setNineSlice(nullptr);
		_passwordLabel.setText(L"Password :");
		_passwordLabel.setFontSize(spk::Font::Size(22, 2));
		_passwordLabel.setTextAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Top);
		_passwordLabel.activate();

		_passwordEdit.setLayer(1);
		_passwordEdit.setPlaceholder(L"Enter your password");
		_passwordEdit.setText(L"61M=j4*Et");
		_passwordEdit.setObscured(true);
		_passwordEdit.activate();

		_connectButton.setLayer(1);
		_connectButton.setText(L"Connect");
		_connectButton.activate();

		_registerButton.setLayer(1);
		_registerButton.setText(L"Register");
		_registerButton.activate();
	}

	const std::wstring& username() const
	{
		return _usernameEdit.text();
	}

	const std::wstring& password() const
	{
		return _passwordEdit.text();
	}

	void setRegisterAction(const spk::PushButton::Job& p_job)
	{
		_registerButtonContract = _registerButton.subscribe(p_job);
	}

	void setConnectAction(const spk::PushButton::Job& p_job)
	{
		_connectButtonContract = _connectButton.subscribe(p_job);
	}
};

class MainWidget : public spk::Widget
{
private:
	spk::Client _client;

	ConnectionWidget _connectionWidget;
	spk::TextLabel _errorMessageLabel;

	void _onGeometryChange() override
	{
		spk::Vector2Int connectionWidgetSize = spk::Vector2Int::min(geometry().size / 2, {400, 300});
		spk::Vector2Int errorMessageLabelSize = spk::Vector2Int(connectionWidgetSize.x, std::min(connectionWidgetSize.y / 4, 50));

		spk::Vector2Int connectionWidgetPos = (geometry().size - connectionWidgetSize) / 2;

		_connectionWidget.setGeometry(connectionWidgetPos, connectionWidgetSize);
		_errorMessageLabel.setGeometry(connectionWidgetPos - spk::Vector2Int(0, errorMessageLabelSize.y), errorMessageLabelSize);
	}

	void _receivedConnectionMessage(spk::Client::MessageObject& p_message)
	{
		spk::cout << "Received connection message" << std::endl;
	}

	void _receivedErrorMessage(spk::Client::MessageObject& p_message)
	{
		spk::cout << "Received error message" << std::endl;
		std::wstring errorMessage = p_message->get<std::wstring>();

		spk::cout << "Error message : " << errorMessage << std::endl;
	}

	void _receivedSystemMessage(spk::Client::MessageObject& p_message)
	{
		spk::cout << "Received system message" << std::endl;
		std::wstring systemeMessage = p_message->get<std::wstring>();

		spk::cout << "System message : " << systemeMessage << std::endl;
	}

	void _onUpdateEvent(spk::UpdateEvent& p_event)
	{
		while (_client.messages().empty() == false)
		{
			spk::Client::MessageObject message = _client.messages().pop();

			MessageType messageType = static_cast<MessageType>(message->header().type);

			switch (messageType)
			{
				case MessageType::Connection:
				{
					_receivedConnectionMessage(message);
					break;
				}
				case MessageType::Error:
				{
					_receivedErrorMessage(message);
					break;
				}
				case MessageType::SystemMessage:
				{
					_receivedSystemMessage(message);
					break;
				}
				default:
					spk::cout << "Unknown message type : " << message->header().type << std::endl;
					break;
			}
			
		}
	}

	void _initializeClient(const spk::JSON::File &p_configurationFile)
	{
		std::wstring address = p_configurationFile[L"Address"].as<std::wstring>();
		long port = p_configurationFile[L"Port"].as<long>();

		try
		{
			_client.connect(spk::StringUtils::wstringToString(address), port);
		}
		catch(const std::runtime_error& e)
		{
			std::cerr << e.what() << '\n';
			return ;
		}

		spk::cout << "Client connected " << address << ":" << port << std::endl;
		spk::cout << "Client connected " << std::boolalpha << _client.isConnected() << std::endl;
	}

	void _onConnectRequest()
	{
		spk::Message connectionRequest(static_cast<int>(MessageType::Connection));

		connectionRequest << _connectionWidget.username();
		connectionRequest << _connectionWidget.password();

		spk::cout << "Send connection request to " << _connectionWidget.username() << " with password " << _connectionWidget.password() << std::endl;

		_client.send(connectionRequest);
	}

	void _onRegisterRequest()
	{
		spk::Message registrationRequest(static_cast<int>(MessageType::Register));

		registrationRequest << _connectionWidget.username();
		registrationRequest << _connectionWidget.password();

		spk::cout << "Send register request to " << _connectionWidget.username() << " with password " << _connectionWidget.password() << std::endl;

		_client.send(registrationRequest);
	}

public:
	MainWidget(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_connectionWidget(p_name + L"/ConnectionWidget", this),
		_errorMessageLabel(p_name + L"/ErrorMessageLabel", this)
	{
		spk::JSON::File configurationFile = spk::JSON::File(L"playground/resources/config.json");
		_initializeClient(configurationFile);

		_connectionWidget.setRegisterAction([&]() {
				if (_client.isConnected() == true)
				{
					_onRegisterRequest();
				}
				else
				{
					spk::cout << "Client not connected" << std::endl;
				}
			});

		_connectionWidget.setConnectAction([&]() {
				if (_client.isConnected() == true)
				{
					_onConnectRequest();
				}
				else
				{
					spk::cout << "Client not connected" << std::endl;
				}
			});
			
		_connectionWidget.activate();

		_errorMessageLabel.setNineSlice(nullptr);
		_errorMessageLabel.setText(L"Default text");
		_errorMessageLabel.activate();
	}
};

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"TAAG Client", {{0, 0}, {800, 800}});

	MainWidget mainWidget(L"MainWidget", win->widget());
	mainWidget.setGeometry(win->geometry());
	mainWidget.activate();

	DEBUG_LINE();
	return (app.run());
}