#include "structure/engine/spk_obj_mesh.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>

#include "spk_debug_macro.hpp"
#include "structure/graphics/texture/spk_image.hpp"
#include "structure/spk_iostream.hpp"

namespace spk
{
	namespace
	{
		static Vertex parseVertex(
			const std::string &p_token,
			const std::vector<spk::Vector3> &p_positions,
			const std::vector<spk::Vector2> &p_uvs,
			const std::vector<spk::Vector3> &p_normals)
		{
			Vertex result;
			std::array<std::string, 3> parts = {"", "", ""};
			size_t start = 0;
			size_t index = 0;
			for (size_t i = 0; i <= p_token.size() && index < parts.size(); ++i)
			{
				if (i == p_token.size() || p_token[i] == '/')
				{
					parts[index] = p_token.substr(start, i - start);
					++index;
					start = i + 1;
				}
			}

			int posIndex = parts[0].empty() == true ? 0 : std::stoi(parts[0]);
			int uvIndex = parts[1].empty() == true ? 0 : std::stoi(parts[1]);
			int normIndex = parts[2].empty() == true ? 0 : std::stoi(parts[2]);

			if (posIndex > 0 && posIndex <= static_cast<int>(p_positions.size()))
			{
				result.position = p_positions[static_cast<std::size_t>(posIndex - 1)];
			}
			else
			{
				GENERATE_ERROR("OBJ: position index out of range: " + parts[0]);
			}
			if (uvIndex > 0 && uvIndex <= static_cast<int>(p_uvs.size()))
			{
				result.uv = p_uvs[static_cast<std::size_t>(uvIndex - 1)];
			}
			else
			{
				GENERATE_ERROR("OBJ: uv index out of range: " + parts[1]);
			}
			if (normIndex > 0 && normIndex <= static_cast<int>(p_normals.size()))
			{
				result.normal = p_normals[static_cast<std::size_t>(normIndex - 1)];
			}
			else
			{
				GENERATE_ERROR("OBJ: normal index out of range: " + parts[2]);
			}

			return result;
		}
	} // namespace

	void ObjMesh::setMaterial(const std::filesystem::path &p_materialPath)
	{
		_materialPath = p_materialPath;
		auto it = _materials.find(p_materialPath);
		if (it == _materials.end())
		{
			spk::Image image;
			image.loadFromFile(p_materialPath);
			_materials.emplace(p_materialPath, std::move(image));
			it = _materials.find(p_materialPath);
		}
		_onMaterialChangeProvider.trigger(spk::SafePointer<spk::Texture>(&it->second));
	}

	spk::SafePointer<const spk::Texture> ObjMesh::material() const
	{
		auto it = _materials.find(_materialPath);
		if (it == _materials.end())
		{
			return (spk::SafePointer<const spk::Texture>(nullptr));
		}
		return (spk::SafePointer<const spk::Texture>(&it->second));
	}

	ObjMesh::MaterialChangeContract ObjMesh::onMaterialChange(const MaterialChangeJob &p_job) const
	{
		return (_onMaterialChangeProvider.subscribe(p_job));
	}

	void ObjMesh::applyOffset(const spk::Vector3 &p_offset)
	{
		for (auto &shape : shapes())
		{
			if (std::holds_alternative<Triangle>(shape) == true)
			{
				auto &tmp = std::get<Triangle>(shape);

				tmp.a.position += p_offset;
				tmp.b.position += p_offset;
				tmp.c.position += p_offset;
			}
			else
			{
				auto &tmp = std::get<Quad>(shape);

				tmp.a.position += p_offset;
				tmp.b.position += p_offset;
				tmp.c.position += p_offset;
				tmp.d.position += p_offset;
			}
		}
	}

