/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#define DEBUG_THIS_FILE    DEBUG_FREERTOS_FILE
#include "debug.h"
#include "rng.h"
#include "audio_playback.h"
#include "adc.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NUM_ACTIONS   (6)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
int buttonPressed = 0;
int twisted = 0;
int bopped = 0;
int blown = 0;
int listenBlow = 0;
int cooldown = 0;
int temp = 10000;

int16_t accXYZ[3];
float gyroXYZ[3];
int threshold = 400000;
/* USER CODE END Variables */
osThreadId gameLogicHandle;
osThreadId SensorPollsHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartTask02(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of gameLogic */
  osThreadDef(gameLogic, StartDefaultTask, osPriorityNormal, 0, 128);
  gameLogicHandle = osThreadCreate(osThread(gameLogic), NULL);

  /* definition and creation of SensorPolls */
  osThreadDef(SensorPolls, StartTask02, osPriorityNormal, 0, 128);
  SensorPollsHandle = osThreadCreate(osThread(SensorPolls), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the gameLogic thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  uint8_t successfull_actions = 0;
  play_start_part1_sample(); 
  osDelay(1400); 
  play_start_part2_sample();
  osDelay(1500);
  /* Infinite loop */
  for(;;)
  {

    // generate a random number, take the modulo 3 to obtain either 0 or 1
    uint32_t randomValue = 0;
    HAL_RNG_GenerateRandomNumber(&hrng, &randomValue);
    randomValue = randomValue%3;
    debugf("Random Number is = %ld\r\n", randomValue);
    if(randomValue == 0 && cooldown == 0){ // Blow It
      printf("Blow It\n");
      play_blow_it_sample();
      cooldown = 5;
      temp = get_temperature();
      printf("%d\n",temp);
      listenBlow = 1;
      osDelay(5000);
      if(bopped == 0 && twisted == 0 && blown == 1){
        successfull_actions++;
        play_success_sample();
        listenBlow = 0;
        osDelay(400); // success sample is 300ms long, waiting for it to play out
      }else {
        play_fail_sample();
        printf("GAME ENDED\r\n");
        break;
      }
    }else if(randomValue == 1){ // Bop It
      printf("Bop It\n");
      play_bop_it_sample();
      osDelay(2000);
      if (cooldown > 0){
        cooldown = cooldown - 1;
      }
      if(bopped == 1 && twisted == 0 && blown ==0){
        successfull_actions++;
        play_success_sample();
        osDelay(400); // success sample is 300ms long, waiting for it to play out
      }else {
        play_fail_sample();
        printf("GAME ENDED\r\n");
        break;
      }
    }else { // Twist It
      printf("Twist It\n");
      play_twist_it_sample();
      osDelay(2000);
      if (cooldown > 0){
        cooldown = cooldown - 1;
      }
      if(bopped == 0 && twisted == 1 && blown ==0){
        successfull_actions++;
        play_success_sample();
        osDelay(400);
      }else {
        play_fail_sample();
        printf("GAME ENDED\r\n");
        break;
      }
    }
    bopped = 0;
    twisted = 0;
    blown = 0;

    if (successfull_actions == NUM_ACTIONS) {
      printf("WON!!!!\n");
      play_success_sample();
      osDelay(250);
      play_success_sample();
      osDelay(250);
      play_success_sample();
      osDelay(250);
      play_win_sample();
      break;
    }


  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the SensorPolls thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void const * argument)
{
  /* USER CODE BEGIN StartTask02 */
  /* Infinite loop */
  for(;;)
  {
    BSP_GYRO_GetXYZ(gyroXYZ);

    // if the gyroscope reads a value greater than the threshold then set "Twisted" to 1
    if(gyroXYZ[0]<-threshold || gyroXYZ[0]>threshold || gyroXYZ[1]<-threshold || gyroXYZ[1]>threshold || gyroXYZ[2]<-threshold || gyroXYZ[2]>threshold){
      printf("HARD TWIST DETECTED!!!!!\r\n");
      twisted = 1;
    }
    int temp2 = get_temperature();
    if (listenBlow == 0) {
      temp = temp2;
    }
    else if(temp2 > temp && temp2<1000 && blown == 0){
      printf("Blowing detected! %d \n",temp2);
      blown = 1;
    }


    osDelay(10);

  }
  /* USER CODE END StartTask02 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {

  bopped = 1;
}
/* USER CODE END Application */
