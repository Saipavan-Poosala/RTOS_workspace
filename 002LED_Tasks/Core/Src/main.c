/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "task.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* LED wiring: confirm against your board schematic before running.
 * Both LEDs are wired pin -> resistor -> UVCC, so they light when the pin is LOW. */
#define LED_PORT            GPIOB
#define LED_GREEN_PIN       GPIO_PIN_14
#define LED_RED_PIN         GPIO_PIN_13
#define LED_ON              GPIO_PIN_RESET
#define LED_OFF             GPIO_PIN_SET

/* Buzzer on PB12 (through a transistor), used as the "orange" task output.
 * Assumed active-high (pin HIGH = buzzer on): not verified, check the schematic. */
#define BUZZER_PORT         GPIOB
#define BUZZER_PIN          GPIO_PIN_12
#define BUZZER_ON           GPIO_PIN_SET
#define BUZZER_OFF          GPIO_PIN_RESET

/* User button SW_ENT on PC10: wired to ground, so internal pull-up, pressed = 0 */
#define BUTTON_PORT         GPIOC
#define BUTTON_PIN          GPIO_PIN_10
#define BUTTON_PRESSED      GPIO_PIN_RESET

#define LED_GREEN_PERIOD_MS 1000
#define BUZZER_PERIOD_MS    800
#define LED_RED_PERIOD_MS   400
#define BUTTON_POLL_MS      10
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
TaskHandle_t ledg_task_handle;
TaskHandle_t buzzer_task_handle;
TaskHandle_t ledr_task_handle;
TaskHandle_t btn_task_handle;

/* Shared between the button task (reads) and the LED tasks (write).
 * Written only while the scheduler is suspended. NULL = nothing left to notify. */
TaskHandle_t volatile next_task_handle;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void gpio_init(void);
static void led_green_handler(void* parameters);
static void buzzer_handler(void* parameters);
static void led_red_handler(void* parameters);
static void button_handler(void* parameters);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  BaseType_t status;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  /* USER CODE BEGIN 2 */
  gpio_init();

  /* Notification chain: button -> green -> buzzer -> red */
  status = xTaskCreate(led_green_handler, "LED_green_task", 200, NULL, 3, &ledg_task_handle);
  configASSERT(status == pdPASS);

  next_task_handle = ledg_task_handle;

  status = xTaskCreate(buzzer_handler, "Buzzer_task", 200, NULL, 2, &buzzer_task_handle);
  configASSERT(status == pdPASS);

  status = xTaskCreate(led_red_handler, "LED_red_task", 200, NULL, 1, &ledr_task_handle);
  configASSERT(status == pdPASS);

  status = xTaskCreate(button_handler, "Button_task", 200, NULL, 4, &btn_task_handle);
  configASSERT(status == pdPASS);

  //start the freeRTOS scheduler
  vTaskStartScheduler();

  //if the control comes here then the launch of the scheduler has failed due to insufficient memory
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

static void gpio_init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* Start with both LEDs off (pin high, active-low) and the buzzer off. */
  HAL_GPIO_WritePin(LED_PORT, LED_GREEN_PIN | LED_RED_PIN, LED_OFF);
  HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, BUZZER_OFF);

  gpio.Pin   = LED_GREEN_PIN | LED_RED_PIN | BUZZER_PIN;
  gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio.Pull  = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PORT, &gpio);          // LED_PORT and BUZZER_PORT are both GPIOB

  gpio.Pin   = BUTTON_PIN;
  gpio.Mode  = GPIO_MODE_INPUT;
  gpio.Pull  = GPIO_PULLUP;                // button connects the pin to ground
  HAL_GPIO_Init(BUTTON_PORT, &gpio);
}

/* Each LED/buzzer task: toggle its output, then wait for a notification for one
 * period. The wait is the delay: timeout (pdFALSE) = toggle again, notification
 * (pdTRUE) = hand over to the next task in the chain and delete itself. */
static void led_green_handler(void* parameters)
{
  BaseType_t status;

  while(1)
  {
    HAL_GPIO_TogglePin(LED_PORT, LED_GREEN_PIN);

    status = xTaskNotifyWait(0, 0, NULL, pdMS_TO_TICKS(LED_GREEN_PERIOD_MS));
    if(status == pdTRUE)
    {
      vTaskSuspendAll();
      next_task_handle = buzzer_task_handle;
      xTaskResumeAll();

      HAL_GPIO_WritePin(LED_PORT, LED_GREEN_PIN, LED_OFF);
      vTaskDelete(NULL);
    }
  }
}

static void buzzer_handler(void* parameters)
{
  BaseType_t status;

  while(1)
  {
    HAL_GPIO_TogglePin(BUZZER_PORT, BUZZER_PIN);

    status = xTaskNotifyWait(0, 0, NULL, pdMS_TO_TICKS(BUZZER_PERIOD_MS));
    if(status == pdTRUE)
    {
      vTaskSuspendAll();
      next_task_handle = ledr_task_handle;
      xTaskResumeAll();

      HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, BUZZER_OFF);
      vTaskDelete(NULL);
    }
  }
}

static void led_red_handler(void* parameters)
{
  BaseType_t status;

  while(1)
  {
    HAL_GPIO_TogglePin(LED_PORT, LED_RED_PIN);

    status = xTaskNotifyWait(0, 0, NULL, pdMS_TO_TICKS(LED_RED_PERIOD_MS));
    if(status == pdTRUE)
    {
      vTaskSuspendAll();
      next_task_handle = NULL;               // last task: nothing left to notify
      xTaskResumeAll();

      HAL_GPIO_WritePin(LED_PORT, LED_RED_PIN, LED_OFF);
      vTaskDelete(NULL);
    }
  }
}

/* Polls the button every 10 ms. A press is the moment the level changes from
 * released to pressed, so holding the button sends only one notification. */
static void button_handler(void* parameters)
{
  uint8_t btn_read = 0;
  uint8_t prev_read = 0;

  while(1)
  {
    btn_read = (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == BUTTON_PRESSED);

    if(btn_read && !prev_read)
    {
      if(next_task_handle != NULL)
      {
        TaskHandle_t target = next_task_handle;
        xTaskNotify(target, 0, eNoAction);

        /* Press 3 notifies the red task, the last one in the chain.
         * Nothing is left to control, so the button task deletes itself too. */
        if(target == ledr_task_handle)
        {
          vTaskDelete(NULL);
        }
      }
    }
    prev_read = btn_read;
    vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
  }
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM5 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM5)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
