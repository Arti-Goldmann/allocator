#include "Allocator.h"
#include <iostream>

uint8_t Allocator::MyHeap[TOTAL_CHUNK_BYTES * config::SIZE_OF_HEAP_CHUNKS];
Allocator::ChunkNodeInfo_t Allocator::start_edge;
Allocator::ChunkNodeInfo_t Allocator::end_edge;

extern void vTaskSuspendAll();
extern void xTaskResumeAll();

Allocator::Allocator() {
	init();
}

void* Allocator::m_alloc_chunk() {

	void* alloc_adress = NULL;

	vTaskSuspendAll();

	//Будем выделять из первого же свободного блока
	ChunkNodeInfo_t* prev_node = &start_edge;
	ChunkNodeInfo_t* cur_node =  start_edge.next;

	if (cur_node == &end_edge) {
		//Уже нечего выделять
		xTaskResumeAll();
		return NULL;
	}

	//Передаем адрес памяти "перепрыгивая" структуру с информацией
	alloc_adress = (void*)((size_t)cur_node + CHUNK_INFO_BYTES);

	//Удаляем этот блок из списка свободных блоков
	prev_node->next = cur_node->next;
	cur_node->next = NULL;

	//Проверяем можно ли лишнюю память, которая осталась добавить в список свободных блоков памяти.
	//Если в этой непрерывной области памяти находится больше одного блока, то ее можно разделить.
	if (cur_node->free_chunks > 1) {
		//Перепрыгиваем через структуру с информацией и через размер блока и получаем адрес нового блока
		ChunkNodeInfo_t* new_free_chunk = (ChunkNodeInfo_t*)((size_t)cur_node + TOTAL_CHUNK_BYTES);

		new_free_chunk->free_chunks = cur_node->free_chunks - 1;
		cur_node->free_chunks = 1;
		add_new_free_chunk_to_list(new_free_chunk);
	}

	chunks_in_use++;
	free_chunks--;

	xTaskResumeAll();
	return alloc_adress;
}

void Allocator::m_free(void* adress_to_free) {
	
	vTaskSuspendAll();
	//Должны сместиться влево на размер структуры с информацией
	ChunkNodeInfo_t* new_free_chunk = (ChunkNodeInfo_t*) ((size_t)adress_to_free - CHUNK_INFO_BYTES);
	
	chunks_in_use--;
	free_chunks++;

	add_new_free_chunk_to_list(new_free_chunk);
	xTaskResumeAll();
}

void Allocator::add_new_free_chunk_to_list(ChunkNodeInfo_t* new_node) {

	//Ищем элементы списка, чей адрес будет левее и правее нового свободного блока
	ChunkNodeInfo_t* prev_node =  &start_edge;
	ChunkNodeInfo_t* cur_node  =  start_edge.next;
	
	//Если это первая свободная ячейка
	if (cur_node == &end_edge) {
		start_edge.next = new_node;
		new_node->next = &end_edge;
		return;
	}

	while ( ((size_t)cur_node < (size_t)new_node) && (cur_node != &end_edge)) {
		prev_node = cur_node;
		cur_node = cur_node->next;
	}
	
	bool prev_and_new_link = false;
	//Проверяем является ли новый блок непрерывным с левым и можно ли их объеднить
	//Если да, то адрес левого блока плюс его размер должен совпасть с адресом нового блока
	if ( ((size_t)prev_node + prev_node->free_chunks * TOTAL_CHUNK_BYTES) == (size_t)new_node ) {
		size_t add_free_chunks = new_node->free_chunks;
		new_node = prev_node;
		new_node->free_chunks += add_free_chunks;
		prev_and_new_link = true;
	}

	bool new_and_cur_link = false;
	//Проверяем является ли новый блок непрерывным с правым блоком и можно ли их объеднить
    //Если да, то адрес нового блока плюс его размер должен совпасть с адресом правого блока
	if (((size_t)new_node + new_node->free_chunks * TOTAL_CHUNK_BYTES) == (size_t)cur_node) {
		//Но только если правый блок не конец массива. Тогда мы просто уперлись в ограничение справа
		if (cur_node != &end_edge) {
			new_node->free_chunks += cur_node->free_chunks;
			new_node->next = cur_node->next;
			new_and_cur_link = true;
		}
	}
	
	//Если не было объединения нового блока c левым блоком, то новый блок это продолжение левого блока
	if (prev_and_new_link == false) {
		
		//Если не было объединения нового блока с правым блоком, то продолжение нового блока это продолжение левого блока 
		//(вставили между блоками)
		if (new_and_cur_link == false)
			new_node->next = prev_node->next;
		
		prev_node->next = new_node;
	}

}