	ObjMesh ObjMesh::loadFromString(const std::string &p_input)
	{
		ObjMesh result;

		std::vector<spk::Vector3> positions;
		std::vector<spk::Vector2> uvs;
		std::vector<spk::Vector3> normals;

		std::istringstream stream(p_input);
		std::string line;
		while (true)
		{
			std::getline(stream, line);
			if (stream.fail() == true)
			{
				break;
			}

			std::istringstream lineStream(line);
			std::string prefix;
			lineStream >> prefix;
			if (lineStream.fail() == true)
			{
				continue;
			}

			if (prefix == "v")
			{
				float x = 0.0f;
				float y = 0.0f;
				float z = 0.0f;
				lineStream >> x >> y >> z;
				positions.emplace_back(x, y, z);
			}
			else if (prefix == "vt")
			{
				float u = 0.0f;
				float v = 0.0f;
				lineStream >> u >> v;
				uvs.emplace_back(u, v);
			}
			else if (prefix == "vn")
			{
				float x = 0.0f;
				float y = 0.0f;
				float z = 0.0f;
				lineStream >> x >> y >> z;
				normals.emplace_back(x, y, z);
			}
			else if (prefix == "f")
			{
				std::vector<Vertex> verts;
				std::string token;
				while (true)
				{
					lineStream >> token;
					if (lineStream.fail() == true)
					{
						break;
					}
					verts.push_back(parseVertex(token, positions, uvs, normals));
				}

				if (verts.size() == 3)
				{
					result.addShape(verts[0], verts[1], verts[2]);
				}
				else if (verts.size() == 4)
				{
					result.addShape(verts[0], verts[1], verts[2], verts[3]);
				}
			}
		}

		return (result);
	}

	ObjMesh ObjMesh::loadFromFile(const std::filesystem::path &p_path)
	{
		spk::cout << L"[ObjMesh] Loading OBJ from " << spk::StringUtils::stringToWString(p_path.string()) << std::endl;

		std::ifstream file(p_path);
		if (file.is_open() == false)
		{
			GENERATE_ERROR("Failed to open OBJ file: " + p_path.string());
		}

		std::stringstream buffer;
		std::string mtllib;

		std::string line;
		while (std::getline(file, line))
		{
			buffer << line << "\n";
			std::istringstream lineStream(line);
			std::string prefix;
			lineStream >> prefix;
			if (prefix == "mtllib")
			{
				lineStream >> mtllib;
				spk::cout << L"[ObjMesh] Found mtllib " << spk::StringUtils::stringToWString(mtllib) << std::endl;
			}
		}

		ObjMesh result = loadFromString(buffer.str());

		if (mtllib.empty() == false)
		{
			std::filesystem::path mtlPath = p_path.parent_path() / mtllib;
			std::ifstream mtlFile(mtlPath);
			if (mtlFile.is_open() == true)
			{
				std::string mtlLine;
				while (std::getline(mtlFile, mtlLine))
				{
					std::istringstream mtlStream(mtlLine);
					std::string mtlPrefix;
					mtlStream >> mtlPrefix;
					if (mtlPrefix == "map_Kd")
					{
						std::string textureFile;
						mtlStream >> textureFile;
						auto texturePath = mtlPath.parent_path() / textureFile;
						spk::cout << L"[ObjMesh] Found texture " << spk::StringUtils::stringToWString(texturePath.string()) << std::endl;
						result.setMaterial(texturePath);
						break;
					}
				}
			}
			else
			{
				spk::cout << L"[ObjMesh] Failed to open MTL file " << spk::StringUtils::stringToWString(mtlPath.string()) << std::endl;
			}
		}

		return (result);
	}

