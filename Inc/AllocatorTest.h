#pragma once
#include "main.h"

enum IndexDir {
	fromStart,
	fromEnd
};

void Test_max_alloc(Allocator& allocator, size_t& alloc_chunks_counter, std::vector<char*>& allocated);
void Test_alloc_and_free(std::string test_name, IndexDir start_index);
void Test_alloc_and_free_through_one(std::string test_name, IndexDir start_index);
void Test_alloc_and_free_through_one_v2(std::string test_name, IndexDir start_index);