#pragma once

#include <Python.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <sstream>

#include "Driver.cpp"
#include "new_search.h"
#include "next_search.h"

#include "Vad.h"

void read_vad(
		unsigned int pid,
		unsigned long long* vad,
		const char* method,
		const char* var_type,
		unsigned int target_length,
		unsigned char* target,
		unsigned char* low,
		unsigned char* high,
		std::vector<SearchResult>* results) {
	int hits = 0;
	int valid_pages = 0;
	int (*search_function)(
			unsigned long long,
			unsigned char*,
			const char* var_type,
			unsigned int,
			unsigned char*,
			unsigned char*,
			unsigned char*,
			std::vector<SearchResult>*);
	if(strcmp(method, "Exact Value") == 0) search_function = &new_exact_value;
	// if(strcmp(method, "Less Than Value") == 0);
	// if(strcmp(method, "Greater Than Value") == 0);
	if(strcmp(method, "Value Between") == 0) search_function = &new_between_value;
	if(strcmp(method, "Unknown Initial Value") == 0) search_function = &new_unknown_value;

	for(unsigned long long page_num = vad[0]; page_num < vad[1]; page_num++) {
		unsigned char  bytes[PAGE_SIZE];
		int status = Driver::read_memory(
				(uintptr_t)pid,
				(uintptr_t)(page_num*PAGE_SIZE),
				(uintptr_t)bytes,
				(size_t)PAGE_SIZE);
		if(!status) {
			hits += (*search_function)(
					page_num,
					bytes,
					var_type,
					target_length,
					target,
					low,
					high,
					results);
			valid_pages++;
		}
	}
}

void next_search(
		unsigned int pid,
		const char* method,
		const char* var_type,
		unsigned int target_length,
		unsigned char* target,
		unsigned char* low,
		unsigned char* high,
		std::vector<std::vector<SearchResult>*>* all_results,
		std::vector<std::vector<SearchResult>*>* prev_results,
		VAD* vads,
		size_t num_vads) {
	for(int i = 0; i < prev_results->size(); i++) delete prev_results->at(i);
	prev_results->clear();
	unsigned long long total_bytes = 0;
	for(int i = 0; i < all_results->size(); i++) {
		prev_results->push_back(all_results->at(i));
		total_bytes += all_results->at(i)->size() * 12;
	}
	bool too_big = total_bytes > 250000000;
	std::cout<<total_bytes<<" bytes of data in the last search"<<std::endl;
	all_results->clear();
	void (*search_function)(
			unsigned long long,
			unsigned char*,
			const char*,
			unsigned int,
			unsigned char*,
			unsigned char*,
			unsigned char*,
			std::vector<SearchResult>*,
			std::vector<SearchResult>*);
	if(strcmp(method, "Exact Value") == 0) search_function = &next_exact_value;
	if(strcmp(method, "Less Than Value") == 0) search_function = &next_less_than;
	if(strcmp(method, "Greater Than Value") == 0) search_function = &next_greater_than;
	if(strcmp(method, "Value Between") == 0) search_function = &next_between_value;
	if(strcmp(method, "Increased Value") == 0) search_function = &next_increased_value;
	if(strcmp(method, "Decreased Value") == 0) search_function = &next_decreased_value;
	if(strcmp(method, "Changed Value") == 0) search_function = &next_changed_value;
	if(strcmp(method, "Unchanged Value") == 0) search_function = &next_unchanged_value;
	unsigned long long total_results = 0;
	unsigned long long last_page = 0x0;
	for(int i = 0; i < prev_results->size(); i++) {
		std::vector<SearchResult> vad_results = *prev_results->at(i);
		std::vector<SearchResult> page_results;
		std::vector<SearchResult>* new_vad_results = new std::vector<SearchResult>();
		for(int j = 0; j < vad_results.size(); j++) {
			unsigned long long page_num = (vad_results.at(j).address >> 12);
			if(page_num != last_page) { //we moved to the next page
				unsigned char bytes[PAGE_SIZE];
				int status = Driver::read_memory(
						(uintptr_t)pid,
						(uintptr_t)(last_page*PAGE_SIZE),
						(uintptr_t)bytes,
						(size_t)PAGE_SIZE);
				if(!status) (*search_function)(
						last_page*PAGE_SIZE,
						bytes,
						var_type,
						target_length,
						target,
						low,
						high,
						new_vad_results,
						&page_results);
				last_page = page_num;
				page_results.clear();
				total_results++;
			}
			page_results.push_back(vad_results.at(j));
		}
		unsigned char bytes[PAGE_SIZE];
		int status = Driver::read_memory(
				(uintptr_t)pid,
				(uintptr_t)(last_page*PAGE_SIZE),
				(uintptr_t)bytes,
				(size_t)PAGE_SIZE);
		if(!status) (*search_function)(
				last_page*PAGE_SIZE,
				bytes,
				var_type,
				target_length,
				target,
				low,
				high,
				new_vad_results,
				&page_results);
		all_results->push_back(new_vad_results);
		if(too_big) {
			vad_results.clear();
		}
	} 
}