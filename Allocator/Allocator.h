#pragma once
#include <stdint.h>

#define DEBUG_ALLOCATOR

#define ALLOCATOR_MEM_ADRESSING_64_BIT
//#define ALLOCATOR_MEM_ADRESSING_32_BIT

class Allocator {
public:
	struct config {
		static constexpr size_t SIZE_OF_HEAP_BYTES = 200;
#ifdef ALLOCATOR_MEM_ADRESSING_64_BIT
		static constexpr size_t ALIGMENT_MASK      = 0b1111;
		static constexpr size_t ALIGMENT           = 16;
#elif  ALLOCATOR_MEM_ADRESSING_32_BIT
		static constexpr size_t ALIGMENT_MASK      = 0b111;
		static constexpr size_t ALIGMENT           = 8;
#else
	#error "Choose memory adressing"
#endif
	};
	
	typedef struct S_ChunkNodeInfo {
		struct S_ChunkNodeInfo* next = { NULL }; //Следующий свободный блок
		size_t freeSize = {0};                   //Размер свободного места в этом блоке
	} ChunkNodeInfo_t;                           //Элемент связанного списка свободных блоков (chunk)
	//Размер одного такого элемента:
	//16 байт для 64-ех - разрядной адресации памяти (например для win64)
	//8  байт для 32-ух - разрядной адресации памяти (например для stm32)

	Allocator();
	~Allocator() = default;

	void* m_malloc(size_t);
	void  m_free(void*);
	void print_heap();
private:

	static uint8_t MyHeap[config::SIZE_OF_HEAP_BYTES];
	static ChunkNodeInfo_t start_edge; //Левая граница в MyHeap, за которую мы не должны выходить
	static ChunkNodeInfo_t end_edge;   //Правая граница в MyHeap, за которую мы не должны выходить

	uint8_t* left_alligned_adress;
	uint8_t* right_alligned_adress;

	size_t total_heap_size;	
	void add_new_free_chunk_to_list(ChunkNodeInfo_t*);
	void init();

	void print_init_info();
	void print_free_list();
	void print_m_malloc_info(size_t, size_t);
	void print_m_free_info(size_t);

};