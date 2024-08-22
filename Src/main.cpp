#include "Allocator.h"
#include <iostream>
#include <cassert>
#include <vector>


void vTaskSuspendAll() {
	//Нужно удалить если подключен FreeRTOS
};

void xTaskResumeAll() {
	//Нужно удалить если подключен FreeRTOS
}

void Test1(){

	std::cout <<"TEST 1 start" << std::endl;
	Allocator allocator = Allocator();

	//Тест 1:
	//Проверим какой максимальный доступный размер у кучи
	//В процессе инициализации границы кучи смещаются поэтому доступный размер может быть не равен заданному, 
	//Куча разделена строго на N ячеек размером 2 x sizeof(ChunkNodeInfo_t):
	
	size_t expected_heap_size = Allocator::config::SIZE_OF_HEAP_BYTES / (2 * sizeof(Allocator::ChunkNodeInfo_t));
	size_t real_heap_size = allocator.get_total_heapSize() / (2 * sizeof(Allocator::ChunkNodeInfo_t));
	assert(expected_heap_size == real_heap_size);

	//Какую-то часть байт могли отбросить
	size_t expected_reminder = Allocator::config::SIZE_OF_HEAP_BYTES % (2 * sizeof(Allocator::ChunkNodeInfo_t));
	size_t real_reminder = Allocator::config::SIZE_OF_HEAP_BYTES - allocator.get_total_heapSize(); 
	assert(expected_reminder == real_reminder);

	//Причем после инициализации остатка в куче быть не должно
	size_t reminder_heap_size = allocator.get_total_heapSize() % (2 * sizeof(Allocator::ChunkNodeInfo_t));
	assert(reminder_heap_size == 0);

	std::cout <<"TEST 1 passed" << std::endl;
}

void Test2(){

	std::cout <<"TEST 2 start" << std::endl;
	Allocator allocator = Allocator();
	const char* ptr1;
	//Тест 2:
	//Выделим и осводим один блок памяти
	size_t ptr1_bytes = 1;
	size_t ChunkInfo = sizeof(Allocator::ChunkNodeInfo_t);

	//Можем выделить сколько есть кучи и минус структура с инфой
	while(ptr1_bytes <= (allocator.get_total_heapSize() - ChunkInfo)){

		//Минимальный блок который может быть выделен: 2 x sizeof(ChunkNodeInfo_t)
		//Поэтому по факту может быть выделено больше, чем пользователь просит
		//Выделяемый блок всегда будет кратен 2 x sizeof(ChunkNodeInfo_t) и будет увеличен, если пользователь хочет меньше
		//одна половина байт под структуру с информацией
		//вторая половина байт пользователю
		ptr1 = (char*)allocator.m_malloc(ptr1_bytes);

		//Прибаляем к пользовательски байтам структуру с информацией
		size_t expected_total_busy_bytes = ptr1_bytes + ChunkInfo;
		if(expected_total_busy_bytes % (2 * ChunkInfo)){
			//Если получишийся блок не кратен 2xChunkInfo, то он расширяется так, чтобы был кратен
			expected_total_busy_bytes -= expected_total_busy_bytes % (2 * ChunkInfo);
			expected_total_busy_bytes += 2 * ChunkInfo;
		}

		size_t real_total_busy_bytes = allocator.get_total_bytes_in_use();
		assert(expected_total_busy_bytes == real_total_busy_bytes);

		allocator.m_free((void*)ptr1);
		ptr1_bytes++;
	}

	//Вся память должна быть освобождена
	assert(allocator.get_total_heapSize() == allocator.get_total_free_bytes());
	assert(allocator.get_total_bytes_in_use() == 0);

	std::cout <<"TEST 2 passed" << std::endl;
}

void Test3(){

	std::cout <<"TEST 3 start" << std::endl;
	Allocator allocator = Allocator();

	size_t ChunkInfo  = sizeof(Allocator::ChunkNodeInfo_t);
	size_t alloc_size = ChunkInfo; //Ячеки будут размером ChunkInfo

	std::vector<char*> allocated = {};

	size_t alloc_counter = 0;
	//Выделим выделим столько памяти сколько сможем, а потом будем ее освобождать с начала
	while(allocator.get_total_free_bytes() != 0) {
		char* tmp = (char*)allocator.m_malloc(alloc_size);
		allocated.push_back(tmp);
		alloc_counter++;

		size_t alloc_bytes_expected = alloc_counter * ChunkInfo * 2;
		assert(alloc_bytes_expected == allocator.get_total_bytes_in_use());
		assert((allocator.get_total_heapSize() - alloc_bytes_expected) == allocator.get_total_free_bytes());
	}

	size_t index = 0;

	while(alloc_counter > 0){
		allocator.m_free(allocated[index]);
		index++;
		alloc_counter--;

		size_t alloc_bytes_expected = alloc_counter * ChunkInfo * 2;
		assert(alloc_bytes_expected == allocator.get_total_bytes_in_use());
		assert((allocator.get_total_heapSize() - alloc_bytes_expected) == allocator.get_total_free_bytes());
		assert(allocator.get_number_of_free_chunks() == 1); //Свободное место должно объединяться
	}

	//Вся память должна быть освобождена
	assert(allocator.get_total_heapSize() == allocator.get_total_free_bytes());
	assert(allocator.get_total_bytes_in_use() == 0);

	std::cout <<"TEST 3 passed" << std::endl;
}


