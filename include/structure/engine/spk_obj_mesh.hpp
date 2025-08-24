#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/engine/spk_mesh.hpp"
#include "structure/engine/spk_vertex.hpp"
#include "structure/graphics/texture/spk_texture.hpp"
#include "structure/spk_safe_pointer.hpp"

namespace spk
{
	class ObjMesh : public TMesh<Vertex>
	{
	private:
		static inline std::unordered_map<std::filesystem::path, spk::Texture> _materials;
		std::filesystem::path _materialPath;
		mutable spk::TContractProvider<spk::SafePointer<spk::Texture>> _onMaterialChangeProvider;

	public:
		using MaterialChangeContract = spk::TContractProvider<spk::SafePointer<spk::Texture>>::Contract;
		using MaterialChangeJob = spk::TContractProvider<spk::SafePointer<spk::Texture>>::Job;

		ObjMesh() = default;

		MaterialChangeContract onMaterialChange(const MaterialChangeJob &p_job) const;
		void setMaterial(const std::filesystem::path &p_materialPath);
		spk::SafePointer<const spk::Texture> material() const;

		void applyOffset(const spk::Vector3 &p_offset);

		static ObjMesh loadFromString(const std::string &p_input);
		static ObjMesh loadFromFile(const std::filesystem::path &p_path);

		void exportToFile(const std::filesystem::path &p_path) const;
	};
}
