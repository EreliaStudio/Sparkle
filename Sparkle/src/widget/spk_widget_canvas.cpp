#include "widget/spk_widget_canvas.hpp"
#include "miscellaneous/JSON/spk_JSON_file.hpp"

namespace spk
{
	spk::Vector2Int WidgetCanvas::_calcValue(Widget* p_target, const Geometry::Value& p_value)
	{
		spk::Vector2Int result;

		switch (p_value.type)
		{
			case Geometry::Value::Type::FixedValue:
				result = p_value.value;
				break;
			case Geometry::Value::Type::Ratio:
				result = p_value.value * size();
				break;
			case Geometry::Value::Type::Cell:
				result = p_value.value * (size() / _gridSize);
				break;
			case Geometry::Value::Type::Formula:
				result = _calcFormula(p_target, p_value.formulaValue);
				break;
			default:

				break;
		}
		return (result);
	}

	void WidgetCanvas::_onGeometryChange()
	{
		for (auto item : _widgetGeometries)
		{
			Widget* target = item.first;
			Geometry& value = item.second;
			spk::Vector2Int targetAnchor = _calcValue(target, value.anchor);
			spk::Vector2Int targetSize = _calcValue(target, value.size);

			target->setGeometry(targetAnchor, targetSize);
		}
	}

	void WidgetCanvas::_loadConfiguration(const spk::JSON::Object& p_object)
	{
		if (p_object.contains("Name") == false)
			spk::throwException("WidgetCanvas Configuration json object must contain a \"Name\" attribute.");
		rename(p_object["Name"].as<std::string>());

		if (p_object.contains("GridSize") == false)
			spk::throwException("WidgetCanvas Configuration json object must contain a \"GridSize\" attribute.");
		_gridSize = spk::Vector2Int(p_object["GridSize"]);
	}

	WidgetCanvas::Geometry::Value::Value()
	{

	}

	WidgetCanvas::Geometry::Value::Value(const spk::JSON::Object& p_object)
	{
		if (p_object.contains("Type") == false)
			throw std::runtime_error("No member Type found");

		std::string typeString = p_object["Type"].as<std::string>();
		if (typeString == "FixedValue")
		{
			type = Type::FixedValue;
			value = spk::Vector2Int(p_object);
		}
		else if (typeString == "Ratio")
		{
			type = Type::Ratio;
			value = spk::Vector2(p_object);
		}
		else if (typeString == "Cel")
		{
			type = Type::Cell;
			value = spk::Vector2(p_object);
		}
		else if (typeString == "Formula")
		{
			type = Type::Formula;
			formulaValue = {p_object["X"].as<std::string>(), p_object["Y"].as<std::string>()};	
		}
		else
		{
			throw std::runtime_error("Invalid Type value. Accepted value : \"FixedValue\", \"Ratio\", \"Cell\" and \"Formula\"");
		}
	}

	WidgetCanvas::Geometry::Geometry()
	{

	}

	WidgetCanvas::Geometry::Geometry(const spk::JSON::Object& p_object)
	{
		if (p_object.contains("Anchor") == true)
		{
			anchor = WidgetCanvas::Geometry::Value(p_object.operator[]("Anchor"));
		}
		if (p_object.contains("Size") == true)
		{
			size = WidgetCanvas::Geometry::Value(p_object.operator[]("Size"));
		}
	}

	void WidgetCanvas::_loadContent(const spk::JSON::Object& p_object)
	{
		for (size_t i = 0; i < p_object.size(); i++)
		{
			spk::JSON::Object widgetDescriptor = p_object[i];
			std::string classType = widgetDescriptor["WidgetType"].as<std::string>();
			if (spk::WidgetCanvas::classInstanciatorLambda.contains(classType) == false)
				spk::throwException("Class type [" + classType + "] isn't registered");
			spk::WidgetCanvas::Instanciator lambda = spk::WidgetCanvas::classInstanciatorLambda[classType];

			spk::Widget* parent = nullptr;
			if (widgetDescriptor.contains("Parent"))
				parent = findChild(widgetDescriptor["Parent"].as<std::string>());

			spk::Widget* newWidget = lambda(widgetDescriptor, parent);

			_widgetGeometries[newWidget] = Geometry(widgetDescriptor);
		}
	}

	WidgetCanvas::WidgetCanvas(const std::filesystem::path& p_canvasFilePath, Widget* p_parent) :
		WidgetCanvas("Anonymous WidgetCanvas", p_canvasFilePath, p_parent)
	{

	}

	WidgetCanvas::WidgetCanvas(const std::string& p_name, const std::filesystem::path& p_canvasFilePath, Widget* p_parent) : 
		spk::Widget("FixedValueName")
	{
		spk::JSON::File file = spk::JSON::File(p_canvasFilePath);

		if (file.contains("Configuration") == false)
			spk::throwException("WidgetCanvas json must contain a \"Configuration\" attribute.");
		_loadConfiguration(file["Configuration"]);

		if (file.contains("Content") == false)
			spk::throwException("WidgetCanvas json must contain a \"Content\" attribute.");
		_loadContent(file["Content"]);
	}

	WidgetCanvas::~WidgetCanvas()
	{

	}
}