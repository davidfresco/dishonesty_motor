#include <Python.h>
#include <iostream>
#include <string.h>

#include "search_funcs.h"
#include "ptr_funcs.h"

#include "Vad.h"

bool CheckDriverStatus() {
    int icheck = 82;
    NTSTATUS status = 0;

    uintptr_t BaseAddr = Driver::GetBaseAddress(GetCurrentProcessId());
    if (BaseAddr == 0) {
        return false;
    }

    int checked = Driver::read<int>(GetCurrentProcessId(), (uintptr_t)&icheck, &status);
    if (checked != icheck) {
        return false;
    }
    return true;
}

std::vector<std::vector<SearchResult>*> prev_results;
std::vector<std::vector<SearchResult>*> all_results;

static PyObject* get_results(PyObject* self, PyObject* args) {
	int target_length;
	if(!PyArg_ParseTuple(args, "I",
			&target_length)) return NULL;
	PyObject* all_py_results = PyList_New(0);
	for(int i = 0; i < all_results.size(); i++) {
		for(int j = 0; j < all_results.at(i)->size(); j++) {
			SearchResult result = all_results.at(i)->at(j);
			PyObject* py_result = PyList_New(0);
			PyList_Append(py_result, Py_BuildValue("K", result.address));
			PyList_Append(py_result, Py_BuildValue("y#", result.data, target_length));
			PyList_Append(all_py_results, py_result);
		}
	}
	return all_py_results;
}

unsigned long long print_size() {
	size_t num_bytes = 0;
	for(size_t i = 0; i < all_results.size(); i++) {
		num_bytes += all_results.at(i)->size() * sizeof(SearchResult);
	}
	return num_bytes;
}

static PyObject* read_all_mem(PyObject* self, PyObject* args) {
	unsigned int pid;
	PyObject* vad_list;
	PyObject* method_obj;
	PyObject* var_type_obj;
	unsigned int target_length;
	PyObject* target_obj;
	PyObject* low_obj;
	PyObject* high_obj;
	if(!PyArg_ParseTuple(args, "IOOOIOOO",
			&pid,
			&vad_list,
			&method_obj,
			&var_type_obj,
			&target_length,
			&target_obj,
			&low_obj,
			&high_obj)) return NULL;
	const char* method = PyUnicode_AsUTF8(method_obj); //search method string
	const char* var_type = PyUnicode_AsUTF8(var_type_obj);
	unsigned char* target = (unsigned char*)PyBytes_AsString((PyObject*) target_obj); //exact value target
	//bytes for gt/lt/between type searches
	unsigned char* low = (unsigned char*)PyBytes_AsString((PyObject*) low_obj);
	unsigned char* high = (unsigned char*)PyBytes_AsString((PyObject*) high_obj);

	size_t num_vads = PyList_GET_SIZE(vad_list);

	//free memory from previous searches
	for(size_t i = 0; i < all_results.size(); i++) delete all_results.at(i);
	for(size_t i = 0; i < prev_results.size(); i++) delete prev_results.at(i);
	all_results.clear();
	prev_results.clear();

	//for each vad...
	for(int i = 0; i < num_vads; i++) {
		unsigned long long vad_range[2] = { //get page range in vad
				PyLong_AsUnsignedLongLongMask(PyList_GET_ITEM(PyList_GET_ITEM(vad_list, i), 0)),
				PyLong_AsUnsignedLongLongMask(PyList_GET_ITEM(PyList_GET_ITEM(vad_list, i), 1))};
		if(!PyLong_AsUnsignedLongLongMask(PyList_GET_ITEM(PyList_GET_ITEM(vad_list, i), 4)))
			continue; //skip vads with commit size of 0

		//create results vector for vad then search it
		std::vector<SearchResult>* results = new std::vector<SearchResult>();
		read_vad(pid, vad_range, method, var_type, target_length, target, low, high, results);
		all_results.push_back(results);
	}

	//after all vads are searched, count up how many hits.
	size_t num_hits = 0;
	for(size_t i = 0; i < all_results.size(); i++) {
		num_hits += all_results.at(i)->size();
	}
	std::cout<<"bytes used: "<<print_size()<<std::endl;
	return Py_BuildValue("I", num_hits); //return number of results as argument to py
}

