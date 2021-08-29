#include <vector>

#include "SearchResult.h"

const unsigned long long PAGE_SIZE = 0x1000;

int new_exact_value(
		unsigned long long page_base,
		unsigned char* bytes,
		const char* var_type,
		unsigned int target_length,
		unsigned char* target,
		unsigned char* low,
		unsigned char* high,
		std::vector<SearchResult>* results) {
	int hits = 0;
	for(unsigned long long i = 0; i < PAGE_SIZE; i += target_length) {
		unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + i);
		if(memcmp(mem_segment, target, target_length) == 0) {
			unsigned long long addr = page_base*PAGE_SIZE + i;
			SearchResult result;
			result.address = addr;
			memcpy(result.data, mem_segment, target_length);
			results->push_back(result);
			hits++;
		}
	}
	return hits;
}

int new_unknown_value(
		unsigned long long page_base,
		unsigned char* bytes,
		const char* var_type,
		unsigned int target_length,
		unsigned char* target,
		unsigned char* low,
		unsigned char* high,
		std::vector<SearchResult>* results) {
	int hits = 0;
	for(unsigned long long i = 0; i < PAGE_SIZE; i += target_length) {
		unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + i);
		unsigned long long addr = page_base*PAGE_SIZE + i;
		SearchResult result;
		result.address = addr;
		memcpy(result.data, mem_segment, target_length);
		results->push_back(result);
		hits++;
	}
	return hits;
}

int new_between_value(
		unsigned long long page_base,
		unsigned char* bytes,
		const char* var_type,
		unsigned int target_length,
		unsigned char* target,
		unsigned char* low,
		unsigned char* high,
		std::vector<SearchResult>* results) {
	int hits = 0;
	
	if(strcmp(var_type, "INT") == 0) {
		for(unsigned long long i = 0; i < PAGE_SIZE; i += target_length) {
			unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + i);
			if(*(int*)mem_segment < *(int*)high && *(int*)mem_segment > *(int*)low) {
				unsigned long long addr = page_base*PAGE_SIZE + i;
				SearchResult result;
				result.address = addr;
				memcpy(result.data, mem_segment, target_length);
				results->push_back(result);
				hits++;
			}
		}
	} else if(strcmp(var_type, "FLOAT") == 0) {
		for(unsigned long long i = 0; i < PAGE_SIZE; i += target_length) {
			unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + i);
			if(*(float*)mem_segment < *(float*)high && *(float*)mem_segment > *(float*)low) {
				unsigned long long addr = page_base*PAGE_SIZE + i;
				SearchResult result;
				result.address = addr;
				memcpy(result.data, mem_segment, target_length);
				results->push_back(result);
				hits++;
			}
		}
	} else {
		for(unsigned long long i = 0; i < PAGE_SIZE; i += target_length) {
		unsigned char* mem_segment = (unsigned char*)(((unsigned long long)bytes) + i);
			if(		memcmp(mem_segment, low, target_length) > 0 &&
					memcmp(mem_segment, high, target_length) < 0) {
				unsigned long long addr = page_base*PAGE_SIZE + i;
				SearchResult result;
				result.address = addr;
				memcpy(result.data, mem_segment, target_length);
				results->push_back(result);
				hits++;
			}
		}
	}
	return hits;
}