	void ObjMesh::exportToFile(const std::filesystem::path &p_path) const
	{
		std::ofstream file(p_path);
		if (file.is_open() == false)
		{
			return;
		}

		std::vector<spk::Vector3> positions;
		std::vector<spk::Vector2> uvs;
		std::vector<spk::Vector3> normals;
		std::unordered_map<spk::Vector3, unsigned int> positionMap;
		std::unordered_map<spk::Vector2, unsigned int> uvMap;
		std::unordered_map<spk::Vector3, unsigned int> normalMap;

		struct Index
		{
			unsigned int position = 0;
			unsigned int uv = 0;
			unsigned int normal = 0;
		};
		std::vector<std::vector<Index>> faces;

		const auto &shapeList = shapes();
		for (const auto &shape : shapeList)
		{
			std::vector<Index> face;

			auto processVertex = [&](const Vertex &p_vertex) -> Index
			{
				Index idx;
				auto posIt = positionMap.find(p_vertex.position);
				if (posIt == positionMap.end())
				{
					idx.position = static_cast<unsigned int>(positions.size());
					positions.push_back(p_vertex.position);
					positionMap[p_vertex.position] = idx.position;
				}
				else
				{
					idx.position = posIt->second;
				}

				auto uvIt = uvMap.find(p_vertex.uv);
				if (uvIt == uvMap.end())
				{
					idx.uv = static_cast<unsigned int>(uvs.size());
					uvs.push_back(p_vertex.uv);
					uvMap[p_vertex.uv] = idx.uv;
				}
				else
				{
					idx.uv = uvIt->second;
				}

				auto normIt = normalMap.find(p_vertex.normal);
				if (normIt == normalMap.end())
				{
					idx.normal = static_cast<unsigned int>(normals.size());
					normals.push_back(p_vertex.normal);
					normalMap[p_vertex.normal] = idx.normal;
				}
				else
				{
					idx.normal = normIt->second;
				}

				return idx;
			};

			if (std::holds_alternative<Triangle>(shape) == true)
			{
				const auto &t = std::get<Triangle>(shape);
				face.push_back(processVertex(t.a));
				face.push_back(processVertex(t.b));
				face.push_back(processVertex(t.c));
			}
			else
			{
				const auto &q = std::get<Quad>(shape);
				face.push_back(processVertex(q.a));
				face.push_back(processVertex(q.b));
				face.push_back(processVertex(q.c));
				face.push_back(processVertex(q.d));
			}

			faces.push_back(face);
		}

		for (const auto &p : positions)
		{
			file << "v " << p.x << " " << p.y << " " << p.z << "\n";
		}
		for (const auto &t : uvs)
		{
			file << "vt " << t.x << " " << t.y << "\n";
		}
		for (const auto &n : normals)
		{
			file << "vn " << n.x << " " << n.y << " " << n.z << "\n";
		}
		for (const auto &face : faces)
		{
			file << "f";
			for (const auto &idx : face)
			{
				file << " " << idx.position + 1 << "/" << idx.uv + 1 << "/" << idx.normal + 1;
			}
			file << "\n";
		}
	}

	namespace
	{
		bool tryMergeQuad(
			const spk::ObjMesh::Quad &p_q1,
			std::size_t p_index,
			const std::vector<spk::ObjMesh::Shape> &p_shapes,
			std::vector<bool> &p_used,
			spk::CollisionMesh &p_result)
		{
			for (std::size_t j = p_index + 1; j < p_shapes.size(); ++j)
			{
				if (p_used[j] == true || std::holds_alternative<spk::ObjMesh::Quad>(p_shapes[j]) == false)
				{
					continue;
				}
				const auto &q2 = std::get<spk::ObjMesh::Quad>(p_shapes[j]);
				std::array<spk::Vector3, 8> verts = {
					p_q1.a.position, p_q1.b.position, p_q1.c.position, p_q1.d.position, q2.a.position, q2.b.position, q2.c.position, q2.d.position};

				std::vector<spk::Vector3> corners;
				for (const auto &v : verts)
				{
					int count = 0;
					for (const auto &u : verts)
					{
						if ((v == u) == true)
						{
							++count;
						}
					}
					if (count == 1)
					{
						corners.push_back(v);
					}
				}
				if (corners.size() == 4)
				{
					spk::Vector3 normal = (corners[1] - corners[0]).cross(corners[2] - corners[0]);
					if ((normal.norm() > 0.0f) == true)
					{
						spk::Vector3 center = (corners[0] + corners[1] + corners[2] + corners[3]) * 0.25f;
						spk::Vector3 axis1 = (corners[0] - center).normalize();
						spk::Vector3 axis2 = normal.normalize().cross(axis1);
						std::sort(
							corners.begin(),
							corners.end(),
							[&](const spk::Vector3 &p_lhs, const spk::Vector3 &p_rhs)
							{
								spk::Vector3 lv = p_lhs - center;
								spk::Vector3 rv = p_rhs - center;
								float angleL = std::atan2(lv.dot(axis2), lv.dot(axis1));
								float angleR = std::atan2(rv.dot(axis2), rv.dot(axis1));
								return (angleL < angleR) == true;
							});
						spk::CollisionMesh::Unit unit;
						unit.addShape(corners[0], corners[1], corners[2], corners[3]);
						p_result.addUnit(unit);
						p_used[j] = true;
						return true;
					}
				}
			}
			return false;
		}

