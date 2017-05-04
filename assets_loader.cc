#include "tiny_obj_loader.hh"
#include "assets_loader.hh"

bool load_model(const char* path, model_t *model) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path);
	if (!ret)
		return false;
	
	std::vector<vertex_t> vertices;
	
	for (tinyobj::shape_t& s : shapes) {
		for (tinyobj::index_t& idx : s.mesh.indices) {
			vertex_t v;
			v.pos = {
				attrib.vertices[3 * idx.vertex_index + 0],
				attrib.vertices[3 * idx.vertex_index + 1],
				attrib.vertices[3 * idx.vertex_index + 2]
			};

			if (attrib.normals.size() != 0) {
				v.nrm = {
					attrib.normals[3 * idx.normal_index + 0],
					attrib.normals[3 * idx.normal_index + 1],
					attrib.normals[3 * idx.normal_index + 2]
				};
			} else {
				v.nrm = { 0, 1, 0 };
			}

			if (attrib.texcoords.size() != 0) {
				v.uv = {
					attrib.texcoords[2 * idx.texcoord_index + 0],
					attrib.texcoords[2 * idx.texcoord_index + 1],
				};
			} else {
				v.uv = { 0, 0 };
			}
			vertices.push_back(v);
		}
	}

	printf("[INFO] Loading model %s [%zu vertices]\n", path, vertices.size());

	model->count = vertices.size();

	model->vertices = new vertex_t[model->count];
	if (model->vertices == NULL)
		return false;

	for (uint64_t i = 0; i < model->count; i++)
		model->vertices[i] = vertices[i];

	return true;
}