void Allocator::init() {

	total_heap_size = TOTAL_CHUNK_BYTES * config::SIZE_OF_HEAP_CHUNKS;

	//Создем первый элемент связанного списка, его расположение будет непосредственно в начале MyHeap
	ChunkNodeInfo_t* first_free_chunk = (ChunkNodeInfo_t*) &MyHeap[0];
	first_free_chunk->next = &end_edge;
	first_free_chunk->free_chunks = config::SIZE_OF_HEAP_CHUNKS;
	start_edge.next = first_free_chunk;

	free_chunks = first_free_chunk->free_chunks;
}

size_t  Allocator::get_free_chunks_num(){
	//Могли бы просто вернуть free_chunks, которые и так записываются,
	//но для проверки пройдемся по связанному списку
	//и посчитаем сколько свободного места
	size_t free_chunks_sum = 0;
	ChunkNodeInfo_t* node = start_edge.next;
	while (node != &end_edge) {
		free_chunks_sum += node->free_chunks;
		node = node->next;
	
	}
	return free_chunks_sum;
}       
size_t  Allocator::get_chunks_num_in_use(){
	return chunks_in_use;
}    
size_t  Allocator::get_total_heap_size(){
	return total_heap_size;
}     
size_t  Allocator::get_size_of_free_list(){
	//Вернем общее кол-во свободных раздельных ячеек
	ChunkNodeInfo_t* node = start_edge.next;
	size_t free_chunks_counter = 0;
	while (node != &end_edge) {
		free_chunks_counter++;
		node = node->next;
	
	}
	return free_chunks_counter;
}
size_t Allocator::get_heap_start_adress(){
	return (size_t)&MyHeap[0];
}     


//Функции для отладки
void Allocator::print_init_info() {

	std::cout << "\n***Init info***\n" << std::endl;

	std::cout << "&MyHeap[0]: " << std::hex << (size_t)&MyHeap[0] << std::endl;
	std::cout << "&MyHeap[total_heap_size - 1]: " << std::hex << (size_t)&MyHeap[total_heap_size - 1] << std::endl;
	std::cout << "total_heap_size: " << std::dec << (size_t)total_heap_size << std::endl;

}

void Allocator::print_free_list() {
	
	ChunkNodeInfo_t* node = start_edge.next;
	std::cout << " free list->\n" << std::endl;
	while (node != &end_edge) {
		std::cout << " start adress of free chunk: " << std::hex << (size_t)node << std::endl;
		std::cout << " free chunks: " << std::dec << (size_t)node->free_chunks << std::endl;
		node = node->next;
	}
}

void Allocator::print_heap() {
	std::cout << "\nMyHeap:\n" << std::endl;
	for(size_t index = 0; index < total_heap_size; index++) {
		std::cout
			<< std::dec << " index: " << index
			<< std::hex << " adr: "   << (size_t) &MyHeap[index]
			<< std::hex << " val: "   << (size_t) MyHeap[index]
			<< std::endl;
	}
}

void Allocator::print_m_malloc_info(size_t ptr, size_t bytes) {
	std::cout << "new adress: " << std::hex << ptr << std::endl;
	std::cout << "bytes allocated: " << std::dec << bytes << std::endl;
}

void Allocator::print_m_free_info(size_t ptr) {
	std::cout << "free adress: " << std::hex << ptr << std::endl;
}