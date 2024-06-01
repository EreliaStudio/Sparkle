#pragma once

#include <tuple>
#include <string>
#include <filesystem>
#include "widget/spk_widget.hpp"
#include "spk_basic_functions.hpp"

#define openRegistration(ClassType) \
struct ClassRegistrar \
{ \
using Instanciable = ClassType##;\
ClassRegistrar()\
{

#define subscribeName(RegistrationName) \
spk::WidgetCanvas::classInstanciatorLambda[RegistrationName] =\
[](const spk::JSON::Object &p_obj, spk::Widget* p_parent) -> spk::Widget::Widget* \
{return (p_parent->makeChild(p_obj, p_parent));}

#define closeRegistration() \
}\
};\
static inline ClassRegistrar _classRegistrar;

namespace spk
{
	class WidgetCanvas : public spk::Widget
	{
	public:
		using Instanciator = std::function<Widget*(const spk::JSON::Object&, Widget*)>;
		static inline std::map<std::string, Instanciator> classInstanciatorLambda;

	private:
		struct Geometry
		{
			using Input = std::tuple<std::string, std::string>;

			struct Value
			{
			public:
				enum class Type
				{
					FixedValue,
					Ratio,
					Cell,
					Formula
				};

			private:

			public:
				bool active = false;
				Type type = Type::FixedValue;
				spk::Vector2 value = 0;
				Geometry::Input formulaValue = {"", ""};

				Value();
				Value(const spk::JSON::Object& p_object);
			};

			Value anchor;
			Value size;

			Geometry();
			Geometry(const spk::JSON::Object& p_object);
		};

		static inline WidgetCanvas* _activeCanvas = nullptr;

		spk::Vector2Int _gridSize;
		std::map<Widget*, Geometry> _widgetGeometries;

		spk::Vector2Int _calcFormula(Widget* p_target, const Geometry::Input& p_formula);

		spk::Vector2Int _calcValue(Widget* p_target, const Geometry::Value& p_value);

		void _onGeometryChange();

		void _loadConfiguration(const spk::JSON::Object& p_object);

		void _loadContent(const spk::JSON::Object& p_object);

	public:
		WidgetCanvas(const std::filesystem::path& p_canvasFilePath, Widget* p_parent = nullptr);
		WidgetCanvas(const std::string& p_name, const std::filesystem::path& p_canvasFilePath, Widget* p_parent = nullptr);

		~WidgetCanvas();
	};
}