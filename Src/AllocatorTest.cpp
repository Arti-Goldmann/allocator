#include "main.h"

void Test_max_alloc(Allocator& allocator, size_t& alloc_chunks_counter, std::vector<char*>& allocated){
	
    while(allocator.get_free_chunks_num() != 0) {
		char* tmp = (char*)allocator.m_alloc_chunk();

		//Проверим выделенный адрес
		size_t start_adress = allocator.get_heap_start_adress();
		size_t expected_adress = start_adress + Allocator::CHUNK_INFO_BYTES + alloc_chunks_counter * Allocator::TOTAL_CHUNK_BYTES;
		assert(expected_adress == (size_t)tmp);

		allocated.push_back(tmp);
		alloc_chunks_counter++;

		//Проверим, что кол-во свободных и выделенных блоков сходится
		assert(alloc_chunks_counter == allocator.get_chunks_num_in_use());
		assert((allocator.get_total_heap_size() / Allocator::TOTAL_CHUNK_BYTES - alloc_chunks_counter) == allocator.get_free_chunks_num());
	}

	//Проверим, что если попытаемся выделить еще блок, то получим NULL
	assert((char*)allocator.m_alloc_chunk() == NULL);
	//Проверим, что в списке свободных блоков нет
	assert(allocator.get_size_of_free_list() == 0);
}

void Test_alloc_and_free(std::string test_name, Index start_index){

	std::cout << test_name << " start" << std::endl;
	Allocator allocator = Allocator();
	std::vector<char*> allocated = {};

	size_t alloc_chunks_counter = 0;
	//Выделим столько блоков, сколько сможем и проверим
	Test_max_alloc(allocator, alloc_chunks_counter, allocated);

	//Будем освобождать с начала или с конца
	size_t index = 0;
	if(start_index == fromStart)    index = 0;
	else if(start_index == fromEnd) index = alloc_chunks_counter - 1;
		
	while(alloc_chunks_counter > 0){
		allocator.m_free(allocated[index]);

		if(start_index == fromStart)    index++;
		else if(start_index == fromEnd) index--;

		alloc_chunks_counter--;

		//Проверим, что кол-во свободных и выделенных блоков сходится
		assert(alloc_chunks_counter == allocator.get_chunks_num_in_use());
		assert((allocator.get_total_heap_size() / Allocator::TOTAL_CHUNK_BYTES - alloc_chunks_counter) == allocator.get_free_chunks_num());
		//Проверим, что всего только одна непрерывная свободная область памяти
		assert(allocator.get_size_of_free_list() == 1);
	}

	//Вся память должна быть освобождена
	assert(allocator.get_total_heap_size() / Allocator::TOTAL_CHUNK_BYTES == allocator.get_free_chunks_num());
	assert(allocator.get_chunks_num_in_use() == 0);

	std::cout << test_name <<" passed" << std::endl;
}

void Test_alloc_and_free_through_one(std::string test_name, Index start_index){
	
	std::cout << test_name << " start" << std::endl;
	Allocator allocator = Allocator();
	std::vector<char*> allocated = {};

	size_t alloc_chunks_counter = 0;
	//Выделим столько блоков, сколько сможем и проверим
	Test_max_alloc(allocator, alloc_chunks_counter, allocated);

	size_t num_of_chunks = alloc_chunks_counter;
	size_t index = 0;
	size_t expected_size_of_list = 0;
	//Переменная, чтобы можно было поменять направление изменение индекса
	size_t index_direction = 0; 
	
	//Будем освобождать сначала четные блоки, потом нечетные
	//Начиная с начала или с конца
	for(size_t offset = 0; offset <= 1; offset++){

		if(start_index == fromStart) {
			index = offset;
			index_direction = 0;
		}
		else if(start_index == fromEnd) {
			index = num_of_chunks - 1 - offset;
			index_direction = num_of_chunks - 1;
		}

		while(abs((long long)index_direction - (long long)index) < num_of_chunks){
			
			allocator.m_free(allocated[index]);
			
			if(start_index == fromStart)
				index += 2;
			else if(start_index == fromEnd)
				index -= 2;

			alloc_chunks_counter--;
			//Пока мы освобождаем четные блоки, размер списка отдельных свободных блоков увеличивается
			if(offset == 0)
				expected_size_of_list++;
			//Когда начинаем освобождать нечетные блоки, то свободные области начнут соединяться
			//И их кол-во будет уменьшаться, но минимальное кол-во будет равно 1
			else if(offset == 1){
				expected_size_of_list--;
				if(expected_size_of_list < 1) expected_size_of_list = 1;
			}

			assert(expected_size_of_list == allocator.get_size_of_free_list());

			//Проверим, что кол-во свободных и выделенных блоков сходится
			assert(alloc_chunks_counter == allocator.get_chunks_num_in_use());
			assert((allocator.get_total_heap_size() / Allocator::TOTAL_CHUNK_BYTES - alloc_chunks_counter) == allocator.get_free_chunks_num());
		}
	}    

	//Вся память должна быть освобождена
	assert(allocator.get_total_heap_size() / Allocator::TOTAL_CHUNK_BYTES == allocator.get_free_chunks_num());
	assert(allocator.get_chunks_num_in_use() == 0);

	std::cout << test_name <<" passed" << std::endl;
}