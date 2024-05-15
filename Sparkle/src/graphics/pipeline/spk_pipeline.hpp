#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "graphics/opengl/spk_opengl_object.hpp"
#include "graphics/opengl/spk_opengl_shader_instruction.hpp"
#include "graphics/texture/spk_image.hpp"
#include <iostream>
#include <regex>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace spk
{
    /**
     * @class Pipeline
     * @brief Manages the rendering pipeline, including shader programs, uniforms, and objects.
     *
     * The Pipeline class orchestrates the rendering process by managing shader programs,
     * uniform variables, and drawable objects. It supports loading shader instructions, defining
     * uniform structures, and creating objects with specific attributes and textures.
     *
     * The Pipeline facilitates communication with the GPU using OpenGL, allowing for efficient rendering of graphics.
     *
     * Pipeline code must be writed in Lumina code.
     *
     * Usage example:
     * @code
     * spk::Pipeline pipeline(shaderCode);
     * auto& constant = pipeline.constant("MyConstant");
     * auto object = pipeline.createObject();
     * object.render();
     * @endcode
     *
     * @see Pipeline::Object, Pipeline::Constant, Pipeline::Texture, Lumina (TODO)
     */
    class Pipeline
    {
    private:
        /**
         * @struct Structure
         * @brief Represents the layout of a structured data block used within the rendering pipeline, such as a uniform block or vertex attribute structure.
         *
         * This class defines the memory layout for complex data structures passed to shaders. It includes information about the overall size of the
         * structure (both with and without padding), the type of elements it contains, and a collection of `Element` instances that describe each field within
         * the structure. This detailed layout information is essential for correctly mapping application data to the GPU, ensuring that shader programs can
         * access structured data efficiently and accurately.
         *
         * `Structure` instances can be created directly based on OpenGL types and sizes, or they can be derived from shader instructions, allowing for dynamic
         * structure generation based on shader code analysis. This flexibility supports a wide range of use cases, from simple uniform blocks to complex
         * vertex data structures.
         *
         * Usage:
         * Structures are primarily used internally within the `Pipeline` class to manage the data layout for shader inputs, such as uniforms and vertex
         * attributes. They facilitate the correct binding and buffering of structured data to OpenGL shaders.
         */
        struct Structure
        {
            /**
             * @struct Element
             * @brief Describes a single element within a `Structure`, such as a field within a uniform block or a vertex attribute.
             *
             * This struct provides the offset (both with and without padding), length, and optionally a pointer to another `Structure` if
             * the element itself is a structured type. This allows for the definition of nested structures, supporting complex data layouts for
             * shader programs.
             *
             * Elements within a `Structure` are identified by name, enabling direct access and manipulation of specific fields within the structured
             * data. This detailed organization is critical for efficient GPU memory utilization and data access patterns.
             *
             * Usage:
             * Elements are defined as part of a `Structure` and used to specify the layout of each piece of data within the structure. This includes
             * setting up uniform buffers and vertex attribute arrays, where each element's offset and size information is used to map application
             * data correctly to shader inputs.
             */
            struct Element
            {
                size_t offsetWithPadding;    ///< The byte offset of the element from the start of the structure, including padding.
                size_t offsetWithoutPadding; ///< The byte offset of the element from the start of the structure, excluding padding.
                size_t length;               ///< The size of the element in bytes.
                const Structure* pointer;    ///< Optional pointer to another `Structure` if this element is a structured type.
            };

            OpenGL::Type type;                       ///< The OpenGL type of the structure.
            size_t sizeWithPadding;                  ///< Total size of the structure including padding, in bytes.
            size_t sizeWithoutPadding;               ///< Total size of the structure excluding padding, in bytes.
            size_t nbElement;                        ///< Number of elements contained within the structure.
            std::map<std::string, Element> elements; ///< A map of element names to their `Element` descriptions within the structure.

            /**
             * @brief Default constructor for creating an empty `Structure`.
             */
            Structure();

            /**
             * @brief Constructs a `Structure` based on a set of available structures and a shader instruction.
             * @param p_availibleStructures Map of available `Structure`s by name, used for building nested structures.
             * @param p_inputInstruction The shader instruction that defines how to interpret the structure.
             */
            Structure(const std::map<std::string, Pipeline::Structure>& p_availibleStructures, const OpenGL::ShaderInstruction& p_inputInstruction);

            /**
             * @brief Constructs a `Structure` with specified type, number of elements, and size.
             * @param p_type The OpenGL type of the structure.
             * @param p_nbElement The number of elements in the structure.
             * @param p_size The size of the structure, in bytes.
             */
            Structure(const OpenGL::Type& p_type, size_t p_nbElement, size_t p_size);

            /**
             * @brief Writes data from a source to a destination, taking the structure's layout into account.
             * @param p_destination Pointer to the destination where data should be written.
             * @param p_source Pointer to the source data to be written.
             * @param p_size Size of the data to be written, in bytes.
             */
            void write(void* p_destination, const void* p_source, size_t p_size) const;
        };

        std::map<std::string, Structure> _structures;

        void _loadStructureFromInstructions(const std::vector<OpenGL::ShaderInstruction>& p_instructions);

        /**
         * @class UniformObject
         * @brief Encapsulates a uniform buffer object (UBO) for Pipeline::Constants and Object::Attribute.
         *
         * This class manages a block of memory that stores uniform variables for use in a shader program.
         *
         * It provides a way to set individual uniform values within a structured layout, handling the
         * underlying OpenGL buffer object creation, data storage, and updating.
         *
         * Usage example: See Pipeline::Constant or Pipeline::Object::Attribute
         *
         * @see Pipeline::Structure, Pipeline::UniformObject::Element, Pipeline::Constants and Object::Attribute
         */
        class UniformObject
        {
        public:
            /**
             * @class Element
             * @brief Provides access and manipulation capabilities for elements within a UniformObject structure.
             *
             * This class allows for setting and getting values of specific elements within a UniformObject, using the structure's layout information. It supports type-safe assignments and retrieval of uniform values, ensuring data integrity when communicating with the shader program.
             *
             * Usage example:
             * @code
             * spk::Pipeline::UniformObject::Element element = uniformObject["elementName"];
             * element = newValue;
             * @endcode
             */
            class Element
            {
                friend class UniformObject;

            private:
                void* _data;
                const Structure* _structure;
                std::map<std::string, Element> _elements;

                Element(void* p_data, const Structure* p_structureLayout);

            public:
                /**
                 * @brief Overloaded subscript operator for accessing nested elements.
                 * @param p_elementName The name of the element to access.
                 * @return Reference to the nested element.
                 */
                Element& operator[](const std::string& p_elementName);

                /**
                 * @brief Overloaded assignment operator for assigning values to the element.
                 * @tparam TType The type of value being assigned.
                 * @param p_value The value to assign.
                 * @return Reference to the modified Element.
                 */
                template <typename TType>
                Element& operator=(const TType& p_value)
                {
                    if (sizeof(TType) != _structure->sizeWithoutPadding)
                        throwException("Unexpected size received : Expected [" + std::to_string(_structure->sizeWithoutPadding) + "] vs received [" + std::to_string(sizeof(TType)) + "].");

                    _structure->write(_data, &p_value, _structure->sizeWithoutPadding);
                    return (*this);
                }
            };

            /**
             * @struct Layout
             * @brief Describes the layout of a uniform object, including its type, binding, and structure.
             *
             * This struct is used to define the layout and binding information of uniform blocks within the shader programs.
             * It specifies the uniform's name, its binding point in the shader, and a reference to the structure
             * defining its internal layout.
             *
             * Usage example: Not directly instantiated by users, used internally by Pipeline to organize uniforms.
             */
            struct Layout
            {
                std::string type;           ///< Type of the uniform object.
                size_t binding;             ///< Binding point in the shader.
                const Structure* structure; ///< Pointer to the structure layout.
            };

        private:
            bool _needUpdate;
            OpenGL::UniformBufferObject _handle;
            std::vector<uint8_t> _buffer;
            Element _generalElement;

        protected:
            /**
             * @brief Constructor used by inheriting classes.
             *
             * @param p_program The program creating the UniformObject.
             * @param p_layout The layout used for this UniformObject.
             */
            UniformObject(const GLuint& p_program, const Layout& p_layout);

        public:
            /**
             * @brief Prevents copy construction to ensure unique management of uniform buffer objects.
             */
            UniformObject(const UniformObject& p_other) = delete;
            /**
             * @brief Prevents copy assignment to ensure unique management of uniform buffer objects.
             */
            UniformObject& operator=(const UniformObject& p_other) = delete;

            /**
             * @brief Supports move semantics for efficient transfer of uniform buffer resources.
             */
            UniformObject(UniformObject&& p_other) = default;
            /**
             * @brief Supports move semantics for efficient transfer of uniform buffer resources.
             */
            UniformObject& operator=(UniformObject&& p_other) = default;

            /**
             * @brief Overloaded subscript operator for accessing elements by name.
             * @param p_elementName The name of the element to access.
             * @return Reference to the accessed Element.
             */
            Element& operator[](const std::string& p_elementName);

            /**
             * @brief Overloaded assignment operator for assigning a value to the uniform object.
             * @tparam TType The type of value being assigned.
             * @param p_value The value to assign.
             * @return Reference to the modified UniformObject.
             */
            template <typename TType>
            UniformObject& operator=(const TType& p_value)
            {
                _generalElement = p_value;
                return (*this);
            }

            /**
             * @brief Getter for the buffer containing data.
             * @return Reference to the buffer.
             */
            const std::vector<uint8_t>& buffer() const;

            /**
             * @brief Updates the uniform object with the current data.
             */
            void update();

            /**
             * @brief Checks if the uniform object needs updating.
             * @return True if the object needs updating, otherwise false.
             */
            bool needUpdate() const;

            /**
             * @brief Pushes the uniform object data to the GPU.
             */
            void push();

            /**
             * @brief Activates the uniform object.
             */
            void activate();

            /**
             * @brief Deactivates the uniform object.
             */
            void deactivate();
        };

    public:
        /**
         * @class Constant
         * @brief Specializes UniformObject for managing global constants in Pipeline.
         *
         * This class is a specialization of UniformObject designed for handling
         * global shader constants that do not change per object or draw call.
         *
         * It simplifies the process of managing constants that are common across
         * multiple objects or the entire render pipeline.
         *
         * Usage example:
         * @code
         * spk::Pipeline::Constant& constant = pipeline.constant("MyConstant");
         * constant["myValue"] = 42;
         * constant.push();
         * @endcode
         *
         * @note Constant sharing the same name are shared by all Pipeline created, allowing
         * to use Constant already pushed and configured by the user or Sparkle.
         *
         * @see Pipeline::UniformObject for more information
         */
        class Constant : public UniformObject
        {
            friend class Pipeline;
            friend size_t convertConstantBlock(std::string& p_source);

        public:
            /**
             * @brief Type alias for UniformObject's Element. See UniformObject::Element for more details.
             */
            using Element = UniformObject::Element;

        private:
            static inline const std::string BlockKey = "ConstantBlock";

            Constant(const GLuint& p_program, const Layout& p_layout);

        public:
            /**
             * @brief Prevents copy construction to ensure unique management of uniform buffer objects.
             */
            Constant(const Constant& p_other) = delete;

            /**
             * @brief Prevents copy assignment to ensure unique management of uniform buffer objects.
             */
            Constant& operator=(const Constant& p_other) = delete;

            /**
             * @brief Supports move semantics for efficient transfer of uniform buffer resources.
             */
            Constant(Constant&& p_other) = default;

            /**
             * @brief Supports move semantics for efficient transfer of uniform buffer resources.
             */
            Constant& operator=(Constant&& p_other) = default;

            /**
             * @brief Assigns a value to the constant, updating the underlying uniform buffer with the new value.
             *
             * @tparam TType The data type of the value being assigned to the constant.
             * @param p_value The new value for the constant.
             * @return A reference to this Constant object, allowing for method chaining.
             */
            template <typename TType>
            Constant& operator=(const TType& p_value)
            {
                UniformObject::operator=(p_value);
                return (*this);
            }
        };

        /**
         * @class Object
         * @brief Represents a drawable object within the Pipeline, including geometry and attributes.
         *
         * This class is responsible for managing the vertex array object (VAO), vertex buffer object (VBO),
         * and any associated attributes or textures.
         *
         * It encapsulates the geometry (vertices and indices) and provides methods to set attribute
         * values and draw the object using the Pipeline that created the Object.
         *
         * Usage example:
         * @code
         * spk::Pipeline::Object object = pipeline.createObject();
         * object.setVertices(myVertices);
         * object.setAttribute("MyAttribute", value);
         * object.render();
         * @endcode
         *
         * @see Pipeline::Object::Attribute
         */
        class Object
        {
            friend class Pipeline;

        public:
            /**
             * @class Storage
             * @brief Manages the storage for vertex and index buffers associated with a drawable object.
             *
             * The Storage class encapsulates vertex and index data for an Object, handling the creation
             * and management of vertex buffer objects (VBOs) and element buffer objects (EBOs).
             * It supports the setup of vertex attributes based on a defined layout.
             *
             * Usage example: Not directly instantiated by users, used internally by Object.
             */
            class Storage
            {
                friend class Object;

            public:
                /**
                 * @struct Layout
                 * @brief Defines the layout of vertex data for the Storage class, including stride and element information.
                 *
                 * This structure specifies how vertex data is organized, including the stride between vertices and
                 * details about each vertex attribute (e.g., position, normal, texture coordinates).
                 * It is critical for setting up the vertex attribute pointers correctly in OpenGL.
                 *
                 * Usage example: Not directly instantiated by users, used internally by Object::Storage.
                 */
                struct Layout
                {
                    /**
                     * @struct Element
                     * @brief Describes a single element within a uniform or vertex attribute structure for rendering.
                     *
                     * This struct provides detailed information about each element within a `Structure`, such as its
                     * offset within the structure, its length (in bytes), and a pointer to another `Structure` if this
                     * element is itself a structured type. This enables precise layout descriptions for complex uniform
                     * or vertex data, facilitating correct shader program interactions.
                     *
                     * Element instances are used to map application data to GPU memory layouts, ensuring that data is
                     * correctly aligned and formatted for efficient access by OpenGL shaders.
                     *
                     * Usage is internal and is primarily involved in the setup and management of uniform buffers and
                     * vertex attribute buffers.
                     */
                    struct Element
                    {
                        size_t location;   ///< The location index of the element within the shader.
                        size_t nbElement;  ///< The number of elements within this vertex attribute.
                        OpenGL::Type type; ///< The data type of the element (e.g., float, vec3).
                        size_t offset;     ///< The byte offset of the element from the start of the structure.
                    };

                    size_t stride;                 ///< The stride (in bytes) between consecutive vertices in the buffer.
                    std::vector<Element> elements; ///< A collection of Element structures defining the attributes of the vertex.
                };

            private:
                size_t _stride;
                OpenGL::VertexBufferObject _vertices;
                OpenGL::VertexBufferObject _indexes;

                template <typename TShaderInput>
                void setVertices(const std::vector<TShaderInput>& p_verticesData)
                {
                    if (sizeof(TShaderInput) != _stride)
                        throwException("Unexpected vertex size inside storage.\nExpected [" + std::to_string(_stride) + "] and received [" + std::to_string(sizeof(TShaderInput)) + "].");
                    _vertices.push(p_verticesData.data(), p_verticesData.size() * sizeof(TShaderInput));
                }

                void setVertices(const std::vector<float>& p_verticesData, size_t p_elementSize)
                {
                    if (p_elementSize != _stride && p_elementSize != 0)
                        throwException("Unexpected vertex size inside storage.\nExpected [" + std::to_string(_stride) + "] and received [" + std::to_string(p_elementSize) + "].");
                    _vertices.push(p_verticesData.data(), p_verticesData.size() * sizeof(float));
                }

                void setIndexes(const std::vector<unsigned int> p_indexesData);

            public:
                /**
                 * @brief Constructs a Storage object with a specified layout.
                 * @param p_layout The layout definition for the vertex data to be stored.
                 */
                Storage(const Layout& p_layout);

                /**
                 * @brief Deleted copy constructor to prevent copying of Storage instances.
                 * @param p_other The other Storage object to copy from.
                 */
                Storage(const Storage& p_other) = delete;

                /**
                 * @brief Deleted assignment operator to prevent copying of Storage instances.
                 * @param p_other The other Storage object to assign from.
                 * @return Reference to this Storage object.
                 */
                Storage& operator=(const Storage& p_other) = delete;

                /**
                 * @brief Default move constructor.
                 * @param p_other The other Storage object to move from.
                 */
                Storage(Storage&& p_other) = default;

                /**
                 * @brief Default move assignment operator.
                 * @param p_other The other Storage object to move assign from.
                 * @return Reference to this Storage object.
                 */
                Storage& operator=(Storage&& p_other) = default;

                /**
                 * @brief Returns the number of triangles stored.
                 * @return The number of triangles, calculated based on the stored indices.
                 */
                size_t nbTriangles() const;

                /**
                 * @brief Returns the number of vertices stored.
                 * @return The number of vertices, calculated based on the stored indices.
                 */
                size_t nbVertices() const;

                /**
                 * @brief Activates this Storage object, making it the current target for rendering operations.
                 */
                void activate();

                /**
                 * @brief Deactivates this Storage object, removing it from being the current target for rendering operations.
                 */
                void deactivate();
            };

            /**
             * @class Attribute
             * @brief Specializes UniformObject for managing object-specific attributes in shaders.
             *
             * This class extends UniformObject to handle attributes specific to an Object, such as material properties or transformation matrices. It provides a mechanism to update these attributes individually, optimizing the rendering process.
             *
             * Usage example: Not directly instantiated by users, used internally by Object.
             *
             * @see Pipeline::UniformObject for more information
             */
            class Attribute : public UniformObject
            {
                friend class Pipeline;
                friend void convertAttributeBlock(std::string& p_source, size_t p_higherBindingPointUsed);
                friend class Object;

            public:
                /**
                 * @brief Type alias for UniformObject's Element. See UniformObject::Element for more details.
                 */
                using Element = UniformObject::Element;

            private:
                static inline const std::string BlockKey = "AttributeBlock";
                Attribute(const GLuint& p_program, const Layout& p_layout);

            public:
                /**
                 * @brief Deleted copy constructor to prevent copying of instances.
                 * @param p_other The other Attribute object to copy from.
                 */
                Attribute(const Attribute& p_other) = delete;

                /**
                 * @brief Deleted assignment operator to prevent copying of instances.
                 * @param p_other The other Attribute object to assign from.
                 * @return Reference to this Attribute object.
                 */
                Attribute& operator=(const Attribute& p_other) = delete;

                /**
                 * @brief Default move constructor.
                 * @param p_other The other Attribute object to move from.
                 */
                Attribute(Attribute&& p_other) = default;

                /**
                 * @brief Default move assignment operator.
                 * @param p_other The other Attribute object to move assign from.
                 * @return Reference to this Attribute object.
                 */
                Attribute& operator=(Attribute&& p_other) = default;

                /**
                 * @brief Templated assignment operator to set the attribute's value.
                 *
                 * This method leverages the assignment operator of the base class (UniformObject) to update the attribute's value.
                 *
                 * @tparam TType The type of the value to assign to the attribute.
                 * @param p_value The value to assign to the attribute.
                 * @return Reference to this Attribute object, allowing for method chaining.
                 */
                template <typename TType>
                Attribute& operator=(const TType& p_value)
                {
                    UniformObject::operator=(p_value);
                    return (*this);
                }
            };

        private:
            Pipeline* _pipeline;
            OpenGL::VertexArrayObject _vao;
            Storage _storage;
            std::map<std::string, Attribute> _attributes;

            Object(Pipeline* p_pipeline, const Storage::Layout& p_storageLayout, const std::map<std::string, UniformObject::Layout>& p_layouts);

        public:
            /**
             * @brief Destructor.
             *
             * Cleans up the Object instance, ensuring that all associated resources such as vertex arrays
             * and buffers are properly deleted.
             */
            ~Object();

            /**
             * @brief Renders the object using the current Pipeline's shader program.
             *
             * This method binds the object's vertex array and draws it using the appropriate OpenGL draw call.
             * It is assumed that the Pipeline's shader program is already activated and all necessary uniforms
             * are set before calling this method.
             */
            void render();

            /**
             * @brief Deleted copy constructor to prevent copying of instances.
             * @param p_other The other Object object to copy from.
             */
            Object(const Object& p_other) = delete;

            /**
             * @brief Deleted assignment operator to prevent copying of instances.
             * @param p_other The other Object object to assign from.
             * @return Reference to this Object object.
             */
            Object& operator=(const Object& p_other) = delete;

            /**
             * @brief Default move constructor.
             * @param p_other The other Object object to move from.
             */
            Object(Object&& p_other) = default;

            /**
             * @brief Default move assignment operator.
             * @param p_other The other Object object to move assign from.
             * @return Reference to this Object object.
             */
            Object& operator=(Object&& p_other) = default;

            /**
             * @brief Sets the vertex data for the object.
             *
             * Template method to set the vertex data of the object. The vertex data is expected to conform to
             * the layout specified by the Pipeline. This method updates the vertex buffer with the provided data.
             *
             * @tparam TShaderInput The data type of the vertex data array.
             * @param p_verticesData A vector containing the vertex data.
             */
            template <typename TShaderInput>
            void setVertices(const std::vector<TShaderInput>& p_verticesData)
            {
                _vao.activate();
                _storage.activate();

                _storage.setVertices(p_verticesData);

                _storage.deactivate();
                _vao.deactivate();
            }

            /**
             * @brief Sets the vertex data for the object with a specified element size.
             *
             * This overload allows setting vertex data using a raw float vector and specifying the size of each
             * element. It is useful when the vertex data does not directly map to a specific struct.
             *
             * @param p_verticesData A vector containing the vertex data as floats.
             * @param p_elementSize The size in bytes of each element in the vertex data array.
             */
            void setVertices(const std::vector<float>& p_verticesData, size_t p_elementSize);

            /**
             * @brief Sets the index data for the object.
             *
             * Updates the index buffer with the provided index data. Index data is used to specify the order
             * in which vertices are drawn, enabling the use of vertex reuse and more efficient rendering.
             *
             * @param p_indexesData A vector containing the index data.
             */
            void setIndexes(const std::vector<unsigned int> p_indexesData);

            /**
             * @brief Retrieves a reference to an attribute by name.
             *
             * Provides access to a specific attribute of the object for setting its value. Attributes are
             * shader-specific properties such as transformation matrices or material properties.
             *
             * @param p_attributeName The name of the attribute to retrieve.
             * @return A reference to the specified Attribute object, allowing for its value to be set.
             */
            Attribute& attribute(const std::string& p_attributeName);

            /**
             * @brief Return the storage of the object
             * 
             * @return The storage of the object, as const reference
            */
			const Storage& storage() const;
        };

        /**
         * @class Texture
         * @brief Manages texture units and binding to shaders.
         *
         * This class handles the association of texture data with shader sampler uniforms.
         * It allows for textures to be attached and activated within the rendering pipeline,
         * making sure textures are correctly bound to their designated texture units
         * and accessible within shader programs.
         *
         * Usage example:
         * @code
         * spk::Pipeline::Texture texture = pipeline.texture("MyTexture");
         * texture.attach(myTexture);
         *
         * //The texture will be automaticaly activated by the Pipeline when renderer
         * @endcode
         *
         * @see spk::Texture
         */
        class Texture
        {
            friend class Pipeline;

        public:
            /**
             * @struct Layout
             * @brief Defines the name and binding point for the Texture class.
             *
             * It is critical for setting up the sampler correctly in OpenGL.
             *
             * Usage example: Not directly instantiated by users, used internally by Pipeline::Texture.
             */
            struct Layout
            {
                std::string name;        //!< The name used inside Lumina.
                int textureBindingPoint; //!< The binding point defined by Sparkle for this texture.
            };

        private:
            int _textureBindingPoint;
            OpenGL::SamplerObject _handle;
            const spk::Texture* _activeTexture;
            static inline const spk::Texture* _lastActiveTexture = nullptr;

            Texture(int p_programID, const Layout& p_layout);

            void _activate();

        public:
            /**
             * @brief Attaches a texture to this Texture instance.
             *
             * This method binds a texture resource to the Texture instance, making it available for use
             * in the rendering pipeline. The texture is associated with the specified sampler uniform in
             * the shader, enabling it to be accessed and used for rendering operations.
             *
             * @param p_textureToSet A pointer to the texture resource to be attached. This texture will
             * be bound to the shader's sampler uniform specified by the Texture instance's layout.
             */
            void attach(const spk::Texture* p_textureToSet);
        };

    private:
        std::map<std::string, UniformObject::Layout> _attributesLayout;
        std::map<std::string, UniformObject::Layout> _constantsLayout;
        std::map<std::string, Texture::Layout> _samplersLayout;

        std::map<std::string, Pipeline::UniformObject::Layout> _parseUniformLayout(
            const std::vector<OpenGL::ShaderInstruction>& p_shaderinstruction,
            const std::string& p_prefixName);
        std::map<std::string, Texture::Layout> _parseSamplerUniforms(const std::vector<OpenGL::ShaderInstruction>& p_shaderinstruction);

        bool _isLoaded = false;
        std::string _vertexString = "";
        std::string _fragmentString = "";

        static inline std::map<std::string, Constant> _constants;

        GLuint _programID;
        Object::Storage::Layout _storageLayout;
        std::map<std::string, Texture> _textures;

        std::vector<Constant*> _activeConstants;

        Object::Storage::Layout _parseStorageLayout(const std::vector<OpenGL::ShaderInstruction>& p_shaderInstruction);

        void _load();

        void launch(size_t p_nbTriangle);

    public:
        /**
         * @brief Default constructor.
         *
         * Initializes a new Pipeline instance without any associated shader code. This pipeline needs to be
         * configured with shader code and other resources before it can be used for rendering.
         */
        Pipeline();

        /**
         * @brief Constructs a Pipeline with the provided shader code.
         *
         * Initializes a new Pipeline instance and compiles the provided shader code into an OpenGL shader program.
         * This constructor allows for immediate setup and use of the pipeline for rendering operations.
         *
         * @param p_code The GLSL shader code for both vertex and fragment shaders.
         */
        Pipeline(const std::string& p_code);

        /**
         * @brief Destructor.
         *
         * Cleans up the Pipeline instance, deleting the associated OpenGL shader program and freeing any allocated resources.
         */
        ~Pipeline();

        /**
         * @brief Activates the shader program managed by this Pipeline.
         *
         * Binds the Pipeline's shader program as the current program for rendering operations. This method must be called
         * before drawing objects or setting uniforms to ensure that the correct shader program is used.
         */
        void activate();

        /**
         * @brief Creates a drawable Object within the Pipeline.
         *
         * Constructs a new Object that can be rendered using this Pipeline. The Object is configured with the appropriate
         * shader program and can have its vertices, indices, attributes, and textures set up for rendering.
         *
         * @return An Object instance ready for configuration and rendering.
         */
        Object createObject();

        /**
         * @brief Retrieves a reference to a named Constant within the Pipeline.
         *
         * Accesses a Constant by name, providing a way to set and update global shader constants that are shared
         * across multiple objects or the entire render pipeline.
         *
         * @param p_constantName The name of the Constant to retrieve.
         * @return A reference to the specified Constant.
         */
        Constant& constant(const std::string& p_constantName);

        /**
         * @brief Retrieves a reference to a named Texture within the Pipeline.
         *
         * Accesses a Texture by name, allowing for the association of texture data with shader sampler uniforms.
         * This facilitates the binding and activation of textures within shaders.
         *
         * @param p_textureName The name of the Texture to retrieve.
         * @return A reference to the specified Texture.
         */
        Texture& texture(const std::string& p_textureName);

        /**
         * @brief Inserts global constants into the Pipeline.
         *
         * Allows for the definition and insertion of global shader constants into the Pipeline. These constants
         * are shared across all instances of the Pipeline, enabling consistent and centralized management of shader
         * uniform data.
         *
         * @param p_constantCode The GLSL code defining the constants to be inserted.
         */
        static void insertConstants(const std::string& p_constantCode);

        /**
         * @brief Retrieves a pointer to a global Constant shared across Pipelines.
         *
         * Accesses a global Constant by name, enabling the use of shared constants across multiple Pipeline instances.
         * This method provides a way to work with constants that are common to various parts of the application.
         *
         * @param p_constantName The name of the global Constant to retrieve.
         * @return A pointer to the specified global Constant.
         */
        static Constant* globalConstant(const std::string& p_constantName);
    };
}