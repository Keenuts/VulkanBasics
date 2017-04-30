#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#include "objloader.hh"

struct face_t {
	uint32_t vtx[3];
	uint32_t uv[3];
	uint32_t nrm[3];
};

struct raw_data_t {
	std::string name;
	std::vector<v3_t> vtx;
	std::vector<v3_t> nrm;
	std::vector<v2_t> uv;
	std::vector<face_t> face;
};


static bool parse_vertex(std::stringstream &stream, std::vector<v3_t> *vtx) {
		std::string a, b, c;
		stream >> a;
		stream >> b;
		stream >> c;
		if (a.length() == 0 || b.length() == 0 || c.length() == 0)
			return false;

		v3_t v;
		v.x = std::stof(a);
		v.y = std::stof(b);
		v.z = std::stof(c);
		vtx->push_back(v);
		return true;
}

static bool parse_uv(std::stringstream &stream, std::vector<v2_t> *uv) {
		std::string a, b;
		stream >> a;
		stream >> b;
		if (a.length() == 0 || b.length() == 0)
			return false;

		v2_t v;
		v.x = std::stof(a);
		v.y = std::stof(b);
		uv->push_back(v);
		return true;
}

static bool parse_face(std::stringstream &stream, std::vector<face_t> *faces) {
	face_t f;
	for (uint32_t i = 0; i < 3; i++) {
		std::string v, vt, nrm;
		getline(stream, v, '/');
		getline(stream, vt, '/');
		getline(stream, nrm, ' ');

		if (v.length() == 0 || vt.length() == 0 || nrm.length() == 0)
			return false;

		f.vtx[i] = std::stoi(v);
		f.uv[i] = std::stoi(vt);
		f.nrm[i] = std::stoi(nrm);
	}
	faces->push_back(f);
	return true;
}

bool parse_obj(const char* path, raw_data_t *data) {
	if (!data)
		return false;

	std::string name = "Default";
	std::ifstream input(path);

	std::vector<v3_t> vtx = std::vector<v3_t>();
	std::vector<v3_t> nrm = std::vector<v3_t>();
	std::vector<v2_t> uv = std::vector<v2_t>();
	std::vector<face_t> face = std::vector<face_t>();

	std::string line;
	while (std::getline(input, line)) {
		std::stringstream in(line);
		std::string head;

		in >> head;
		if (head == "#" || head == "")
				continue;
		else if (head == "v") {
				if (!parse_vertex(in, &vtx)) { return false; }
		}
		else if (head == "vn") {
				if (!parse_vertex(in, &nrm)) { return false; }
		}
		else if (head == "vt") {
				if (!parse_uv(in, &uv)) { return false; }
		}
		else if (head == "f") {
				if (!parse_face(in, &face)) { return false; }
		}
		else if (head == "o")
			getline(in, name);
		else if (head == "g")
		{ }	//NON IMPLEMENTED
		else if (head == "usemlt")
		{ }	//NON IMPLEMENTED
		else {
				printf("[ERROR] Unknown head: %s\n", head.data());
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
	raw_data_t raw;
	
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
