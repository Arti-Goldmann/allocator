#include "Allocator.h"
#include <iostream>

uint8_t Allocator::MyHeap[config::SIZE_OF_HEAP_BYTES];
Allocator::ChunkNodeInfo_t Allocator::start_edge;
Allocator::ChunkNodeInfo_t Allocator::end_edge;

extern void vTaskSuspendAll();
extern void xTaskResumeAll();

Allocator::Allocator() {
	init();
}

void* Allocator::m_malloc(size_t bytes_to_alloc) {

#ifdef DEBUG_ALLOCATOR
	std::cout << "\n***M_MALLOC***\n" << std::endl;
	std::cout << "required bytes to allocate: " << bytes_to_alloc << std::endl;
#endif

	void* alloc_adress = NULL;
	if (!bytes_to_alloc) return NULL;

	vTaskSuspendAll();
	//Добавляем к выделяемой памяти место под объект связанного списка с информацией
	bytes_to_alloc += sizeof(ChunkNodeInfo_t);
	//Все блоки в памяти должны состоять из одинаковых ячеек размера sizeof(ChunkNodeInfo_t)
	//Дорастим выделяемый блок, если нужно, чтобы он был размером N * sizeof(ChunkNodeInfo_t)
	if ((bytes_to_alloc & config::ALIGMENT_MASK) != 0) {
		bytes_to_alloc += config::ALIGMENT;
		bytes_to_alloc &= ~config::ALIGMENT_MASK;
	}

	//Проходим по связанному списку с информацией о блоках, чтобы найти свободный необходимого размера
	ChunkNodeInfo_t* prev_node = &start_edge;
	ChunkNodeInfo_t* cur_node = start_edge.next;
	while ((cur_node->freeSize < bytes_to_alloc) && (cur_node != &end_edge)) {
		prev_node = cur_node;
		cur_node = cur_node->next;
	}
	if (cur_node == &end_edge) {
		//Нет такого большого блока
#ifdef DEBUG_ALLOCATOR
		std::cout << "\n***ERROR_CAN'T_ALLOC***\n" << std::endl;
#endif
		xTaskResumeAll();
		return NULL;
	}

	//Передаем адрес памяти "перепрыгивая" структуру с информацией
	alloc_adress = (void*)((size_t)cur_node + sizeof(ChunkNodeInfo_t));

	//Удаляем этот блок из списка свободных блоков
	prev_node->next = cur_node->next;
	cur_node->next = NULL;

#ifdef DEBUG_ALLOCATOR
	print_m_malloc_info((size_t)alloc_adress, bytes_to_alloc);
#endif

	//Проверяем можно ли лишнюю память выделенного блока добавить в список свободных блоков памяти.
	//Если осталось больше минимально возможного блока ( sizeof(ChunkNodeInfo_t) x 2: место под информацию и под выделяемую память), 
	//то тогда разделим и добавим лишнее в список
	//<!Здесь нужно подумать: строго больше или больше либо равно
	if ((cur_node->freeSize - bytes_to_alloc) >= (sizeof(ChunkNodeInfo_t) << 1)) {
		ChunkNodeInfo_t* new_free_chunk = (ChunkNodeInfo_t*)((size_t)cur_node + bytes_to_alloc);
		new_free_chunk->freeSize = cur_node->freeSize - bytes_to_alloc;
		//Сохраним сколько байт мы выделили для этого блока 
		cur_node->freeSize = bytes_to_alloc;
		add_new_free_chunk_to_list(new_free_chunk);
	}

	total_bytes_in_use += bytes_to_alloc;
	user_bytes_in_use  += bytes_to_alloc - sizeof(ChunkNodeInfo_t);

	xTaskResumeAll();
	return alloc_adress;
}

