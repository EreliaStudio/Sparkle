#pragma once

#include <regex>
#include <unordered_set>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include "graphics/opengl/spk_opengl_shader_instruction.hpp"
#include "graphics/opengl/spk_opengl_object.hpp"
#include "graphics/texture/spk_image.hpp"

namespace spk
{
    class Pipeline
    {    
    private:
        struct Structure
        {
            struct Element
            {
                size_t offsetWithPadding;
                size_t offsetWithoutPadding;
                size_t length;
                const Structure* pointer;
            };

            OpenGL::Type type;
            size_t sizeWithPadding;
            size_t sizeWithoutPadding;
            size_t nbElement;
            std::map<std::string, Element> elements;

            Structure();
            Structure(const std::map<std::string, Pipeline::Structure>& p_availibleStructures, const OpenGL::ShaderInstruction& p_inputInstruction);
            Structure(const OpenGL::Type& p_type, size_t p_nbElement, size_t p_size);

            void write(void *p_destination, const void *p_source, size_t p_size) const;
        };

        std::map<std::string, Structure> _structures;

	    void _loadStructureFromInstructions(const std::vector<OpenGL::ShaderInstruction>& p_instructions);

        class UniformObject
        {
        public:
            class Element
            {
                friend class UniformObject;
            private:
                void* _data;
                const Structure* _structure;
                std::map<std::string, Element> _elements;

                Element(void *p_data, const Structure* p_structureLayout);

            public:
                Element(const Element& p_other) = delete;
                Element& operator = (const Element& p_other) = delete;
                
                Element(Element&& p_other) = default;
                Element& operator = (Element&& p_other) = default;

                Element& operator[](const std::string& p_elementName);

                template <typename TType>
                Element& operator = (const TType& p_value)
                {
                    if (sizeof(TType) != _structure->sizeWithoutPadding)
                        throwException("Unexpected size received : Expected [" + std::to_string(_structure->sizeWithoutPadding) + "] vs received [" + std::to_string(sizeof(TType)) + "].");

                    _structure->write(_data, &p_value, _structure->sizeWithoutPadding);
                    return (*this);
                }
            };

            struct Layout
            {
                std::string type;
                size_t binding;
                const Structure* structure;
            };

        private:
            bool _needUpdate;
            OpenGL::UniformBufferObject _handle;
            std::vector<uint8_t> _buffer;
            Element _generalElement;
        
        protected:
            UniformObject(const GLuint& p_program, const Layout& p_layout);
        public:
            UniformObject(const UniformObject& p_other) = delete;
            UniformObject& operator = (const UniformObject& p_other) = delete;
            
            UniformObject(UniformObject&& p_other) = default;
            UniformObject& operator = (UniformObject&& p_other) = default;

            Element& operator[](const std::string& p_elementName);
            
            template <typename TType>
            UniformObject& operator = (const TType& p_value)
            {
                _generalElement = p_value;
                return (*this);
            }

            const std::vector<uint8_t>& buffer() const;
            
            void update();
            bool needUpdate() const;
            void push();

            void activate();
            void deactivate();
        };

    public:

        class Constant : public UniformObject
        {
            friend class Pipeline;

        public:
            static inline const std::string BlockKey = "ConstantBlock";
            using Element = UniformObject::Element;

        private:
            Constant(const GLuint& p_program, const Layout& p_layout);

        public:
            Constant(const Constant& p_other) = delete;
            Constant& operator = (const Constant& p_other) = delete;
            
            Constant(Constant&& p_other) = default;
            Constant& operator = (Constant&& p_other) = default;

            template <typename TType>
            Constant& operator = (const TType& p_value)
            {
                UniformObject::operator=(p_value);
                return (*this);
            }
        };

        class Object
        {
            friend class Pipeline;

        public:
            class Storage
            {
                friend class Object;
                
            public:
                struct Layout
                {
                    struct Element
                    {
                        size_t location;
                        size_t nbElement;
                        OpenGL::Type type;
                        size_t offset;
                    };

                    size_t stride;
                    std::vector<Element> elements;
                };

            private:
                size_t _stride;
                OpenGL::VertexBufferObject _vertices;
                OpenGL::VertexBufferObject _indexes;

