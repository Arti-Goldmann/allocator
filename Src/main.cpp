#include "main.h"

void vTaskSuspendAll() {
	//Нужно удалить если подключен FreeRTOS
};

void xTaskResumeAll() {
	//Нужно удалить если подключен FreeRTOS
}

int main() {
	//Выделим максимальное кол-во блоков, а потом освободим по порядку с конца или с начала
	Test_alloc_and_free("TEST 1", fromStart);
	Test_alloc_and_free("TEST 2", fromEnd);
	//Выделим максимальное кол-во блоков, а потом будем освобождать через один с конца или с начала
	Test_alloc_and_free_through_one("TEST 3", fromStart);
	Test_alloc_and_free_through_one("TEST 4", fromEnd);
	//Выделим максимальное кол-во блоков, освободим блоки через один, потом опять выделим все
	Test_alloc_and_free_through_one_v2("Test 5", fromStart);
	Test_alloc_and_free_through_one_v2("Test 6", fromEnd);

	return 0;
}