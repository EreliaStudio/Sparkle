#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <GL/glew.h>
#include <GL/wglew.h>
#include <Windows.h>
#include <atomic>
#include <deque>
#include <functional>
#include <gl/GL.h>
#include <mutex>
#include <string>

#include "math/spk_matrix4x4.hpp"

#include "data_structure/spk_thread_safe_queue.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"
#include "input/spk_input_decoder.hpp"
#include "input/spk_keyboard.hpp"
#include "input/spk_mouse.hpp"
#include "profiler/spk_profiler.hpp"
#include "profiler/spk_trigger_metric.hpp"
#include "system/spk_time_manager.hpp"
#include "system/spk_timer.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
    /**
     * @class Application
     * @brief Manages a graphical application lifecycle and central functionalities such as time, mouse or keyboard handling.
     *
     * The Application class serves as the core of the Sparkle library, orchestrating the application lifecycle,
     * managing the main event loop, and facilitating graphics rendering and event handling. It supports both
     * monothreaded and multithreaded modes of operation for flexibility in application development.
     *
     * Features include:
     * - Window creation and management using OpenGL for rendering.
     * - Event handling for keyboard and mouse input.
     * - Time metrics collection.
     * - Profiling for performance analysis.
     *
     * The Application class is designed to be the starting point for applications using the Sparkle library,
     * allowing developers to easily create, manage, and render graphics content while handling user input
     * and other application events via the Sparkle IWidget system.
     *
     * Usage example:
     * @code
     * spk::Application app("Example App", spk::Vector2UInt(800, 600), spk::Application::Mode::Monothread);
     *
     * MyWidget myWidget = MyWidget("MyCustomWidget"); // This widget will automaticaly be included inside app, and will be rendered and updated by it once activated.
     * myWidget.setGeometry(spk::Vector2Int(0, 0), app.size()); // Set the size and anchor of the widget to hold the whole application size.
     * myWidget.activate(); // Activate the widget, to allow application to render and update it
     *
     * return (app.run());
     * @endcode
     *
     * @see IWidget, Pipeline, Keyboard, Mouse, TimeManager, Profiler
     */
    class Application
    {
    private:
        static thread_local inline Application* _activeApplication = nullptr;

    public:
        /**
         * @brief Return a pointer to the currently active application
         * @note This method is static, and is not thread local
         */
        static Application* activeApplication()
        {
            return (_activeApplication);
        }

        /**
         * @brief Set this Application as the "active" application, allowing activeApplication() to return a pointer to this.
         */
        void setAsActiveApplication()
        {
            _activeApplication = this;
        }

        /**
         * @enum Mode
         * @brief Define the expected mode of threading for an application
         * Will be passed thought the Application constructor.
         */
        enum class Mode
        {
            Monothread, //< The application will run on a single thread
            Multithread //< The application will run on multiples threads (At least 2 threads)
        };

        /**
         * @class CentralWidget
         * @brief CentralWidget serves as the core widget in an application, managing time-based updates via a custom rendering pipeline.
         *
         * Usage:
         * CentralWidget is designed to be instantiated once and used as the root of an application's widget hierarchy. It
         * automatically becomes the default parent for other widgets, ensuring that all UI components can benefit from its
         * rendering capabilities and time-based updates.
         *
         * @note This widget is instancied automaticaly by the spk::Application, and isn't instanciable by any other object.
         */
        class CentralWidget : public widget::IWidget
        {
            friend class Application;

            static inline std::string _prerenderingPipelineCode = R"(#version 450
			
			#include <timeConstants>
			#include <screenConstants>
			
			void geometryPass() {}
			void renderPass() {})";
            static inline spk::Pipeline _prerenderingPipeline = spk::Pipeline(_prerenderingPipelineCode);
            spk::Pipeline::Constant& _timeConstant;
            spk::Pipeline::Constant::Element& _timeConstantEpochElement;
            long long oldTime = 0;

        private:
            void _onGeometryChange()
            {
            }

            void _onRender()
            {
            }

            void _onUpdate()
            {
                long long time = spk::getTime();
                if (time != oldTime)
                {
                    oldTime = time;
                    _timeConstantEpochElement = static_cast<int>(time % 100000);
                    _timeConstant.update();
                }
            }

            CentralWidget() :
                IWidget("CentralWidget"),
                _timeConstant(_prerenderingPipeline.constant("timeConstants")),
                _timeConstantEpochElement(_timeConstant["epoch"])
            {
                defaultParent = this;
            }

        public:
            /**
             * @brief Will set the default parent widget parent to nullptr
             */
            ~CentralWidget()
            {
                defaultParent = nullptr;
            }
        };

    private:
        static LRESULT CALLBACK WindowProc(HWND p_hwnd, UINT p_messageID, WPARAM p_firstParam, LPARAM p_secondParam);

        class Handle
        {
        private:
            HWND _windowHandle;
            WNDCLASS _frameClass;
            HINSTANCE _applicationInstance;

            HGLRC _openglContext;
            HDC _deviceContext;
            PIXELFORMATDESCRIPTOR _pixelFormatDescriptor;

            Vector2Int _anchor;
            Vector2UInt _size;

        public:
            Handle(Application* p_application, const std::string& p_title, const Vector2UInt& p_size);
            ~Handle();

            void configure(Application* p_application);

            void clear();
            void swap();
            void resize(const Vector2UInt& p_newSize);
            void place(const Vector2Int& p_anchor);

            const spk::Vector2UInt& size() const;
        };

        bool _creationComplete = false;
        Mode _mode;
        std::atomic_bool _isRunning;
        std::atomic_int _errorCode;

        Profiler _profiler;
        TimeManager _timeManager;
        Timer _counterTimer;
        TriggerMetric& _fpsCounter;
        TriggerMetric& _upsCounter;

        spk::ThreadSafeDeque<std::function<void()>> _updaterJobs;

        Keyboard _keyboard;
        Mouse _mouse;
        InputDecoder _inputDecoder;

        Handle _handle;
        CentralWidget* _centralWidget;

        LRESULT _handleMessage(const HWND& p_hwnd, const UINT& p_messageID, const WPARAM& p_firstParam, const LPARAM& p_secondParam);
        void _pullMessage();

        void _executeRendererTick();
        void _executeUpdaterTick();

        void _executeRendererLoop();
        void _executeUpdaterLoop();

        int _runMultithread();
        int _runMonothread();

    public:
        /**
         * @brief Constructor
         *
         * @param p_title A string containing the title of the application. Can contain unicode, expressed as std::string.
         * @param p_size The expected size of the window, as unsigned int in both axis and expressed as pixel.
         * @param p_mode The expected threading mode of the application. (CF the enum Mode for more information). Monothread by default.
         */
        Application(const std::string& p_title, const Vector2UInt& p_size, const Mode& p_mode = Mode::Monothread);

        /**
         * @brief Destructor
         */
        ~Application();

        /**
         * @brief Launch the application and return an error code once completed and quitted.
         * @return The error code of the application.
         */
        int run();

        /**
         * @brief Required a closure of the application, with the given error code.
         * @param p_error The error to return after the run().
         */
        void quit(int p_error);

        /**
         * @brief Return a const reference to the size of the screen.
         * @return The size of the screen.
         */
        const spk::Vector2UInt& size() const;

        /**
         * @brief Return a const reference to the Keyboard.
         * @return The Keyboard managed by the application.
         */
        const Keyboard& keyboard() const;

        /**
         * @brief Return a const reference to the Mouse.
         * @return The Mouse managed by the application.
         */
        const Mouse& mouse() const;

        /**
         * @brief Return a reference to the InputDecoder.
         * @return The InputDecoder managed by the application.
         */
        InputDecoder& inputDecoder();

        /**
         * @brief Return a const reference to the TimeManager.
         * @return The TimeManager managed by the application.
         */
        const TimeManager& timeManager() const;

        /**
         * @brief Return a reference to the Profiler.
         * @return The Profiler managed by the application.
         */
        Profiler& profiler();
    };
}