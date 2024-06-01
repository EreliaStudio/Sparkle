#include "sparkle.hpp"

class WidgetA : public spk::Widget
{
	openRegistration(WidgetA);
	subscribeName("MyWidget");
	subscribeName("Blablabla");
	closeRegistration();

private:
	void _onUpdate() override
	{
		
	}

	void _onGeometryChange() override
	{

	}

	void _onRender() override
	{

	}

public:
	WidgetA(spk::Widget* p_parent) :
		WidgetA("Unnamed WidgetA", p_parent)
	{

	}

	WidgetA(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent)
	{
		
	}

	WidgetA(const spk::JSON::Object &p_obj, spk::Widget* p_parent) :
		spk::Widget(p_obj, p_parent)
	{
		
	}
};

class WidgetB : public spk::Widget
{
	openRegistration(WidgetB);
	subscribeName("MyWidgetB");
	subscribeName("Other Blablabla");
	closeRegistration();

private:
	void _onUpdate() override
	{
		
	}

	void _onGeometryChange() override
	{

	}

	void _onRender() override
	{

	}

public:
	WidgetB(spk::Widget* p_parent) :
		WidgetB("Unnamed WidgetB", p_parent)
	{

	}

	WidgetB(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent)
	{
		
	}

	WidgetB(const spk::JSON::Object &p_obj, spk::Widget* p_parent) :
		spk::Widget(p_obj, p_parent)
	{
		
	}
};

int main()
{
	std::cout << "Nb subscribed widget : " << spk::WidgetCanvas::classInstanciatorLambda.size() << std::endl;
	for (const auto& [key, lambda] : spk::WidgetCanvas::classInstanciatorLambda)
	{
		std::cout << "Subscriber : " << key << std::endl;
	}
	return (0);
}
