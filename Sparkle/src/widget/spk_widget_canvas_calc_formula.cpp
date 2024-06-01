#include "widget/spk_widget_canvas.hpp"
#include "application/spk_application.hpp"
#include <stack>

namespace spk
{
	 double evaluateExpression(const std::string& expression)
    {
        std::stack<double> values;
        std::stack<wchar_t> ops;

        for (size_t i = 0; i < expression.size(); i++)
        {
            if (expression[i] == L' ')
                continue;
            
            if (isdigit(expression[i]))
            {
                int value = 0;
                while (i < expression.size() && isdigit(expression[i]))
                {
                    value = (value * 10) + (expression[i] - L'0');
                    i++;
                }
                values.push(value);
                i--;
            }
            else if (expression[i] == L'(')
            {
                ops.push(expression[i]);
            }
            else if (expression[i] == L')')
            {
                while (ops.top() != L'(')
                {
                    double val2 = values.top();
                    values.pop();

                    double val1 = values.top();
                    values.pop();

                    wchar_t op = ops.top();
                    ops.pop();

                    switch (op)
                    {
                        case L'+': values.push(val1 + val2); break;
                        case L'-': values.push(val1 - val2); break;
                        case L'*': values.push(val1 * val2); break;
                        case L'/': 
                            if (val2 != 0)
                                values.push(val1 / val2);
                            else
                                throw std::runtime_error("Division by zero!");
                            break;
                        default: throw std::runtime_error("Unknown operator!");
                    }
                }
                ops.pop();
            }
            else if (expression[i] == L'+' || expression[i] == L'-' || expression[i] == L'*' || expression[i] == L'/')
            {
                ops.push(expression[i]);
            }
        }

        while (!ops.empty())
        {
            double val2 = values.top();
            values.pop();

            double val1 = values.top();
            values.pop();

            wchar_t op = ops.top();
            ops.pop();

            switch (op)
            {
                case '+': values.push(val1 + val2); break;
                case '-': values.push(val1 - val2); break;
                case '*': values.push(val1 * val2); break;
                case '/':
                    if (val2 != 0)
                        values.push(val1 / val2);
                    else
                        throw std::runtime_error("Division by zero!");
                    break;
                default: throw std::runtime_error("Unknown operator!");
            }
        }

        return values.top();
    }

	spk::Vector2Int WidgetCanvas::_calcFormula(Widget* p_target, const Geometry::Input& p_formula)
	{
		std::string xFormula = std::get<0>(p_formula);
		std::string yFormula = std::get<1>(p_formula);

		std::map<std::string, int> variables {
			{ "CellX", size().x / _gridSize.x },
			{ "CellY", size().y / _gridSize.y },
			{ "ScreenX", spk::Application::activeApplication()->size().x },
			{ "ScreenY", spk::Application::activeApplication()->size().y },
			{ "ParentX", p_target->parent() ? p_target->parent()->size().x : spk::Application::activeApplication()->size().x },
			{ "ParentY", p_target->parent() ? p_target->parent()->size().y : spk::Application::activeApplication()->size().y },
		};

		for (const auto& [key, value] : variables)
		{
			size_t pos;
			while ((pos = xFormula.find(key)) != std::string::npos)
				xFormula.replace(pos, key.size(), std::to_string(value));

			while ((pos = yFormula.find(key)) != std::string::npos)
				yFormula.replace(pos, key.size(), std::to_string(value));
		}

		int x = static_cast<int>(evaluateExpression(xFormula));
		int y = static_cast<int>(evaluateExpression(yFormula));

		return spk::Vector2Int(x, y);
	}
}