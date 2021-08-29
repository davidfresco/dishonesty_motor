#pragma once

#include <Python.h>
#include <vector>

#include "Vad.h"

struct PtrNode {
	unsigned long long address;
	unsigned long long offset;
	std::vector<PtrNode*> children;
};

void free_PtrNode(PtrNode* node) {
	if(!node->children.size()) delete node;
	else {
		for(PtrNode* next_node : node->children) free_PtrNode(next_node);
		delete node;
	}
}

int bin_search_vads(VAD* vads, int left, int right, unsigned long long target) {
	if(right < left) return -1;
	int mid = (right + left) / 2;
	if(target >= vads[mid].start && target < vads[mid].end) return mid;
	if(target < vads[mid].start) return bin_search_vads(vads, left, mid-1, target);
	if(target > vads[mid].end) return bin_search_vads(vads, mid+1, right, target);
	return -1;
}

PyObject* tree_to_pydict(PtrNode* node) {
	if(node->children.size() == 0) {
		return Py_BuildValue("");
	}
	PyObject* dict = PyDict_New();
	for(PtrNode* node : node->children) {
		PyDict_SetItem(dict, Py_BuildValue("K", node->offset), tree_to_pydict(node));
	}
	return dict;
}

void get_all_static_ptrs(
		unsigned int pid,
		VAD static_vad,
		VAD* vads,
		size_t num_vads,
		unsigned long long target,
		std::vector<PtrNode*>* valid_ptrs) {
	std::cout<<num_vads<<" vads found"<<std::endl;

	unsigned long long static_region_start = static_vad.start * 0x1000;
	unsigned long long static_region_size = (static_vad.end - static_vad.start) * 0x1000;
	unsigned char* static_bytes = (unsigned char*)malloc(static_region_size);
	std::cout<<"starting mem read of size "<<static_region_size<<std::endl;
	int status = Driver::read_memory(
			(uintptr_t)pid,
			(uintptr_t)(static_region_start),
			(uintptr_t)static_bytes,
			(size_t)static_region_size);
	std::cout<<"mem read completed"<<std::endl;

	for(unsigned long long offset = 0; offset < static_region_size; offset += 8) {
		unsigned long long ptr = *(unsigned long long*)(static_bytes + offset);
		if(!ptr) continue;
		if(bin_search_vads(vads, 0, num_vads-1, ptr) != -1) {
			PtrNode* node = new PtrNode;
			node->offset = static_region_start + offset;
			node->address = ptr;
			valid_ptrs->push_back(node);
		}
	}
	std::cout<<"found "<<valid_ptrs->size()<<" static ptrs"<<std::endl;
	free(static_bytes);
}

PtrNode* reverse_ptr_scan(
	unsigned int pid,
	unsigned long long target,
	PtrNode* parent,
	int depth,
	int max_depth,
	int max_struct_size,
	VAD* vads,
	size_t num_vads) {

	if(target - parent->address < max_struct_size && target - parent->address >= 0) {
		PtrNode* node = new PtrNode;
		node->address = target;
		node->offset = target-parent->address;
		parent->children.push_back(node);
		return parent;
	} else if(depth == max_depth) {
		return NULL;
	}

	unsigned long long* next_addresses = (unsigned long long*)malloc(max_struct_size);
	int status = Driver::read_memory(
			(uintptr_t)pid,
			(uintptr_t)parent->address,
			(uintptr_t)next_addresses,
			(uintptr_t)max_struct_size);
	if(status) return NULL;

	bool got_one = false;
	for(unsigned long long i = 0; i < max_struct_size / 8; i++) {
		if(bin_search_vads(vads, 0, num_vads-1, next_addresses[i]) != -1) {

			PtrNode* node = new PtrNode;
			node->offset = (i * 8);
			node->address = next_addresses[i];

			PtrNode* next_node = reverse_ptr_scan(
					pid,
					target,
					node,
					depth + 1,
					max_depth, max_struct_size, vads, num_vads);
			if(next_node) {
				got_one = true;
				parent->children.push_back(node);
			} else free_PtrNode(node);
		}
	}
	if(got_one) return parent;
	else return NULL;
}

void print_ptr_tree(PtrNode* node, int depth) {
	for(int i = 0; i < depth; i++) std::cout<<"\t";
	std::cout<<(void*)node->offset<<std::endl;
	if(node->children.size() == 0) {
		return;
	} else {
		for(PtrNode* next_node : node->children) {
			print_ptr_tree(next_node, depth + 1);
		}
	}
}

PyObject* scan_from_static(
		unsigned int pid,
		PyObject* vad_list,
		unsigned long long target,
		int max_depth,
		int max_struct_size) {
	
	//parse python vad list into cpp vad list
	size_t num_vads = PyList_GET_SIZE(vad_list) - 1;
	std::cout<<num_vads<<" vads found"<<std::endl;
	VAD static_vad = {
				PyLong_AsUnsignedLongLongMask(
						PyList_GET_ITEM(PyList_GET_ITEM(vad_list, num_vads), 0)),
				PyLong_AsUnsignedLongLongMask(
						PyList_GET_ITEM(PyList_GET_ITEM(vad_list, num_vads), 1))};
	VAD* vads = (VAD*)malloc((num_vads) * sizeof(VAD));
	for(size_t i = 0; i < num_vads; i++) {
		vads[i] = {
				PyLong_AsUnsignedLongLongMask(
						PyList_GET_ITEM(PyList_GET_ITEM(vad_list, i), 0))*0x1000,
				PyLong_AsUnsignedLongLongMask(
						PyList_GET_ITEM(PyList_GET_ITEM(vad_list, i), 1))*0x1000};
	}

	//get all valid ptrs from static memory
	std::vector<PtrNode*> static_ptrs;
	get_all_static_ptrs(pid, static_vad, vads, num_vads, target, &static_ptrs);

	PtrNode* root = new PtrNode;
	root->address = 0;
	root->offset = 0;
	for(size_t i = 0; i < static_ptrs.size(); i++) {
		PtrNode* node = reverse_ptr_scan(
				pid,
				target,
				static_ptrs.at(i),
				1,
				max_depth,
				max_struct_size,
				vads,
				num_vads);
		if(node) {
			root->children.push_back(node);
		}
	}
	std::cout<<"ptrs from "<<root->children.size()<<" static addresses"<<std::endl;
	PyObject* dict = PyDict_New();
	PyDict_SetItem(dict, Py_BuildValue("K", root->address), tree_to_pydict(root));

	free(vads);
	free_PtrNode(root);

	return dict;
}