void Test4(){

	std::cout <<"TEST 4 start" << std::endl;
	Allocator allocator = Allocator();

	size_t ChunkInfo  = sizeof(Allocator::ChunkNodeInfo_t);
	size_t alloc_size = ChunkInfo; //Ячеки будут размером ChunkInfo

	std::vector<char*> allocated = {};

	size_t alloc_counter = 0;
	//Выделим выделим столько памяти сколько сможем, а потом будем ее освобождать с конца
	while(allocator.get_total_free_bytes() != 0) {
		char* tmp = (char*)allocator.m_malloc(alloc_size);
		allocated.push_back(tmp);
		alloc_counter++;

		size_t alloc_bytes_expected = alloc_counter * ChunkInfo * 2;
		assert(alloc_bytes_expected == allocator.get_total_bytes_in_use());
		assert((allocator.get_total_heapSize() - alloc_bytes_expected) == allocator.get_total_free_bytes());
	}

	size_t index = alloc_counter - 1;

	while(alloc_counter > 0){
		allocator.m_free(allocated[index]);
		index--;
		alloc_counter--;

		size_t alloc_bytes_expected = alloc_counter * ChunkInfo * 2;
		assert(alloc_bytes_expected == allocator.get_total_bytes_in_use());
		assert((allocator.get_total_heapSize() - alloc_bytes_expected) == allocator.get_total_free_bytes());
		assert(allocator.get_number_of_free_chunks() == 1); //Свободное место должно объединяться
	}

	//Вся память должна быть освобождена
	assert(allocator.get_total_heapSize() == allocator.get_total_free_bytes());
	assert(allocator.get_total_bytes_in_use() == 0);

	std::cout <<"TEST 4 passed" << std::endl;
}

void Test5(){

	std::cout <<"TEST 5 start" << std::endl;
	Allocator allocator = Allocator();

	size_t ChunkInfo  = sizeof(Allocator::ChunkNodeInfo_t);
	size_t alloc_size = ChunkInfo; //Ячеки будут размером ChunkInfo

	std::vector<char*> allocated = {};

	size_t alloc_counter = 0;
	//Выделим выделим столько памяти сколько сможем
	while(allocator.get_total_free_bytes() != 0) {
		char* tmp = (char*)allocator.m_malloc(alloc_size);
		allocated.push_back(tmp);
		alloc_counter++;

		size_t alloc_bytes_expected = alloc_counter * ChunkInfo * 2;
		assert(alloc_bytes_expected == allocator.get_total_bytes_in_use());
		assert((allocator.get_total_heapSize() - alloc_bytes_expected) == allocator.get_total_free_bytes());
	}

	size_t size_alloc = alloc_counter;

	//А теперь будем совобождать через один сначала
	size_t index = 0;
	while(index < size_alloc){
		allocator.m_free(allocated[index]);
		index += 2;
		alloc_counter--;
		size_t alloc_bytes_expected = alloc_counter * ChunkInfo * 2;
		assert(alloc_bytes_expected == allocator.get_total_bytes_in_use());
		assert((allocator.get_total_heapSize() - alloc_bytes_expected) == allocator.get_total_free_bytes());
	}

	//А теперь через 1 со смещением 1
	index = 1;
	while(index < size_alloc){
		allocator.m_free(allocated[index]);
		index += 2;
		alloc_counter--;
		size_t alloc_bytes_expected = alloc_counter * ChunkInfo * 2;
		assert(alloc_bytes_expected == allocator.get_total_bytes_in_use());
		assert((allocator.get_total_heapSize() - alloc_bytes_expected) == allocator.get_total_free_bytes());
	}

	//Вся память должна быть освобождена
	assert(allocator.get_total_heapSize() == allocator.get_total_free_bytes());
	assert(allocator.get_total_bytes_in_use() == 0);

	std::cout <<"TEST 5 passed" << std::endl;
}


int main() {
	
	Test1();
	Test2();
	Test3();
	Test4();
	Test5();

	return 0;
}