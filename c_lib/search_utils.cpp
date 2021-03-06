#include <Python.h>
#include <iostream>
#include <string.h>

#include "search_funcs.h"
#include "ptr_funcs.h"
#include "sigscan.h"

#include "Vad.h"

std::vector<std::vector<SearchResult>*> prev_results;
std::vector<std::vector<SearchResult>*> all_results;


/*
 * returns all_results vector as an array of 2-element arrays. each 2-element array
 * is like [address, value] for a result. use this sparingly once you have narrowed
 * down a search considerably because python objects waste a crap ton of memory
 */
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


/*
 * prints the number of bytes used up by all_results so you can truly appreciate
 * the massive memory usage
 */
unsigned long long print_size() {
	size_t num_bytes = 0;
	for(size_t i = 0; i < all_results.size(); i++) {
		num_bytes += all_results.at(i)->size() * sizeof(SearchResult);
	}
	return num_bytes;
}


/*
 * this is the new_search function, it has a weird name because it was the first
 * thing in this library and i didnt know what i was doing yet.
 * it takes all the arguments, creates a list of memory page ranges, and searches
 * each page range individually. most of this should be done in another function
 * to keep this main page clean and organized, but whatever.
 */
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


/*
 * next_search function was made after i decided this whole thing shouldn't be in one
 * file, but its still pretty long just because of the python to c++ conversion stuff.
 */
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


/*
 * searches memory for a string of bytes, some of which may be unspecified
 * byte strings are like "AA BB CC ?? ?? DD EE ?? FF" where ?? are wildcard or
 * unknown bytes.
 *
 * this function is a slightly modified, copy-pasta'd version of the
 * read_all_mem/new_search function so theres a lot of junk in this one too
 */
static PyObject* sig_scan(PyObject* self, PyObject* args) {
	unsigned int pid;
	PyObject* vad_list;
	PyObject* py_sig_string;
	if(!PyArg_ParseTuple(args, "IOO",
			&pid,
			&vad_list,
			&py_sig_string)) return NULL;
	const char* sig_string = PyUnicode_AsUTF8(py_sig_string);
	int sig_string_length = PyUnicode_GetLength(py_sig_string);

	/*
	 * sig_bytes is what actually gets searched. each SigByte struct has a byte value
	 * and offset that describes the byte value at a specific point in the signature.
	 * this way we don't have to do any "??" comparisons for every byte while we search,
	 * because we can just check a list of defined bytes at known offsets into the
	 * signature. it runs really fast which is cool.
	 */
	SigByte* sig_bytes = (SigByte*)malloc(sig_string_length);
	int sig_length = 0;
	for(int i = 0; i < sig_string_length; i += 2) {
		if (sig_string[i] == '?' && sig_string[i + 1] == '?')
			continue;
		SigByte byte;
		byte.offset = (uchar)(i / 2);
		byte.value = (hex_to_bin(sig_string[i]) << 4) | hex_to_bin(sig_string[i + 1]);
		sig_bytes[sig_length++] = byte;
	}

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
		sig_scan_vad(pid, vad_range, sig_length, sig_bytes, results);
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


/*
 * probably the only well-segregated function here. finds pointer paths from static
 * memory up to a target memory address which is cool. returns a big ol python dict that
 * is a tree starting at memory address 0 where each node is an offset from the previous
 * address that is then traversed as a pointer
 */
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
	{"sig_scan", sig_scan, METH_VARARGS, "finds next exact"},
	{"ptr_scan_static", ptr_scan_static, METH_VARARGS, "finds next exact"},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef myModule = {
	PyModuleDef_HEAD_INIT,
	"c_lib",
	"c functions for searching lots and lots of memory",
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

    }
	return PyModule_Create(&myModule);
}