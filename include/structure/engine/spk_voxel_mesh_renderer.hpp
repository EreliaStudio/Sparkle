#pragma once

#include "structure/engine/spk_voxel_mesh.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/graphics/lumina/spk_shader.hpp"
#include "structure/graphics/lumina/spk_shader_object_factory.hpp"
#include "structure/graphics/texture/spk_image.hpp"
#include "structure/spk_safe_pointer.hpp"
#include "structure/engine/spk_transform.hpp"

namespace spk
{
        class VoxelMeshRenderer : public spk::Component
        {
        public:
                class Painter
                {
                private:
                        static inline spk::Lumina::ShaderObjectFactory::Instanciator _instanciator;

                        static spk::Lumina::Shader _createShader();
                        static spk::Lumina::Shader _shader;

                        spk::Lumina::Shader::Object _object;
                        spk::OpenGL::BufferSet &_bufferSet;
                        spk::OpenGL::SamplerObject &_diffuseSampler;
                        spk::OpenGL::UniformBufferObject &_transformUBO;

                public:
                        Painter();

                        void clear();
                        void prepare(const VoxelMesh &p_mesh);
                        void validate();
                        void render();

                        void setTransform(const spk::Transform &p_transform);
                        void setTexture(const spk::SafePointer<const spk::Texture> &p_texture);
                        const spk::SafePointer<const spk::Texture> &texture() const;
                };

        private:
                spk::SafePointer<VoxelMesh> _mesh;
                Painter _painter;
                spk::ContractProvider::Contract _onOwnerTransformEditionContract;

        public:
                VoxelMeshRenderer(const std::wstring &p_name);

                void setTexture(spk::SafePointer<const spk::Texture> p_texture);
                const spk::SafePointer<const spk::Texture> &texture() const;

                void setMesh(const spk::SafePointer<VoxelMesh> &p_mesh);
                const spk::SafePointer<VoxelMesh> &mesh() const;

                void onPaintEvent(spk::PaintEvent &p_event) override;
                void start() override;
        };
}
