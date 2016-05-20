#include "stm32f4xx_hal.h"

#include <stdbool.h>
#include <inttypes.h>
#include <math.h>

#include "led.h"
#include "spi.h"

/**
 * buffer containing led data to be sent to sink drivers
 */
static uint16_t led_buffer[10][10] = {

    // 1th layer
    {
        0b0000000000000000, // 9th row of columns
        0b0000000000000000, // 8th row of columns
        0b0000000000000000, // 7th row of columns
        0b0000000000000000, // 6th row of columns
        0b0000000000000000, // 5th row of columns
        0b0000000000000000, // 4th row of columns
        0b0000000000000000, // 3nd row of columns
        0b0000000000000000, // 2nd row of columns
        0b0000000000000000, // 1st row of columns
        0b0000000000000001, // layer selection
    },
    // 2th layer
    {
        0b0000000000000000, // 9th row of columns
        0b0000000000000000, // 8th row of columns
        0b0000000000000000, // 7th row of columns
        0b0000000000000000, // 6th row of columns
        0b0000000000000000, // 5th row of columns
        0b0000000000000000, // 4th row of columns
        0b0000000000000000, // 3nd row of columns
        0b0000000000000000, // 2nd row of columns
        0b0000000000000000, // 1st row of columns
        0b0000000000000010, // layer selection
    },
    // 3th layer
    {
        0b0000000000000000, // 9th row of columns
        0b0000000000000000, // 8th row of columns
        0b0000000000000000, // 7th row of columns
        0b0000000000000000, // 6th row of columns
        0b0000000000000000, // 5th row of columns
        0b0000000000000000, // 4th row of columns
        0b0000000000000000, // 3nd row of columns
        0b0000000000000000, // 2nd row of columns
        0b0000000000000000, // 1st row of columns
        0b0000000000000100, // layer selection
    },
    // 4th layer
    {
        0b0000000000000000, // 9th row of columns
        0b0000000000000000, // 8th row of columns
        0b0000000000000000, // 7th row of columns
        0b0000000000000000, // 6th row of columns
        0b0000000000000000, // 5th row of columns
        0b0000000000000000, // 4th row of columns
        0b0000000000000000, // 3nd row of columns
        0b0000000000000000, // 2nd row of columns
        0b0000000000000000, // 1st row of columns
        0b0000000000001000, // layer selection
    },
    // 5th layer
    {
        0b0000000000000000, // 9th row of columns
        0b0000000000000000, // 8th row of columns
        0b0000000000000000, // 7th row of columns
        0b0000000000000000, // 6th row of columns
        0b0000000000000000, // 5th row of columns
        0b0000000000000000, // 4th row of columns
        0b0000000000000000, // 3nd row of columns
        0b0000000000000000, // 2nd row of columns
        0b0000000000000000, // 1st row of columns
        0b0000000000010000, // layer selection
    },
    // 6th layer
    {
        0b0000000000000000, // 9th row of columns
        0b0000000000000000, // 8th row of columns
        0b0000000000000000, // 7th row of columns
        0b0000000000000000, // 6th row of columns
        0b0000000000000000, // 5th row of columns
        0b0000000000000000, // 4th row of columns
        0b0000000000000000, // 3nd row of columns
        0b0000000000000000, // 2nd row of columns
        0b0000000000000000, // 1st row of columns
        0b0000000000100000, // layer selection
    },
    // 7th layer
    {
        0b0000000000000000, // 9th row of columns
        0b0000000000000000, // 8th row of columns
        0b0000000000000000, // 7th row of columns
        0b0000000000000000, // 6th row of columns
        0b0000000000000000, // 5th row of columns
        0b0000000000000000, // 4th row of columns
        0b0000000000000000, // 3nd row of columns
        0b0000000000000000, // 2nd row of columns
        0b0000000000000000, // 1st row of columns
        0b0000000001000000, // layer selection
    },
    // 8th layer
    {
        0b0000000000000000, // 9th row of columns
        0b0000000000000000, // 8th row of columns
        0b0000000000000000, // 7th row of columns
        0b0000000000000000, // 6th row of columns
        0b0000000000000000, // 5th row of columns
        0b0000000000000000, // 4th row of columns
        0b0000000000000000, // 3nd row of columns
        0b0000000000000000, // 2nd row of columns
        0b0000000000000000, // 1st row of columns
        0b0000000010000000, // layer selection
    },
    // 9th layer
    {
        0b0000000000000000, // 9th row of columns
        0b0000000000000000, // 8th row of columns
        0b0000000000000000, // 7th row of columns
        0b0000000000000000, // 6th row of columns
        0b0000000000000000, // 5th row of columns
        0b0000000000000000, // 4th row of columns
        0b0000000000000000, // 3nd row of columns
        0b0000000000000000, // 2nd row of columns
        0b0000000000000000, // 1st row of columns
        0b0000000100000000, // layer selection
    },
    {
        0b0000000000000000, // 9th row of columns
        0b0000000000000000, // 8th row of columns
        0b0000000000000000, // 7th row of columns
        0b0000000000000000, // 6th row of columns
        0b0000000000000000, // 5th row of columns
        0b0000000000000000, // 4th row of columns
        0b0000000000000000, // 3nd row of columns
        0b0000000000000000, // 2nd row of columns
        0b0000000000000000, // 1st row of columns
        0b0000000000000000, // layer selection
    }
};