                template <typename TVertexData>
                void setVertices(const std::vector<TVertexData>& p_verticesData)
                {
                    if (sizeof(TVertexData) != _stride)
                        throwException("Unexpected vertex size inside storage.\nExpected [" + std::to_string(_stride) + "] and received [" + std::to_string(sizeof(TVertexData)) + "].");
                    _vertices.push(p_verticesData.data(), p_verticesData.size() * sizeof(TVertexData));
                }

                void setVertices(const std::vector<float>& p_verticesData, size_t p_elementSize)
                {
                    if (p_elementSize != _stride)
                        throwException("Unexpected vertex size inside storage.\nExpected [" + std::to_string(_stride) + "] and received [" + std::to_string(p_elementSize) + "].");
                    _vertices.push(p_verticesData.data(), p_verticesData.size() * sizeof(float));
                }

                void setIndexes(const std::vector<size_t> p_indexesData);

            public:
                Storage(const Layout& p_layout);

                Storage(const Storage& p_other) = delete;
                Storage& operator = (const Storage& p_other) = delete;
                
                Storage(Storage&& p_other) = default;
                Storage& operator = (Storage&& p_other) = default;

                size_t nbTriangles() const;

                void activate();
                void deactivate();
            };

            class Attribute : public UniformObject
            {
                friend class Object;

            public:
                static inline const std::string BlockKey = "AttributeBlock";
                using Element = UniformObject::Element;
        
            private:
                Attribute(const GLuint& p_program, const Layout& p_layout);

            public:
                Attribute(const Attribute& p_other) = delete;
                Attribute& operator = (const Attribute& p_other) = delete;
                
                Attribute(Attribute&& p_other) = default;
                Attribute& operator = (Attribute&& p_other) = default;

                template <typename TType>
                Attribute& operator = (const TType& p_value)
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
            ~Object();

            void render();

            Object(const Object& p_other) = delete;
            Object& operator = (const Object& p_other) = delete;
            
            Object(Object&& p_other) = default;
            Object& operator = (Object&& p_other) = default;

            template <typename TVertexData>
            void setVertices(const std::vector<TVertexData>& p_verticesData)
            {
                _vao.activate();
                _storage.activate();
                
                _storage.setVertices(p_verticesData);

                _storage.deactivate();
                _vao.deactivate();
            }

            void setVertices(const std::vector<float>& p_verticesData, size_t p_elementSize)
            {
                _vao.activate();
                _storage.activate();
                
                _storage.setVertices(p_verticesData, p_elementSize);

                _storage.deactivate();
                _vao.deactivate();
            }

            void setIndexes(const std::vector<size_t> p_indexesData);

            Attribute& attribute(const std::string& p_attributeName);
        };

    public:
        class Texture
        {
            friend class Pipeline;
                        
        public:
            struct Layout
            {
                std::string name;
                int textureBindingPoint;
            };

        private:
            int _textureBindingPoint;
            OpenGL::SamplerObject _handle;
            Constant& _constant;
            Constant::Element& _unitElement;
            const spk::Texture* _activeTexture;

            Texture(int p_programID, const Layout& p_layout, Constant& p_constant);

        public:
            void attach(const spk::Texture* p_textureToSet);

            void activate();
        };

    private:
		std::map<std::string, UniformObject::Layout> _attributesLayout;
		std::map<std::string, UniformObject::Layout> _constantsLayout;
        std::map<std::string, Texture::Layout> _samplersLayout;

	std::map<std::string, Pipeline::UniformObject::Layout> _parseUniformLayout(
        const std::vector<OpenGL::ShaderInstruction>& p_shaderinstruction,
        const std::string& p_prefixName);
    std::map<std::string, Texture::Layout> _parseSamplerUniforms(const std::vector<OpenGL::ShaderInstruction>& p_shaderinstruction);

    public:

    private:
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
        Pipeline();
        Pipeline(const std::string& p_code);
        ~Pipeline();

        void activate();

        static Constant* globalConstant(const std::string& p_constantName);
        static void insertConstants(const std::string& p_constantCode);

        Object createObject();
        Constant& constant(const std::string& p_constantName);
        Texture& texture(const std::string& p_textureName);
    };
}