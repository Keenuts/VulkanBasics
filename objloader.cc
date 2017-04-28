#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#include "objloader.hh"

typedef struct face {
	uint32_t vtx[3];
	uint32_t uv[3];
	uint32_t nrm[3];
} face_t;

struct raw_data {
	std::string name;
	std::vector<v3_t> vtx;
	std::vector<v3_t> nrm;
	std::vector<v2_t> uv;
	std::vector<struct face> face;
};

bool parse_obj(const char* path, struct raw_data* data) {
	if (!data)
		return false;

	std::string name = "Default";
	std::ifstream input;
	input.open(path);

	std::vector<v3_t> vtx = std::vector<v3_t>();
	std::vector<v3_t> nrm = std::vector<v3_t>();
	std::vector<v2_t> uv = std::vector<v2_t>();
	std::vector<struct face> face = std::vector<struct face>();

	bool reading = true;

	while (reading) {
		std::string head;
		input >> head;
		if (!strcmp(head.data(), "o"))
			input >> name;
		else if (!strcmp(head.data(), "v")) {
			v3_t v;
			input >> v.x;
			input >> v.y;
			input >> v.z;
			vtx.push_back(v);
		}
		else if (!strcmp(head.data(), "vt")) {
			v2_t v;
			input >> v.x;
			input >> v.y;
			uv.push_back(v);
		}
		else if (!strcmp(head.data(), "vn")) {
			v3_t n;
			input >> n.x;
			input >> n.y;
			input >> n.z;
			nrm.push_back(n);
		}
		else if (!strcmp(head.data(), "f")) {
			struct face f;
			for (uint32_t i = 0; i < 3; i++) {
				input >> f.vtx[i];
				input.get();
				input >> f.uv[i];
				input.get();
				input >> f.nrm[i];
			}
			face.push_back(f);
		}
		else if (head.size() < 1)
			break;
		else {
			printf("[ERROR] Invalid token %s\n", head.data());
			return false;
		}
	}

	data->name = name;
	data->vtx = vtx;
	data->nrm = nrm;
	data->uv = uv;
	data->face = face;

	printf("[INFO] Loading a mesh: %s\n", name.data());
	printf("[INFO] \t\t%zu vtx, %zu tris\n", vtx.size(), face.size());
	return true;
}

bool load_model(const char* path, model_t *model) {
	struct raw_data raw;
	
	if (!model)
		return false;
	if (!parse_obj(path, &raw))
		return false;
	
	model->count = raw.face.size() * 3;
	model->vertices = new vertex_t[model->count];
	
	uint32_t index = 0;
	for (face_t f : raw.face) {
		for (uint32_t i = 0; i < 3; i++) {
			vertex_t v;

			if (f.vtx[i] <= 0 || f.vtx[i] > raw.vtx.size()) {
				printf("[ERROR] Invalid vtx index: %u\n", f.vtx[i]);
				goto ERROR;
			}
			if (f.nrm[i] <= 0 || f.nrm[i] > raw.nrm.size()) {
				printf("[ERROR] Invalid nrm index: %u\n", f.vtx[i]);
				goto ERROR;
			}
			if (f.uv[i] <= 0 || f.uv[i] > raw.uv.size()) {
				printf("[ERROR] Invalid uv index: %u\n", f.vtx[i]);
				goto ERROR;
			}

			v.pos = raw.vtx[f.vtx[i] - 1];
			v.nrm = raw.nrm[f.nrm[i] - 1];
			v.uv = raw.uv[f.uv[i] - 1];
			model->vertices[index++] = v;
		}
	}

	return true;

ERROR:
	printf("[ERROR] Mesh %s: invalid vtx index\n", raw.name.data());
	delete model->vertices;
	return false;
}