		bool tryMergeTriangle(
			const spk::ObjMesh::Triangle &p_t1,
			std::size_t p_index,
			const std::vector<spk::ObjMesh::Shape> &p_shapes,
			std::vector<bool> &p_used,
			spk::CollisionMesh &p_result)
		{
			for (std::size_t j = p_index + 1; j < p_shapes.size(); ++j)
			{
				if (p_used[j] == true || std::holds_alternative<spk::ObjMesh::Triangle>(p_shapes[j]) == false)
				{
					continue;
				}
				const auto &t2 = std::get<spk::ObjMesh::Triangle>(p_shapes[j]);
				std::array<spk::Vector3, 3> v1 = {p_t1.a.position, p_t1.b.position, p_t1.c.position};
				std::array<spk::Vector3, 3> v2 = {t2.a.position, t2.b.position, t2.c.position};

				std::vector<spk::Vector3> shared;
				for (const auto &v : v1)
				{
					for (const auto &u : v2)
					{
						if ((v == u) == true)
						{
							shared.push_back(v);
						}
					}
				}
				if (shared.size() == 2)
				{
					std::vector<spk::Vector3> unique = {v1.begin(), v1.end()};
					for (const auto &v : v2)
					{
						if (std::find(unique.begin(), unique.end(), v) == unique.end())
						{
							unique.push_back(v);
						}
					}
					if (unique.size() == 4)
					{
						spk::CollisionMesh::Unit unit;
						unit.addShape(unique[0], unique[1], unique[2], unique[3]);
						p_result.addUnit(unit);
						p_used[j] = true;
						return true;
					}
				}
			}
			return false;
		}
	}

	spk::CollisionMesh ObjMesh::createCollider() const
	{
		spk::CollisionMesh result;
		const auto &shapeList = shapes();
		std::vector<bool> used(shapeList.size(), false);

		for (std::size_t i = 0; i < shapeList.size(); ++i)
		{
			if (used[i] == true)
			{
				continue;
			}

			if (std::holds_alternative<Quad>(shapeList[i]) == true)
			{
				const auto &q1 = std::get<Quad>(shapeList[i]);
				bool merged = tryMergeQuad(q1, i, shapeList, used, result);
				if (merged == false)
				{
					spk::CollisionMesh::Unit unit;
					unit.addShape(q1.a.position, q1.b.position, q1.c.position, q1.d.position);
					result.addUnit(unit);
				}
			}
			else
			{
				const auto &t1 = std::get<Triangle>(shapeList[i]);
				bool merged = tryMergeTriangle(t1, i, shapeList, used, result);
				if (merged == false)
				{
					spk::CollisionMesh::Unit unit;
					unit.addShape(t1.a.position, t1.b.position, t1.c.position);
					result.addUnit(unit);
				}
			}
			used[i] = true;
		}

		return result;
	}
}
