
#include "SearchResult.h"

#define PAGE_SIZE 0x1000

typedef unsigned char uchar;

typedef struct _sig_byte {
	uchar offset;
	uchar value;
} SigByte;

char hex_to_bin(uchar c) {
	uchar byte = 0;
	if (c >= 48 && c <= 57) //digit
		byte = byte | (c - 48);
	if (c >= 65 && c <=70) //letter
		byte = byte | (c - 55);
	return byte;
}

int search_page_for_sig(
		unsigned long long page_num,
		uchar* page_data,
		SigByte* sig_bytes,
		int sig_length,
		std::vector<SearchResult>* results)
{
	int hits = 0;
	for(unsigned long long offset = 0; offset < PAGE_SIZE; offset++) {
		bool found = true;
		for(int i = 0; i < sig_length; i++) {
			uchar sig_offset = sig_bytes[i].offset;
			if(page_data[offset + sig_offset] != sig_bytes[i].value) {
				found = false;
				break;
			}
		}
		if(found) {
			SearchResult result;
			result.address = (page_num * PAGE_SIZE) + offset;
			results->push_back(result);
			hits++;
		}
	}
	return hits;
}

void sig_scan_vad(
		int pid,
		unsigned long long* vad_range,
		int sig_length,
		SigByte* sig_bytes,
		std::vector<SearchResult>* results)
{
	int hits = 0;
	int valid_pages = 0;

	int extra_bytes = sig_bytes[sig_length-1].offset;
	for(unsigned long long page_num = vad_range[0]; page_num < vad_range[1]; page_num++) {
		uchar* bytes = (uchar*)malloc(PAGE_SIZE+extra_bytes);
		int status = Driver::read_memory(
				(uintptr_t)pid,
				(uintptr_t)(page_num*PAGE_SIZE),
				(uintptr_t)bytes,
				(size_t)PAGE_SIZE+extra_bytes);
		if(!status) {
			hits += search_page_for_sig(
					page_num,
					bytes,
					sig_bytes,
					sig_length,
					results);
			valid_pages++;
		}
		free(bytes);
	}
}