void buffer_update(int i, int j, uint16_t newValue) {
    led_buffer[i][j] = newValue;
}


inline void led_set(uint8_t x, uint8_t y, uint8_t z)
{
    led_buffer[z][8-y] |= ( 1 << x );
}

inline void led_unset(uint8_t x, uint8_t y, uint8_t z)
{
    led_buffer[z][8-y] &= ~( 1 << x );
}


inline void led_toggle(uint8_t x, uint8_t y, uint8_t z)
{
    led_buffer[z][8-y] ^= ( 1 << x );
}


inline void led_set_state(uint8_t x, uint8_t y, uint8_t z, bool state)
{
    led_buffer[z][8-y] ^= (-state ^ led_buffer[z][8-y]) & ( 1 << x );
}


inline bool led_get(uint8_t x, uint8_t y, uint8_t z)
{
    return (bool) ( (led_buffer[z][8-y] >> x) & 1 );
}

extern SPI_HandleTypeDef hspi1;

bool led_update(int i)
{
    HAL_StatusTypeDef status;

    // populate the sink drivers
    status = HAL_SPI_Transmit(&hspi1, (uint8_t*)led_buffer[i], CUBE_WIDTH+1, SPI_TIMEOUT);
    // latch enable
    // TODO maybe have an other function do the latch enabling
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

    return (status==HAL_OK);
}

void led_clear(void)
{
    for (uint8_t z = 0; z < CUBE_WIDTH; z++)
	{
            for (uint8_t y = 0; y < CUBE_WIDTH; y++)
		{
                    for (uint8_t x = 0; x < CUBE_WIDTH; x++)
			{
                            led_unset(x,y,z);
			}
		}
	}
}
static int offset_test1 = 0;

void led_test1(void)
{
    offset_test1 = offset_test1 % 81;
    led_set(offset_test1/9, offset_test1%9,0);
    offset_test1 = offset_test1 + 1;
    if (offset_test1 == 81) {
	led_clear();
    }
    led_update(0);
}

static int offset_test2 = 0;
void led_test2(void)
{
    offset_test2 = offset_test2 % 81;
    led_set(offset_test2/9, offset_test2%9, 8);
    offset_test2 = offset_test2 + 1;
    if (offset_test2 == 81) {
	led_clear();
    }
    led_update(8);
}

static int offset_test3 = 0;
void led_test3(void)
{
    offset_test3 = offset_test3 % 81;
    led_set(offset_test3/9, offset_test3%9, 4);
    offset_test3 = offset_test3 + 1;
    if (offset_test3 == 81) {
	led_clear();
    }
    led_update(4);

}

void led_test_ok(void) 
{
    led_clear();

    // The O
    led_set(4, 1, 6);
    led_set(4, 2, 6);
    led_set(4, 3, 6);

    led_set(4, 0, 5);
    led_set(4, 0, 4);    
    led_set(4, 0, 3);

    led_set(4, 4, 5);
    led_set(4, 4, 4);    
    led_set(4, 4, 3);

    led_set(4, 1, 2);
    led_set(4, 2, 2);    
    led_set(4, 3, 2);

    // The K
    led_set(4, 6, 6);
    led_set(4, 6, 5);
    led_set(4, 6, 4);
    led_set(4, 6, 3);
    led_set(4, 6, 2);
    led_set(4, 8, 6);
    led_set(4, 7, 5);
    led_set(4, 7, 3);
    led_set(4, 7, 2);

    for (uint8_t z = 0; z < CUBE_WIDTH; z++) {

            led_update(z);
        }
    
    for (int i = 0; i < 100000000; ++i) {}
    led_clear();
}

void led_test_uncheck(void) 
{
    led_clear();

    for (uint8_t i = 0; i < CUBE_WIDTH; i++) {
        led_set(i, i, i);
        led_set(CUBE_WIDTH - i, CUBE_WIDTH - i, CUBE_WIDTH - i);
    }

    for (uint8_t z = 0; z < CUBE_WIDTH; z++) {

        led_update(z);
    }
    
    for (int i = 0; i < 100000000; ++i) {}
    led_clear();
}

void led_test_check(void) 
{
    led_clear();

    led_set(0, 0, 8);
    led_set(1, 1, 7);
    led_set(2, 2, 6);
    
    led_set(2, 2, 5);
    led_set(3, 3, 4);
    led_set(4, 4, 3);

    led_set(4, 4, 2);
    led_set(5, 5, 1);
    led_set(6, 6, 0);

    led_set(7, 7, 2);
    led_set(8, 8, 4);

    for (uint8_t z = 0; z < CUBE_WIDTH; z++) {

        led_update(z);
    }
    
    for (int i = 0; i < 100000000; ++i) {}
    led_clear();
}
