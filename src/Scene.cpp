#include "Scene.hpp"

Scene::Scene()
{
	m_Camera = Camera();
}

Scene::Scene(Camera camera)
{
	m_Camera = camera;
}

Scene::Scene(Scene&& other) noexcept
{
	m_Camera = std::move(other.m_Camera);

	vertexBuffer = std::move(other.vertexBuffer);
	indexBuffer = std::move(other.indexBuffer);
	primitives = std::move(other.primitives);
	textures = std::move(other.textures);
}

Scene::~Scene()
{
	// Clean up resources
	for (const auto& elem : textures)
		delete elem.second;
}

Scene& Scene::operator=(Scene&& other) noexcept
{
	// Clean up resources
	for (const auto& elem : textures)
		delete elem.second;

	m_Camera = std::move(other.m_Camera);

	vertexBuffer = std::move(other.vertexBuffer);
	indexBuffer = std::move(other.indexBuffer);
	primitives = std::move(other.primitives);
	textures = std::move(other.textures);
	return *this;
}

void Scene::LoadObject(std::string_view fileName)
{
	tinyobj::attrib_t attribs;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err = "";

	bool isLoaded = tinyobj::LoadObj(&attribs, &shapes, &materials, nullptr, &err, fileName.data(), "../assets/", true /*triangulate*/, true /*default_vcols_fallback*/);
	if (isLoaded)
	{
		//Get all materials
		for (size_t i = 0; i < materials.size(); i++)
		{
			const tinyobj::material_t& mat = materials[i];

			std::string_view diffuseTexName = mat.diffuse_texname;
			assert(!diffuseTexName.empty() && "Mesh missing texture!");

			if (textures.find(diffuseTexName.data()) == textures.end())
			{
				std::string fileName = std::string("../assets/") + diffuseTexName.data();

				Texture* pAlbedo = new Texture();
				pAlbedo->data = stbi_load(fileName.data(), &pAlbedo->width, &pAlbedo->height, &pAlbedo->numChannels, 0);
				assert(pAlbedo->data != nullptr && "Failed to load image!");

				//Set textures
				textures[diffuseTexName.data()] = pAlbedo;
			}
		}
		std::map<IndexedPrimitive, std::uint32_t> indexedPrims;
		for (size_t shapeIndex = 0; shapeIndex < shapes.size(); shapeIndex++)
		{
			const tinyobj::shape_t& currentShape = shapes[shapeIndex];

			std::uint32_t meshIdxBase = indexBuffer.size();
			for (size_t i = 0; i < currentShape.mesh.indices.size(); i++)
			{
				auto index = currentShape.mesh.indices[i];

				IndexedPrimitive prim;

				//Set vertex index if existing
				int vtxIdx = index.vertex_index;
				assert(vtxIdx != -1);
				prim.posIdx = vtxIdx;

				//Set Normal index if existing
				bool hasNormals = index.normal_index != -1;
				int normalIdx = index.normal_index;
				prim.normalIdx = hasNormals ? normalIdx : UINT32_MAX;

				//Set UV index if existing
				bool hasUV = index.texcoord_index != -1;
				int uvIdx = index.texcoord_index;
				prim.uvIdx = hasUV ? uvIdx : UINT32_MAX;


				auto res = indexedPrims.find(prim);
				// Vertex is already defined in terms of POS/NORMAL/UV indices, just append index data to index buffer
				if (res != indexedPrims.end())
				{
					indexBuffer.push_back(res->second);
				}
				// New unique vertex found, get vertex data and append it to vertex buffer and update indexed primitives
				else
				{
					auto newIdx = vertexBuffer.size();
					indexedPrims[prim] = newIdx;
					indexBuffer.push_back(newIdx);


					auto vx = attribs.vertices[3 * index.vertex_index];
					auto vy = attribs.vertices[3 * index.vertex_index + 1];
					auto vz = attribs.vertices[3 * index.vertex_index + 2];
					glm::vec3 pos(vx, vy, vz);

					glm::vec3 normal(0.f);
					if (hasNormals)
					{
						auto nx = attribs.normals[3 * index.normal_index];
						auto ny = attribs.normals[3 * index.normal_index + 1];
						auto nz = attribs.normals[3 * index.normal_index + 2];

						normal.x = nx;
						normal.y = ny;
						normal.z = nz;
					}

					glm::vec2 uv(0.f);
					if (hasUV)
					{
						auto ux = attribs.texcoords[2 * index.texcoord_index];
						auto uy = 1.f - attribs.texcoords[2 * index.texcoord_index + 1];

						uv.s = glm::abs(ux);
						uv.t = glm::abs(uy);
					}

					VertexInput uniqueVertex = { pos, normal, uv };
					vertexBuffer.push_back(uniqueVertex);
				}
			}
				// Push new mesh to be rendered in the scene 
				Mesh mesh;
				mesh.idxOffset = meshIdxBase;
				mesh.idxCount = currentShape.mesh.indices.size();

				assert((shapes[shapeIndex].mesh.material_ids[0] != -1) && "Mesh missing a material!");
				mesh.diffuseTexName = materials[currentShape.mesh.material_ids[0]].diffuse_texname; // No per-face material but fixed one
				primitives.push_back(mesh);

		}
		//std::cout << "Obj " << fileName << " loaded\n";
	}
	else
	{
		std::cout << "ERROR " << err << "\n";
		assert(false && "Failed to load .OBJ file, check file paths!");
	}

}
