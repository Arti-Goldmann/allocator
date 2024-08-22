#include "Allocator.h"
#include <iostream>


void vTaskSuspendAll() {
};

void xTaskResumeAll() {
}


int main() {
	
	char* ptr1;
	char* ptr2;
	char* ptr3;
	char* ptr4;
	char* ptr5;
	char* ptr6;

	std::cout << "----------TEST 1----------" << std::endl;

	//Выделяем 3 и смотрим как вставится средний
	Allocator allocator = Allocator();
	std::cout << "---------ptr1 malloc" << std::endl;
	ptr1 = (char*)allocator.m_malloc(2 * sizeof(char));
	std::cout << "---------ptr2 malloc" << std::endl;
	ptr2 = (char*)allocator.m_malloc(2 * sizeof(char));
	std::cout << "---------ptr3 malloc" << std::endl;
	ptr3 = (char*)allocator.m_malloc(2 * sizeof(char));
	std::cout << "---------ptr1 free" << std::endl;
	allocator.m_free(ptr1);
	std::cout << "---------ptr3 free" << std::endl;
	allocator.m_free(ptr3);
	std::cout << "---------ptr2 free" << std::endl;
	allocator.m_free(ptr2);

	std::cout << "----------TEST 2----------" << std::endl;

	//Выделяем 2
	std::cout << "---------ptr1 malloc" << std::endl;
	ptr1 = (char*)allocator.m_malloc(2 * sizeof(char));
	std::cout << "---------ptr2 malloc" << std::endl;
	ptr2 = (char*)allocator.m_malloc(2 * sizeof(char));
	std::cout << "---------ptr1 free" << std::endl;
	allocator.m_free(ptr1);
	std::cout << "---------ptr2 free" << std::endl;
	allocator.m_free(ptr2);

	std::cout << "----------TEST 3----------" << std::endl;

	//Выделяем 2
	std::cout << "---------ptr1 malloc" << std::endl;
	ptr1 = (char*)allocator.m_malloc(2 * sizeof(char));
	std::cout << "---------ptr2 malloc" << std::endl;
	ptr2 = (char*)allocator.m_malloc(2 * sizeof(char));
	std::cout << "---------ptr2 free" << std::endl;
	allocator.m_free(ptr2);
	std::cout << "---------ptr1 free" << std::endl;
	allocator.m_free(ptr1);

	std::cout << "----------TEST 4----------" << std::endl;
	ptr1 = (char*)allocator.m_malloc(2 * sizeof(char));
	ptr2 = (char*)allocator.m_malloc(2 * sizeof(char));
	ptr3 = (char*)allocator.m_malloc(2 * sizeof(char));
	ptr4 = (char*)allocator.m_malloc(2 * sizeof(char));
	ptr5 = (char*)allocator.m_malloc(2 * sizeof(char));
	ptr6 = (char*)allocator.m_malloc(2 * sizeof(char));

	allocator.m_free(ptr2);
	allocator.m_free(ptr4);
	allocator.m_free(ptr1);
	allocator.m_free(ptr3);
	allocator.m_free(ptr5);
	allocator.m_free(ptr6);

	std::cout << "----------TEST 5----------" << std::endl;
	ptr1 = (char*)allocator.m_malloc(192 - 16);
	allocator.m_free(ptr1);

	allocator.print_heap();
	return 0;
}