void Allocator::m_free(void* adress_to_free) {

#ifdef DEBUG_ALLOCATOR
	std::cout << "\n***M_FREE***\n" << std::endl;
#endif
	
	//Должны сместиться влево на размер структуры с информацией
	ChunkNodeInfo_t* new_free_chunk = (ChunkNodeInfo_t*) ((size_t)adress_to_free - sizeof(ChunkNodeInfo_t));
	
	total_bytes_in_use -= new_free_chunk->freeSize;
	user_bytes_in_use  -= new_free_chunk->freeSize - sizeof(ChunkNodeInfo_t);

	vTaskSuspendAll();
	add_new_free_chunk_to_list(new_free_chunk);
	xTaskResumeAll();

#ifdef DEBUG_ALLOCATOR
	print_m_free_info((size_t)adress_to_free);
#endif
}

void Allocator::add_new_free_chunk_to_list(ChunkNodeInfo_t* new_node) {

#ifdef DEBUG_ALLOCATOR
	std::cout << " add_new_free_chunk_to_list->" << std::endl;
#endif

	//Ищем элементы списка, чей адрес будет левее и правее нового свободного блока
	ChunkNodeInfo_t* prev_node =  &start_edge;
	ChunkNodeInfo_t* cur_node  =  start_edge.next;
	
	//Если это первая свободная ячейка
	if (cur_node == &end_edge) {
		start_edge.next = new_node;
		new_node->next = &end_edge;

#ifdef DEBUG_ALLOCATOR
		std::cout << " first free chunk->" << std::endl;
		print_free_list();
#endif
		return;
	}

	while ( ((size_t)cur_node < (size_t)new_node) && (cur_node != &end_edge)) {
		prev_node = cur_node;
		cur_node = cur_node->next;
	}
	
	bool prev_and_new_link = false;
	//Проверяем является ли новый блок непрерывным с левым и можно ли их объеднить
	//Если да, то адрес левого блока плюс его размер должен совпасть с адресом нового блока
	if ( ((size_t)prev_node + (size_t)prev_node->freeSize) == (size_t)new_node ) {
		size_t addFreeSize = new_node->freeSize;
		new_node = prev_node;
		new_node->freeSize += addFreeSize;
		prev_and_new_link = true;

#ifdef DEBUG_ALLOCATOR
		std::cout << " left and new chunk link->" << std::endl;
#endif
	}

	bool new_and_cur_link = false;
	//Проверяем является ли новый блок непрерывным с правым блоком и можно ли их объеднить
    //Если да, то адрес нового блока плюс его размер должен совпасть с адресом правого блока
	if (((size_t)new_node + (size_t)new_node->freeSize) == (size_t)cur_node) {
		//Но только если правый блок не конец массива. Тогда мы просто уперлись в ограничение справа
		if (cur_node != &end_edge) {
			new_node->freeSize += cur_node->freeSize;
			new_node->next = cur_node->next;
			new_and_cur_link = true;

#ifdef DEBUG_ALLOCATOR
			std::cout << " new chunk and right link->" << std::endl;
#endif
		}
	}
	
	//Если не было объединения нового блока c левым блоком, то новый блок это продолжение левого блока
	if (prev_and_new_link == false) {
		
		//Если не было объединения нового блока с правым блоком, то продолжение нового блока это продолжение левого блока 
		//(вставили между блоками)
		if (new_and_cur_link == false)
			new_node->next = prev_node->next;
		
		prev_node->next = new_node;

#ifdef DEBUG_ALLOCATOR
		std::cout << " new chunk is the next for prev->" << std::endl;
#endif
	}

#ifdef DEBUG_ALLOCATOR
	print_free_list();
#endif

}

void Allocator::init() {

	total_heap_size = config::SIZE_OF_HEAP_BYTES;

	//Нужно выровнять расположение ячеек в памяти,
	//чтобы все блоки в памяти были кратны sizeof(ChunkNodeInfo_t) - стркутуре с информацией о памяти

	//Настраиваем левую границу
	size_t temp_adress = (size_t) &MyHeap[0];

	if ((temp_adress & config::ALIGMENT_MASK) != 0)
	{
		temp_adress += (config::ALIGMENT - 1);
	    temp_adress &= ~config::ALIGMENT_MASK;
		total_heap_size -= (temp_adress - (size_t)&MyHeap[0]);
	}
	left_alligned_adress = (uint8_t*)temp_adress;

	//Настраиваем правую границу
	temp_adress = (size_t)(left_alligned_adress + total_heap_size);
	right_alligned_adress = (uint8_t*)(temp_adress & ~config::ALIGMENT_MASK);
	total_heap_size = right_alligned_adress - left_alligned_adress;

	//Создем первый элемент связанного списка, его расположение будет непосредственно в MyHeap в самом начале с учетом смещения 
	ChunkNodeInfo_t* first_free_chunk = (ChunkNodeInfo_t*)left_alligned_adress;
	first_free_chunk->next = &end_edge;
	first_free_chunk->freeSize = total_heap_size;
	start_edge.next = first_free_chunk;

#ifdef DEBUG_ALLOCATOR
	print_init_info();
	print_heap();
	print_free_list();
#endif
}