static PyObject* next_search(PyObject* self, PyObject* args) {
	unsigned int pid;
	PyObject* method_obj;
	PyObject* var_type_obj;
	unsigned int target_length;
	PyObject* target_obj;
	PyObject* low_obj;
	PyObject* high_obj;
	PyObject* vad_list;
	if(!PyArg_ParseTuple(args, "IOOIOOOO",
			&pid,
			&method_obj,
			&var_type_obj,
			&target_length,
			&target_obj,
			&low_obj,
			&high_obj,
			&vad_list)) return NULL;
	const char* method = PyUnicode_AsUTF8(method_obj);
	const char* var_type = PyUnicode_AsUTF8(var_type_obj);
	unsigned char* target = (unsigned char*)PyBytes_AsString((PyObject*) target_obj);
	unsigned char* low = (unsigned char*)PyBytes_AsString((PyObject*) low_obj);
	unsigned char* high = (unsigned char*)PyBytes_AsString((PyObject*) high_obj);

	size_t num_vads = PyList_GET_SIZE(vad_list);
	std::cout<<num_vads<<" vads found"<<std::endl;
	VAD* vads = (VAD*)malloc((num_vads) * sizeof(VAD));
	for(size_t i = 0; i < num_vads; i++) {
		vads[i] = {
				PyLong_AsUnsignedLongLongMask(
						PyList_GET_ITEM(PyList_GET_ITEM(vad_list, i), 0))*0x1000,
				PyLong_AsUnsignedLongLongMask(
						PyList_GET_ITEM(PyList_GET_ITEM(vad_list, i), 1))*0x1000};
	}

	next_search(
			pid,
			method,
			var_type,
			target_length,
			target,
			low,
			high,
			&all_results,
			&prev_results,
			vads,
			num_vads);

	size_t num_hits = 0;
	for(size_t i = 0; i < all_results.size(); i++) {
		num_hits += all_results.at(i)->size();
	}
	std::cout<<"bytes used: "<<print_size()<<std::endl;
	free(vads);
	return Py_BuildValue("I", num_hits);
}

static PyObject* ptr_scan_static(PyObject* self, PyObject* args) {
	unsigned int pid;
	PyObject* vad_list;
	PyObject* target_obj;
	int max_depth;
	int max_struct_size;
	if(!PyArg_ParseTuple(args, "IOOii",
			&pid,
			&vad_list,
			&target_obj,
			&max_depth,
			&max_struct_size)) return NULL;
	unsigned long long target = *(unsigned long long*)PyBytes_AsString((PyObject*) target_obj);
	max_struct_size = ((int)(max_struct_size / 8)) * 8;

	return scan_from_static(pid, vad_list, target, max_depth, max_struct_size);
}

static PyMethodDef funcs[] = {
	{"get_results", get_results, METH_VARARGS, "finds next exact"},
	{"read_all_mem", read_all_mem, METH_VARARGS, "finds next exact"},
	{"next_search", next_search, METH_VARARGS, "finds next exact"},
	// {"ptr_scan", ptr_scan, METH_VARARGS, "finds next exact"},
	{"ptr_scan_static", ptr_scan_static, METH_VARARGS, "finds next exact"},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef myModule = {
	PyModuleDef_HEAD_INIT,
	"c_lib",
	"c shit",
	-1,
	funcs
};

PyMODINIT_FUNC PyInit_c_lib(void) {
	if (!Driver::initialize() || !CheckDriverStatus()) {
        UNICODE_STRING VariableName = RTL_CONSTANT_STRING(VARIABLE_NAME);
        NtSetSystemEnvironmentValueEx(
            &VariableName,
            &DummyGuid,
            0,
            0,
            ATTRIBUTES);//delete var

        printf("search_utils: No EFI Driver found\n");
    }
	return PyModule_Create(&myModule);
}