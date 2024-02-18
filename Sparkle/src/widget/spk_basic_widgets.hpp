#pragma once

#include "math/spk_vector2.hpp"
#include "widget/spk_box_constraints.hpp"
#include "widget/spk_widget.hpp"
#include <functional>

namespace spk::widget
{

    /**
     * @brief LayoutBuilder allows defining _onLayout through a Builder without writing a new class.
     */
    class LayoutBuilder : public IWidget
    {
    public:
        using Builder = std::function<Vector2(const BoxConstraints&)>;
        LayoutBuilder(const Builder& p_builder, IWidget* p_parent) :
            IWidget("LayoutBuilder", p_parent),
            _builder(p_builder)
        {
        }

    private:
        Vector2 _onLayout(const BoxConstraints& p_constraints)
        {
            return _builder(p_constraints);
        }

        Builder _builder;
    };

    /**
     * @brief SizedBox forces its child to a fixed size.
     */
    class SizedBox : public SingleChildWidget
    {
    public:
        SizedBox(const Vector2& p_size, IWidget* p_parent) :
            SingleChildWidget("SizedBox", p_parent),
            _size(p_size)
        {
        }

        const Vector2& size() const;

    private:
        Vector2 _onLayout(const BoxConstraints& p_constraints) override;

        Vector2 _size;
    };

    /**
     * @brief FractionallySizedBox sizes its widgets to a fraction of its parent's constraints.
     */
    class FractionallySizedBox : public SingleChildWidget
    {
    public:
        FractionallySizedBox(float p_horizontal, float p_vertical, IWidget* p_parent) :
            SingleChildWidget("FractionallySizedBox", p_parent),
            _horizontal(p_horizontal),
            _vertical(p_vertical)
        {
        }

        Vector2 _onLayout(const BoxConstraints& p_constraints) override;

    private:
        float _horizontal;
        float _vertical;
    };

    /**
     * @brief Padding adds padding around its child.
     */
    class Padding : public SingleChildWidget
    {
    public:
        struct Config
        {
            float left;
            float right;
            float top;
            float bottom;
        };

        Padding(const Config& p_config, IWidget* p_parent) :
            SingleChildWidget("Padding", p_parent),
            _config(p_config)
        {
        }

        static Padding symmetric(float p_horizontal, float p_vertical, IWidget* p_parent) { return Padding({p_horizontal, p_horizontal, p_vertical, p_vertical}, p_parent); }
        static Padding all(float p_value, IWidget* p_parent) { return Padding({p_value, p_value, p_value, p_value}, p_parent); }

        Vector2 _onLayout(const BoxConstraints& p_constraints) override;

    private:
        Config _config;
    };

    /**
     * @brief Expanded allows flexible sizing of its child.
     *
     * It must be used with a Column or a Row.
     * The child will share the available space with other Expanded widgets.
     */
    class Expanded : public SingleChildWidget
    {
    public:
        /**
         * @brief Constructor.
         * @param p_flex Set this value to give different portions of the available space to each Expanded widget. Default to 1.0f
         */
        Expanded(float p_flex, IWidget* p_parent) :
            SingleChildWidget("Expanded", p_parent),
            _flex(p_flex)
        {
        }
        Expanded(IWidget* p_parent) :
            Expanded(1.0f, p_parent)
        {
        }

        float flex() const { return _flex; }

        Vector2 _onLayout(const BoxConstraints& p_constraints) override;

    private:
        float _flex;
    };

    enum class MainAxisAlignment
    {
        start,
        center,
        end
    };

    enum class CrossAxisAlignment
    {
        start,
        center,
        end
    };

    class Column : public IWidget
    {
    public:
        struct Config
        {
            MainAxisAlignment mainAxisAlignment;
            CrossAxisAlignment crossAxisAlignment;

            Config() :
                mainAxisAlignment(MainAxisAlignment::start),
                crossAxisAlignment(CrossAxisAlignment::start)
            {
            }
        };

        Column(const Config& p_config, IWidget* p_parent) :
            IWidget("Column", p_parent),
            _config(p_config)
        {
        }

        Column(IWidget* p_parent) :
            Column(Config(), p_parent)
        {
        }

        Vector2 _onLayout(const BoxConstraints& p_constraints) override;

    private:
        Config _config;
    };

    class Row : public IWidget
    {
    public:
        struct Config
        {
            MainAxisAlignment mainAxisAlignment;
            CrossAxisAlignment crossAxisAlignment;

            Config() :
                mainAxisAlignment(MainAxisAlignment::start),
                crossAxisAlignment(CrossAxisAlignment::start)
            {
            }
        };

        Row(const Config& p_config, IWidget* p_parent) :
            IWidget("Row", p_parent),
            _config(p_config)
        {
        }

        Row(IWidget* p_parent) :
            Row(Config(), p_parent)
        {
        }

        Vector2 _onLayout(const BoxConstraints& p_constraints) override;

    private:
        Config _config;
    };

    class Center : public SingleChildWidget
    {
    public:
        Center(IWidget* p_parent) :
            SingleChildWidget("Center", p_parent)
        {
        }

        Vector2 _onLayout(const BoxConstraints& p_constraints);
    };
}
