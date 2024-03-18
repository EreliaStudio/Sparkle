#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk
{
	Pipeline::Object::Attribute::Attribute(const GLuint& p_program, const Layout& p_layout) :
		Pipeline::UniformObject(p_program, p_layout)
	{

	}

	Pipeline::Object::Object(Pipeline* p_pipeline, const Storage::Layout& p_storageLayout, const std::map<std::string, UniformObject::Layout>& p_layouts) :
		_pipeline(p_pipeline),
		_vao(),
		_storage(p_storageLayout)
	{
		for (const auto& [key, layout] : p_layouts)
		{
			_attributes.emplace(key, std::move(Attribute(p_pipeline->_programID, layout)));
		}
	}

    Pipeline::Object::~Object()
	{

	}

    void Pipeline::Object::render()
	{
		_pipeline->activate();
		
		_vao.activate();
		_storage.activate();

		for (auto& [key, attribute] : _attributes)
		{
			attribute.activate();
		}

		_pipeline->launch(_storage.nbTriangles());

		_storage.deactivate();
		_vao.deactivate();
	}

	void Pipeline::Object::setVertices(const std::vector<float>& p_verticesData, size_t p_elementSize)
	{
		_vao.activate();
		_storage.activate();
		
		_storage.setVertices(p_verticesData, p_elementSize);

		_storage.deactivate();
		_vao.deactivate();
	}

	void Pipeline::Object::setIndexes(const std::vector<size_t> p_indexesData)
	{
		_vao.activate();
        _storage.activate();

		_storage.setIndexes(p_indexesData);

		_storage.deactivate();
		_vao.deactivate();
	}

    Pipeline::Object::Attribute& Pipeline::Object::attribute(const std::string& p_attributeName)
	{
		if (_attributes.contains(p_attributeName) == false)
			throwException("Attribute [" + p_attributeName + "] doesn't exist");
		return (_attributes.at(p_attributeName));
	}
}