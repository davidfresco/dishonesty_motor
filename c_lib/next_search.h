#include <vector>

#include "SearchResult.h"

void next_exact_value(
		unsigned long long page_base,
		unsigned char* bytes,
		const char* var_type,
		unsigned int target_length,
		unsigned char* target,
		unsigned char* low,
		unsigned char* high,
		std::vector<SearchResult>* new_vad_results,
		std::vector<SearchResult>* page_results) {
	for(int i = 0; i < page_results->size(); i++) {
		unsigned long long offset = (page_results->at(i).address - page_base);
		unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + offset);
		if(memcmp(mem_segment, target, target_length) == 0) {
			unsigned long long addr = page_base + offset;
			SearchResult result;
			result.address = addr;
			memcpy(result.data, mem_segment, target_length);
			new_vad_results->push_back(result);
		}
	}
}

void next_greater_than(
		unsigned long long page_base,
		unsigned char* bytes,
		const char* var_type,
		unsigned int target_length,
		unsigned char* target,
		unsigned char* low,
		unsigned char* high,
		std::vector<SearchResult>* new_vad_results,
		std::vector<SearchResult>* page_results) {
	for(int i = 0; i < page_results->size(); i++) {
		unsigned long long offset = (page_results->at(i).address - page_base);
		unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + offset);
		if(memcmp(mem_segment, target, target_length) > 0) {
			unsigned long long addr = page_base + offset;
			SearchResult result;
			result.address = addr;
			memcpy(result.data, mem_segment, target_length);
			new_vad_results->push_back(result);
		}
	}
}

void next_less_than(
		unsigned long long page_base,
		unsigned char* bytes,
		const char* var_type,
		unsigned int target_length,
		unsigned char* target,
		unsigned char* low,
		unsigned char* high,
		std::vector<SearchResult>* new_vad_results,
		std::vector<SearchResult>* page_results) {
	for(int i = 0; i < page_results->size(); i++) {
		unsigned long long offset = (page_results->at(i).address - page_base);
		unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + offset);
		if(memcmp(mem_segment, target, target_length) < 0) {
			unsigned long long addr = page_base + offset;
			SearchResult result;
			result.address = addr;
			memcpy(result.data, mem_segment, target_length);
			new_vad_results->push_back(result);
		}
	}
}

void next_between_value(
		unsigned long long page_base,
		unsigned char* bytes,
		const char* var_type,
		unsigned int target_length,
		unsigned char* target,
		unsigned char* low,
		unsigned char* high,
		std::vector<SearchResult>* new_vad_results,
		std::vector<SearchResult>* page_results) {
	if(strcmp(var_type, "INT") == 0) {
		for(size_t i = 0; i < page_results->size(); i++) {
			unsigned long long offset = (page_results->at(i).address - page_base);
			unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + offset);
			unsigned char* prev = page_results->at(i).data;
			if(*(int*)mem_segment > *(int*)low &&*(int*)mem_segment < *(int*)high) {
				unsigned long long addr = page_base + offset;
				SearchResult result;
				result.address = addr;
				memcpy(result.data, mem_segment, target_length);
				new_vad_results->push_back(result);
			}
		}
	} else if(strcmp(var_type, "FLOAT") == 0) {
		for(size_t i = 0; i < page_results->size(); i++) {
			unsigned long long offset = (page_results->at(i).address - page_base);
			unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + offset);
			unsigned char* prev = page_results->at(i).data;
			if(*(float*)mem_segment > *(float*)low &&*(float*)mem_segment < *(float*)high) {
				unsigned long long addr = page_base + offset;
				SearchResult result;
				result.address = addr;
				memcpy(result.data, mem_segment, target_length);
				new_vad_results->push_back(result);
			}
		}
	} else {
		for(size_t i = 0; i < page_results->size(); i++) {
			unsigned long long offset = (page_results->at(i).address - page_base);
			unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + offset);
			unsigned char* prev = page_results->at(i).data;
			if(memcmp(mem_segment, prev, target_length) > 0) {
				unsigned long long addr = page_base + offset;
				SearchResult result;
				result.address = addr;
				memcpy(result.data, mem_segment, target_length);
				new_vad_results->push_back(result);
			}
		}
	}
}

