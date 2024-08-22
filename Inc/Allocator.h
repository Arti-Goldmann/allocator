#pragma once
#include <iostream>

//#define DEBUG_ALLOCATOR

#define ALLOCATOR_MEM_ADRESSING_64_BIT		//win64
//#define ALLOCATOR_MEM_ADRESSING_32_BIT	//stm32

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
	
	size_t  get_free_bytes();            //Кол-во свободных байт для выделению пользователю
	size_t  get_total_free_bytes();      //Всего свободных байт (с учетом списка для информации)
	size_t  get_bytes_in_use();          //Кол-во занятых байт выделенных пользователю
	size_t  get_total_bytes_in_use();    //Всего занятых байт (с учетом списка для информации)
	size_t  get_number_of_free_chunks(); //Кол-во свободных разделенных ячеек памяти
	size_t  get_total_heapSize();        //Всего размер кучи

private:

	static uint8_t MyHeap[config::SIZE_OF_HEAP_BYTES];
	static ChunkNodeInfo_t start_edge; //Левая граница в MyHeap, за которую мы не должны выходить
	static ChunkNodeInfo_t end_edge;   //Правая граница в MyHeap, за которую мы не должны выходить

	uint8_t* left_alligned_adress;
	uint8_t* right_alligned_adress;

	size_t total_heap_size;
	size_t total_bytes_in_use = {0};
	size_t user_bytes_in_use = {0};			
	void add_new_free_chunk_to_list(ChunkNodeInfo_t*);
	void init();

	//Функции для отладки
	void print_heap();
	void print_init_info();
	void print_free_list();
	void print_m_malloc_info(size_t, size_t);
	void print_m_free_info(size_t);

};