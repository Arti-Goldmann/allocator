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

	return 0;
}