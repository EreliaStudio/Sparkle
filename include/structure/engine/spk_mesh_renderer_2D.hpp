#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"

#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_mesh_2d.hpp"
#include "structure/engine/spk_material.hpp"

namespace spk
{
    class MeshRenderer2D : public spk::Component
    {
    private:
		ContractProvider::Contract _onTransformEditionContract;
        ContractProvider::Contract _onMeshValidationContract;
        ContractProvider::Contract _onMaterialValidationContract;

        spk::SafePointer<spk::Mesh2D> _mesh2D;
        spk::SafePointer<spk::Material> _material;

    public:
        MeshRenderer2D(const std::wstring& p_name);

        void setMesh(spk::SafePointer<spk::Mesh2D> p_mesh);
        spk::SafePointer<spk::Mesh2D> mesh() const;

        void setMaterial(spk::SafePointer<spk::Material> p_material);
        spk::SafePointer<spk::Material> material() const;

        void onPaintEvent(spk::PaintEvent& p_event);

        void awake() override;
        void sleep() override;
    };
}