void next_increased_value(
		unsigned long long page_base,
		unsigned char* bytes,
		const char* var_type,
		unsigned int target_length,
		unsigned char* target,
		unsigned char* low,
		unsigned char* high,
		std::vector<SearchResult>* new_vad_results,
		std::vector<SearchResult>* page_results) {
	if(strcmp(var_type, "INT") == 0) {
		for(size_t i = 0; i < page_results->size(); i++) {
			unsigned long long offset = (page_results->at(i).address - page_base);
			unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + offset);
			unsigned char* prev = page_results->at(i).data;
			if(*(int*)mem_segment > *(int*)prev) {
				unsigned long long addr = page_base + offset;
				SearchResult result;
				result.address = addr;
				memcpy(result.data, mem_segment, target_length);
				new_vad_results->push_back(result);
			}
		}
	} else if(strcmp(var_type, "FLOAT") == 0) {
		for(size_t i = 0; i < page_results->size(); i++) {
			unsigned long long offset = (page_results->at(i).address - page_base);
			unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + offset);
			unsigned char* prev = page_results->at(i).data;
			if(*(float*)mem_segment > *(float*)prev) {
				unsigned long long addr = page_base + offset;
				SearchResult result;
				result.address = addr;
				memcpy(result.data, mem_segment, target_length);
				new_vad_results->push_back(result);
			}
		}
	} else {
		for(size_t i = 0; i < page_results->size(); i++) {
			unsigned long long offset = (page_results->at(i).address - page_base);
			unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + offset);
			unsigned char* prev = page_results->at(i).data;
			if(memcmp(mem_segment, prev, target_length) > 0) {
				unsigned long long addr = page_base + offset;
				SearchResult result;
				result.address = addr;
				memcpy(result.data, mem_segment, target_length);
				new_vad_results->push_back(result);
			}
		}
	}
}

void next_decreased_value(
		unsigned long long page_base,
		unsigned char* bytes,
		const char* var_type,
		unsigned int target_length,
		unsigned char* target,
		unsigned char* low,
		unsigned char* high,
		std::vector<SearchResult>* new_vad_results,
		std::vector<SearchResult>* page_results) {
	if(strcmp(var_type, "INT") == 0) {
		for(size_t i = 0; i < page_results->size(); i++) {
			unsigned long long offset = (page_results->at(i).address - page_base);
			unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + offset);
			unsigned char* prev = page_results->at(i).data;
			if(*(int*)mem_segment < *(int*)prev) {
				unsigned long long addr = page_base + offset;
				SearchResult result;
				result.address = addr;
				memcpy(result.data, mem_segment, target_length);
				new_vad_results->push_back(result);
			}
		}
	} else if(strcmp(var_type, "FLOAT") == 0) {
		for(size_t i = 0; i < page_results->size(); i++) {
			unsigned long long offset = (page_results->at(i).address - page_base);
			unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + offset);
			unsigned char* prev = page_results->at(i).data;
			if(*(float*)mem_segment < *(float*)prev) {
				unsigned long long addr = page_base + offset;
				SearchResult result;
				result.address = addr;
				memcpy(result.data, mem_segment, target_length);
				new_vad_results->push_back(result);
			}
		}
	} else {
		for(size_t i = 0; i < page_results->size(); i++) {
			unsigned long long offset = (page_results->at(i).address - page_base);
			unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + offset);
			unsigned char* prev = page_results->at(i).data;
			if(memcmp(mem_segment, prev, target_length) < 0) {
				unsigned long long addr = page_base + offset;
				SearchResult result;
				result.address = addr;
				memcpy(result.data, mem_segment, target_length);
				new_vad_results->push_back(result);
			}
		}
	}
}

void next_unchanged_value(
		unsigned long long page_base,
		unsigned char* bytes,
		const char* var_type,
		unsigned int target_length,
		unsigned char* target,
		unsigned char* low,
		unsigned char* high,
		std::vector<SearchResult>* new_vad_results,
		std::vector<SearchResult>* page_results) {
	for(int i = 0; i < page_results->size(); i++) {
		unsigned long long offset = (page_results->at(i).address - page_base);
		unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + offset);
		unsigned char* prev = page_results->at(i).data;
		if(memcmp(mem_segment, prev, target_length) == 0) {
			unsigned long long addr = page_base + offset;
			SearchResult result;
			result.address = addr;
			memcpy(result.data, mem_segment, target_length);
			new_vad_results->push_back(result);
		}
	}
}

void next_changed_value(
		unsigned long long page_base,
		unsigned char* bytes,
		const char* var_type,
		unsigned int target_length,
		unsigned char* target,
		unsigned char* low,
		unsigned char* high,
		std::vector<SearchResult>* new_vad_results,
		std::vector<SearchResult>* page_results) {
	for(int i = 0; i < page_results->size(); i++) {
		unsigned long long offset = (page_results->at(i).address - page_base);
		unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + offset);
		unsigned char* prev = page_results->at(i).data;
		if(memcmp(mem_segment, prev, target_length) != 0) {
			unsigned long long addr = page_base + offset;
			SearchResult result;
			result.address = addr;
			memcpy(result.data, mem_segment, target_length);
			new_vad_results->push_back(result);
		}
	}
}