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
	std::vector<v3_t> vtx;
	std::vector<v3_t> nrm;
	std::vector<v2_t> uv;
	std::vector<face_t> face;
};

bool parse_obj(const char* path, struct raw_data* data) {
	std::string name = "Default";
	std::ifstream input;
	input.open(path);

	std::vector<v3_t> vtx = std::vector<v3_t>();
	std::vector<v3_t> nrm = std::vector<v3_t>();
	std::vector<v2_t> uv = std::vector<v2_t>();
	std::vector<face_t> face = std::vector<face_t>();

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
			face_t f;
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
			return false;
		else
			break;
	}

	printf("[INFO] Loading a mesh: %s\n", name.data());
	printf("[INFO] \t\t%zu vtx, %zu tris\n", vtx.size(), face.size());
	return true;
}

bool load_model(const char* data) {
	return true;
}
