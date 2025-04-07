#pragma once

#include <GL/glew.h>

#include <GL/gl.h>

#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

#include "spk_sfinae.hpp"

#include "structure/container/spk_data_buffer_layout.hpp"
#include "structure/graphics/opengl/spk_vertex_buffer_object.hpp"

#include "spk_debug_macro.hpp"

namespace spk::OpenGL
{
	class UniformBufferObject : public VertexBufferObject
	{
	public:
		using BindingPoint = int;
		using Element = spk::DataBufferLayout::Element;

	private:
		std::wstring _blockName = L"Unnamed UBO";
		BindingPoint _bindingPoint = -1;
		spk::DataBufferLayout _dataBufferLayout;
		std::unordered_map<GLint, GLint> _programBlockIndex;

	public:
		UniformBufferObject() = default;
		UniformBufferObject(const std::wstring &p_blockName, BindingPoint p_bindingPoint, size_t p_size);

		UniformBufferObject(const UniformBufferObject &p_other);
		UniformBufferObject(UniformBufferObject &&p_other);

		UniformBufferObject &operator=(const UniformBufferObject &p_other);
		UniformBufferObject &operator=(UniformBufferObject &&p_other);

		void activate();

		const std::wstring &blockName() const;
		void setBlockName(const std::wstring &p_blockName);

		BindingPoint bindingPoint() const;
		void setBindingPoint(BindingPoint p_bindingPoint);

		DataBufferLayout &layout();
		const DataBufferLayout &layout() const;

		bool contains(const std::wstring &p_name);
		void resize(size_t p_size) override;
		DataBufferLayout::Element &addElement(const std::wstring &p_name, size_t p_offset, size_t p_size);
		DataBufferLayout::Element &addElement(const std::wstring &p_name, size_t p_offset, size_t p_nbElement, size_t p_elementSize,
											  size_t p_elementPadding);

		void removeElement(const std::wstring &p_name);

		DataBufferLayout::Element &operator[](size_t index);
		const DataBufferLayout::Element &operator[](size_t index) const;

		DataBufferLayout::Element &operator[](const std::wstring &key);
		const DataBufferLayout::Element &operator[](const std::wstring &key) const;
	};
}
