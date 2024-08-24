#pragma once
#include <iostream>

//#define DEBUG_ALLOCATOR

#define ALLOCATOR_MEM_ADRESSING_64_BIT		//win64
//#define ALLOCATOR_MEM_ADRESSING_32_BIT	//stm32

#if defined(ALLOCATOR_MEM_ADRESSING_64_BIT) && defined(ALLOCATOR_MEM_ADRESSING_32_BIT)
	#error "Choose only one memory adressing"
#endif

class Allocator {
public:
	struct config {
		static constexpr size_t SIZE_OF_HEAP_CHUNKS = 200;  //Сколько блоков в куче
		static constexpr size_t SIZE_OF_CHUNK_BYTES = 16;   //Размер блока в байтах
	};
	
	typedef struct S_ChunkNodeInfo {
		struct S_ChunkNodeInfo* next = { NULL }; //Следующий свободный блок
		size_t free_chunks = {0};                //Кол-во свободных блоков в непрерывной области памяти 
	} ChunkNodeInfo_t;                           //Элемент связанного списка свободных блоков (chunk)
	//Размер одного такого элемента:
	//16 байт для 64-ех - разрядной адресации памяти (например для win64)
	//8  байт для 32-ух - разрядной адресации памяти (например для stm32)

#if   defined(ALLOCATOR_MEM_ADRESSING_64_BIT)
	static constexpr size_t CHUNK_INFO_BYTES = 16;
#elif defined(ALLOCATOR_MEM_ADRESSING_32_BIT)
	static constexpr size_t CHUNK_INFO_BYTES = 8;
#else
	#error "Choose memory adressing"
#endif
	//Суммарный размер одного блока памяти с учетом структуры для информации
	static constexpr size_t TOTAL_CHUNK_BYTES = CHUNK_INFO_BYTES + config::SIZE_OF_CHUNK_BYTES;

	Allocator();
	~Allocator() = default;

	void* m_alloc_chunk();				 //Выделяет 1 блок фиксированного размера
	void  m_free(void*);			     //Освобождает 1 блок фиксированного размера
	
	size_t  get_free_chunks_num();       //Кол-во свободных блоков для выделения пользователю
	size_t  get_chunks_num_in_use();     //Кол-во занятых блоков пользователем
	size_t  get_total_heap_size();       //Сколько всего байт занимает куча
	size_t  get_size_of_free_list();     //Размер связанного списка свободных блоков
	size_t  get_heap_start_adress();     //Начальный адрес кучи

private:

	static uint8_t MyHeap[ TOTAL_CHUNK_BYTES * config::SIZE_OF_HEAP_CHUNKS ];
	static ChunkNodeInfo_t start_edge; //Левая граница в MyHeap, за которую мы не должны выходить
	static ChunkNodeInfo_t end_edge;   //Правая граница в MyHeap, за которую мы не должны выходить

	size_t total_heap_size;
	size_t chunks_in_use = {0};
	size_t free_chunks = {0};

	void add_new_free_chunk_to_list(ChunkNodeInfo_t*);
	void init();

	//Функции для отладки
	void print_heap();
	void print_init_info();
	void print_free_list();
	void print_m_malloc_info(size_t, size_t);
	void print_m_free_info(size_t);

};