size_t Allocator::get_free_bytes(){
	//Вернем кол-во байтов свободных для пользователя
	//c учетом того, что место еще требуется под связанный список с информацией
	ChunkNodeInfo_t* node = start_edge.next;
	size_t free_bytes_to_alloc = 0;
	while (node != &end_edge) {
		free_bytes_to_alloc += node->freeSize - sizeof(ChunkNodeInfo_t);
		node = node->next;
	}

	return free_bytes_to_alloc;
}
size_t Allocator::get_total_free_bytes(){
	//Вернем общее кол-во свободных байт
	ChunkNodeInfo_t* node = start_edge.next;
	size_t free_bytes_to_alloc = 0;
	while (node != &end_edge) {
		free_bytes_to_alloc += node->freeSize;
		node = node->next;
	}

	return free_bytes_to_alloc;

}

size_t Allocator::get_number_of_free_chunks(){
	//Вернем общее кол-во свободных раздельных ячеек
	ChunkNodeInfo_t* node = start_edge.next;
	size_t free_chunks_counter = 0;
	while (node != &end_edge) {
		free_chunks_counter++;
		node = node->next;
	
	}

	return free_chunks_counter;
}

size_t Allocator::get_bytes_in_use(){
	return user_bytes_in_use;
}

size_t Allocator::get_total_bytes_in_use(){
	return total_bytes_in_use;
}

size_t Allocator::get_total_heapSize(){
	return total_heap_size;
}


//Функции для отладки
void Allocator::print_init_info() {

	std::cout << "\n***Init info***\n" << std::endl;

	std::cout << "&MyHeap[0]: " << std::hex << (size_t)&MyHeap[0] << std::endl;
	std::cout << "&MyHeap[config::SIZE_OF_HEAP_BYTES - 1]: " << std::hex << (size_t)&MyHeap[config::SIZE_OF_HEAP_BYTES - 1] << std::endl;
	std::cout << "left_alligned_adress: " << std::hex << (size_t)left_alligned_adress << std::endl;
	std::cout << "right_alligned_adress: " << std::hex << (size_t)right_alligned_adress << std::endl;
	std::cout << "total_heap_size: " << std::dec << (size_t)total_heap_size << std::endl;

}

void Allocator::print_free_list() {
	
	ChunkNodeInfo_t* node = start_edge.next;
	std::cout << " free list->\n" << std::endl;
	while (node != &end_edge) {
		std::cout << " start adress of free chunk: " << std::hex << (size_t)node << std::endl;
		std::cout << " free size of chunk: " << std::dec << (size_t)node->freeSize << std::endl;
		node = node->next;
	}
}

void Allocator::print_heap() {
	std::cout << "\n***Alligned MyHeap***\n" << std::endl;
	uint8_t* it = left_alligned_adress;
	while (it < right_alligned_adress) {
		std::cout
			<< std::dec << " index: " << it - left_alligned_adress
			<< std::hex << " adr: " << (size_t)it
			<< std::hex << " val: " << (size_t)*it
			<< std::endl;
		it++;
	}
}

void Allocator::print_m_malloc_info(size_t ptr, size_t bytes) {
	std::cout << "new adress: " << std::hex << ptr << std::endl;
	std::cout << "bytes allocated: " << std::dec << bytes << std::endl;
}

void Allocator::print_m_free_info(size_t ptr) {
	std::cout << "free adress: " << std::hex << ptr << std::endl;
}