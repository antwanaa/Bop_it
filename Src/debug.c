#include "debug.h"
#include "usart.h"


int _write(int fd, char * ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t *) ptr, len, HAL_MAX_DELAY);

    return len;
}