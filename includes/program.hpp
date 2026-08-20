#pragma once

#include <GL/glew.h>

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include "gpu_resource.hpp"
#include "index_buffer.hpp"

namespace spk
{
	class RenderContext;

	class Program final : public GPUResource
	{
	public:
		enum class Primitive
		{
			Points,
			Lines,
			LineLoop,
			LineStrip,
			Triangles,
			TriangleStrip,
			TriangleFan
		};

	private:
		class Instance;

		std::string _vertexShaderSource;
		std::string _fragmentShaderSource;
		std::unordered_map<std::string, std::size_t> _uniformBlockBindings;

		[[nodiscard]] static GLenum _openGLPrimitive(Primitive primitive);
		[[nodiscard]] static GLenum _openGLIndexType(IndexBuffer::Type type) noexcept;
		[[nodiscard]] static std::string _shaderLog(GLuint shader);
		[[nodiscard]] static std::string _programLog(GLuint program);
		[[nodiscard]] static GLuint _compileShader(GLenum type, const std::string &source);
		[[nodiscard]] static GLuint _linkProgram(GLuint vertexShader, GLuint fragmentShader);
		[[nodiscard]] static GLuint _buildProgram(const std::string &vertexSource, const std::string &fragmentSource);
		static void _validateGLCount(std::size_t count);

		void _applyUniformBlockBindings(GLuint identifier) const;

	protected:
		[[nodiscard]] Kind _kind() const noexcept override;
		[[nodiscard]] std::unique_ptr<GPUResource::Instance> _create(RenderContext &context) const override;
		void _synchronize(GPUResource::Instance &instance, RenderContext &context) const override;
		void _bind(GPUResource::Instance &instance, RenderContext &context) const override;

	public:
		Program() = default;
		Program(std::string vertexShaderSource, std::string fragmentShaderSource);

		void setSources(std::string vertexShaderSource, std::string fragmentShaderSource);
		[[nodiscard]] bool isValid() const noexcept;

		void bindUniformBlock(std::string name, std::size_t bindingPoint);

		void renderRaw(Primitive primitive, std::size_t firstVertex, std::size_t vertexCount) const;
		void render(Primitive primitive, IndexBuffer::Type indexType, std::size_t firstIndex, std::size_t indexCount) const;
		void renderInstanced(Primitive primitive, IndexBuffer::Type indexType, std::size_t firstIndex, std::size_t indexCount, std::size_t instanceCount) const;
	};
}