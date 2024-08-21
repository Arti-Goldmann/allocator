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
	//��������� � ���������� ������ ����� ��� ������ ���������� ������ � �����������
	bytes_to_alloc += sizeof(ChunkNodeInfo_t);
	//��� ����� � ������ ������ �������� �� ���������� ����� ������� sizeof(ChunkNodeInfo_t)
	//�������� ���������� ����, ���� �����, ����� �� ��� �������� N * sizeof(ChunkNodeInfo_t)
	if ((bytes_to_alloc & config::ALIGMENT_MASK) != 0) {
		bytes_to_alloc += config::ALIGMENT;
		bytes_to_alloc &= ~config::ALIGMENT_MASK;
	}

	//�������� �� ���������� ������ � ����������� � ������, ����� ����� ��������� ������������ �������
	ChunkNodeInfo_t* prev_node = &start_edge;
	ChunkNodeInfo_t* cur_node = start_edge.next;
	while ((cur_node->freeSize < bytes_to_alloc) && (cur_node != &end_edge)) {
		prev_node = cur_node;
		cur_node = cur_node->next;
	}
	if (cur_node == &end_edge) {
		//��� ������ �������� �����
#ifdef DEBUG_ALLOCATOR
		std::cout << "\n***ERROR_CAN'T_ALLOC***\n" << std::endl;
#endif
		xTaskResumeAll();
		return NULL;
	}

	//�������� ����� ������ "������������" ��������� � �����������
	alloc_adress = (void*)((size_t)cur_node + sizeof(ChunkNodeInfo_t));

	//������� ���� ���� �� ������ ��������� ������
	prev_node->next = cur_node->next;
	cur_node->next = NULL;

#ifdef DEBUG_ALLOCATOR
	print_m_malloc_info((size_t)alloc_adress, bytes_to_alloc);
#endif

	//��������� ����� �� ������ ������ ����������� ����� �������� � ������ ��������� ������ ������.
	//���� �������� ������ ���������� ���������� ����� ( sizeof(ChunkNodeInfo_t) x 2: ����� ��� ���������� � ��� ���������� ������), 
	//�� ����� �������� � ������� ������ � ������
	//<!����� ����� ��������: ������ ������ ��� ������ ���� �����
	if ((cur_node->freeSize - bytes_to_alloc) >= (sizeof(ChunkNodeInfo_t) << 1)) {
		ChunkNodeInfo_t* new_free_chunk = (ChunkNodeInfo_t*)((size_t)cur_node + bytes_to_alloc);
		new_free_chunk->freeSize = cur_node->freeSize - bytes_to_alloc;
		//�������� ������� ���� �� �������� ��� ����� ����� 
		cur_node->freeSize = bytes_to_alloc;
		add_new_free_chunk_to_list(new_free_chunk);
	}

	xTaskResumeAll();
	return alloc_adress;
}

void Allocator::m_free(void* adress_to_free) {

#ifdef DEBUG_ALLOCATOR
	std::cout << "\n***M_FREE***\n" << std::endl;
#endif
	
	//������ ���������� ����� �� ������ ��������� � �����������
	ChunkNodeInfo_t* new_free_chunk = (ChunkNodeInfo_t*) ((size_t)adress_to_free - sizeof(ChunkNodeInfo_t));
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

	//���� �������� ������, ��� ����� ����� ����� � ������ ������ ���������� �����
	ChunkNodeInfo_t* prev_node =  &start_edge;
	ChunkNodeInfo_t* cur_node  =  start_edge.next;
	
	//���� ��� ������ ��������� ������
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
	//��������� �������� �� ����� ���� ����������� � ����� � ����� �� �� ���������
	//���� ��, �� ����� ������ ����� ���� ��� ������ ������ �������� � ������� ������ �����
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
	//��������� �������� �� ����� ���� ����������� � ������ ������ � ����� �� �� ���������
    //���� ��, �� ����� ������ ����� ���� ��� ������ ������ �������� � ������� ������� �����
	if (((size_t)new_node + (size_t)new_node->freeSize) == (size_t)cur_node) {
		//�� ������ ���� ������ ���� �� ����� �������. ����� �� ������ �������� � ����������� ������
		if (cur_node != &end_edge) {
			new_node->freeSize += cur_node->freeSize;
			new_node->next = cur_node->next;
			new_and_cur_link = true;

#ifdef DEBUG_ALLOCATOR
			std::cout << " new chunk and right link->" << std::endl;
#endif
		}
	}
	
	//���� �� ���� ����������� ������ ����� c ����� ������, �� ����� ���� ��� ����������� ������ �����
	if (prev_and_new_link == false) {
		
		//���� �� ���� ����������� ������ ����� � ������ ������, �� ����������� ������ ����� ��� ����������� ������ ����� 
		//(�������� ����� �������)
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

	//����� ��������� ������������ ����� � ������,
	//����� ��� ����� � ������ ���� ������ sizeof(ChunkNodeInfo_t) - ��������� � ����������� � ������

	//����������� ����� �������
	size_t temp_adress = (size_t) &MyHeap[0];

	if ((temp_adress & config::ALIGMENT_MASK) != 0)
	{
		temp_adress += (config::ALIGMENT - 1);
	    temp_adress &= ~config::ALIGMENT_MASK;
		total_heap_size -= (temp_adress - (size_t)&MyHeap[0]);
	}
	left_alligned_adress = (uint8_t*)temp_adress;

	//����������� ������ �������
	temp_adress = (size_t)(left_alligned_adress + total_heap_size);
	right_alligned_adress = (uint8_t*)(temp_adress & ~config::ALIGMENT_MASK);
	total_heap_size = right_alligned_adress - left_alligned_adress;

	//������ ������ ������� ���������� ������, ��� ������������ ����� ��������������� � MyHeap � ����� ������ � ������ �������� 
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
		std::cout << " start adress of new free chunk: " << std::hex << (size_t)node << std::endl;
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