#pragma once

#include <tuple>
#include <string>
#include <filesystem>
#include "widget/spk_widget.hpp"
#include "spk_basic_functions.hpp"

/**
 * @brief Macro to open a class registration block for a widget class.
 */
#define openRegistration(ClassType) \
struct ClassRegistrar \
{ \
using Instanciable = ClassType##;\
ClassRegistrar()\
{

/**
 * @brief Macro to subscribe a widget class with a registration name.
 * 
 * @param RegistrationName The name used for registration.
 */
#define subscribeName(RegistrationName) \
spk::WidgetCanvas::classInstanciatorLambda[RegistrationName] =\
[](const spk::JSON::Object &p_obj, spk::Widget* p_parent) -> spk::Widget::Widget* \
{return (p_parent->makeChild(p_obj, p_parent));}

/**
 * @brief Macro to close a class registration block.
 */
#define closeRegistration() \
}\
};\
static inline ClassRegistrar _classRegistrar;

namespace spk
{
	/**
	 * @class WidgetCanvas
	 * @brief A class that represents a canvas for widgets.
	 *
	 * The WidgetCanvas class is used to manage and display a collection of widgets.
	 * It handles widget geometry, loading configuration and content, and calculating
	 * values and formulas for widget layout.
	 */
	class WidgetCanvas : public spk::Widget
	{
	public:
		/**
		 * @brief Typedef for the instantiation function of a widget.
		 */
		using Instanciator = std::function<Widget*(const spk::JSON::Object&, Widget*)>;

		/**
		 * @brief Static map of widget instantiation lambdas.
		 */
		static inline std::map<std::string, Instanciator> classInstanciatorLambda;

	private:
		/**
		 * @struct Geometry
		 * @brief Represents the geometry configuration of a widget.
		 */
		struct Geometry
		{
			/**
			 * @brief Typedef for the input tuple used in geometry formulas.
			 */
			using Input = std::tuple<std::string, std::string>;

			/**
			 * @struct Value
			 * @brief Represents a value used in widget geometry.
			 */
			struct Value
			{
			public:
				/**
				 * @enum Type
				 * @brief Enum for the type of geometry value.
				 */
				enum class Type
				{
					FixedValue, //!< Fixed value type.
					Ratio,      //!< Ratio type.
					Cell,       //!< Cell type.
					Formula     //!< Formula type.
				};

			private:
				// Private members can be added here if necessary.

			public:
				bool active = false;                     //!< Indicates if the value is active.
				Type type = Type::FixedValue;            //!< The type of the value.
				spk::Vector2 value = 0;                  //!< The value.
				Geometry::Input formulaValue = {"", ""}; //!< The formula value.

				/**
				 * @brief Default constructor.
				 */
				Value();

				/**
				 * @brief Constructor to initialize a value from a JSON object.
				 *
				 * @param p_object The JSON object containing the value data.
				 */
				Value(const spk::JSON::Object& p_object);
			};

			Value anchor; //!< The anchor value.
			Value size;   //!< The size value.

			/**
			 * @brief Default constructor.
			 */
			Geometry();

			/**
			 * @brief Constructor to initialize geometry from a JSON object.
			 *
			 * @param p_object The JSON object containing the geometry data.
			 */
			Geometry(const spk::JSON::Object& p_object);
		};

		static inline WidgetCanvas* _activeCanvas = nullptr; //!< Pointer to the active canvas.

		spk::Vector2Int _gridSize; //!< The grid size of the canvas.
		std::map<Widget*, Geometry> _widgetGeometries; //!< Map of widget geometries.

		/**
		 * @brief Calculates the formula value for a widget.
		 *
		 * @param p_target The target widget.
		 * @param p_formula The formula input.
		 * @return The calculated value as Vector2Int.
		 */
		spk::Vector2Int _calcFormula(Widget* p_target, const Geometry::Input& p_formula);

		/**
		 * @brief Calculates the value for a widget.
		 *
		 * @param p_target The target widget.
		 * @param p_value The geometry value.
		 * @return The calculated value as Vector2Int.
		 */
		spk::Vector2Int _calcValue(Widget* p_target, const Geometry::Value& p_value);

		/**
		 * @brief Handles geometry changes.
		 */
		void _onGeometryChange();

		/**
		 * @brief Loads the canvas configuration from a JSON object.
		 *
		 * @param p_object The JSON object containing the configuration data.
		 */
		void _loadConfiguration(const spk::JSON::Object& p_object);

		/**
		 * @brief Loads the content of the canvas from a JSON object.
		 *
		 * @param p_object The JSON object containing the content data.
		 */
		void _loadContent(const spk::JSON::Object& p_object);

	public:
		/**
		 * @brief Constructor to initialize a WidgetCanvas from a file path.
		 *
		 * @param p_canvasFilePath The file path to the canvas file.
		 * @param p_parent Optional parent widget.
		 */
		WidgetCanvas(const std::filesystem::path& p_canvasFilePath, Widget* p_parent = nullptr);

		/**
		 * @brief Constructor to initialize a WidgetCanvas with a name and file path.
		 *
		 * @param p_name The name of the canvas.
		 * @param p_canvasFilePath The file path to the canvas file.
		 * @param p_parent Optional parent widget.
		 */
		WidgetCanvas(const std::string& p_name, const std::filesystem::path& p_canvasFilePath, Widget* p_parent = nullptr);

		/**
		 * @brief Destructor for WidgetCanvas.
		 */
		~WidgetCanvas();
	};
}
