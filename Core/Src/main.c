/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sch.h"
#include "stm32f1xx_hal_gpio_ex.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_ON(pin)      HAL_GPIO_WritePin(GPIOA, (pin), GPIO_PIN_RESET)
#define LED_OFF(pin)     HAL_GPIO_WritePin(GPIOA, (pin), GPIO_PIN_SET)
#define LED_TOGGLE(pin)  HAL_GPIO_TogglePin(GPIOA, (pin))
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
static void Debug_UART_Init(void);
static void Task_Print_Time(void);
static void Task_Button(void);
static void Task_LED_0_5s(void);
static void Task_LED_1s(void);
static void Task_LED_1_5s(void);
static void Task_LED_2s(void);
static void Task_LED_2_5s(void);
static void Demo_OneShot_A(void);
static void Demo_OneShot_B(void);
static void Demo_Cancelled(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void Debug_UART_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint32_t pclk2;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    /* USART1 remap: TX is PB6, leaving PA2/PA3/PA9 for the LEDs. */
    __HAL_AFIO_REMAP_USART1_ENABLE();

    gpio.Pin = GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    USART1->CR1 = 0U;
    USART1->CR2 = 0U;
    USART1->CR3 = 0U;

    /* 8 MHz PCLK2, 57600 baud; calculate BRR from the actual PCLK2. */
    pclk2 = HAL_RCC_GetPCLK2Freq();
    USART1->BRR = (pclk2 + 28800U) / 57600U;
    USART1->CR1 = USART_CR1_TE | USART_CR1_UE;
}

/* syscalls.c routes printf through this function. */
int __io_putchar(int character)
{
    while ((USART1->SR & USART_SR_TXE) == 0U)
    {
    }

    USART1->DR = (uint16_t)(uint8_t)character;
    return character;
}

static void Task_Print_Time(void)
{
    printf("get_time = %lu ms\r\n", (unsigned long)get_time());
}

static void Task_Button(void)
{
    /* Three equal 10 ms samples debounce the active-low button. */
    static uint8_t candidate = 1U;
    static uint8_t stable_state = 1U;
    static uint8_t sample_count = 0U;
    static uint8_t led_button_on = 0U;
    uint8_t raw_state =
        (HAL_GPIO_ReadPin(button_GPIO_Port, button_Pin) == GPIO_PIN_SET) ? 1U : 0U;

    if (raw_state != candidate)
    {
        candidate = raw_state;
        sample_count = 1U;
        return;
    }

    if (sample_count < 3U)
    {
        sample_count++;
    }

    if ((sample_count >= 3U) && (candidate != stable_state))
    {
        stable_state = candidate;

        /* Toggle D6 once on the debounced press edge; release does not undo it. */
        if (stable_state == 0U)
        {
            led_button_on ^= 1U;
            if (led_button_on != 0U)
            {
                LED_ON(led_button_Pin);
                printf("button toggled D6 ON at %lu ms\r\n",
                       (unsigned long)get_time());
            }
            else
            {
                LED_OFF(led_button_Pin);
                printf("button toggled D6 OFF at %lu ms\r\n",
                       (unsigned long)get_time());
            }
        }
    }
}

static void Task_LED_0_5s(void) { LED_TOGGLE(led_0_5s_Pin); }
static void Task_LED_1s(void)   { LED_TOGGLE(led_1s_Pin); }
static void Task_LED_1_5s(void) { LED_TOGGLE(led_1_5s_Pin); }
static void Task_LED_2s(void)   { LED_TOGGLE(led_2s_Pin); }
static void Task_LED_2_5s(void) { LED_TOGGLE(led_2_5s_Pin); }

static void Demo_OneShot_A(void)
{
    printf("one-shot A at %lu ms\r\n", (unsigned long)get_time());
}

static void Demo_OneShot_B(void)
{
    printf("one-shot B at %lu ms\r\n", (unsigned long)get_time());
}

static void Demo_Cancelled(void)
{
    printf("ERROR: deleted task ran at %lu ms\r\n", (unsigned long)get_time());
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */
    uint32_t cancelled_task_id;
    /* USER CODE END 1 */

    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    MX_GPIO_Init();
    MX_TIM2_Init();

    /* USER CODE BEGIN 2 */
    /* All six LEDs are wired to +3.3 V and are active-low: high means OFF. */
    HAL_GPIO_WritePin(GPIOA,
                      led_0_5s_Pin | led_1s_Pin | led_1_5s_Pin | led_2s_Pin |
                      led_2_5s_Pin | led_button_Pin,
                      GPIO_PIN_SET);

    Debug_UART_Init();
    SCH_Init();

    /* One callback each 10 ms prints the scheduler's time base. */
    (void)SCH_Add_Task(Task_Print_Time, 1U, 1U);

    /* Required periods: 0.5, 1.0, 1.5, 2.0 and 2.5 seconds. */
    (void)SCH_Add_Task(Task_LED_0_5s, 50U, 50U);
    (void)SCH_Add_Task(Task_LED_1s, 100U, 100U);
    (void)SCH_Add_Task(Task_LED_1_5s, 150U, 150U);
    (void)SCH_Add_Task(Task_LED_2s, 200U, 200U);
    (void)SCH_Add_Task(Task_LED_2_5s, 250U, 250U);

    /* Button scan/debounce also runs every 10 ms. */
    (void)SCH_Add_Task(Task_Button, 1U, 1U);

    /* Two one-shot timeouts, both registered before the main loop. */
    (void)SCH_Add_Task(Demo_OneShot_A, 20U, 0U); /* 200 ms */
    (void)SCH_Add_Task(Demo_OneShot_B, 35U, 0U); /* 350 ms */

    /* Exercise Delete_Task: this callback must never run. */
    cancelled_task_id = SCH_Add_Task(Demo_Cancelled, 70U, 0U);
    if ((cancelled_task_id == SCH_INVALID_TASK_ID) ||
        (SCH_Delete_Task(cancelled_task_id) != 0U))
    {
        Error_Handler();
    }

    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE END 2 */

    while (1)
    {
        /* USER CODE BEGIN WHILE */
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        SCH_Dispatch_Tasks();
        /* USER CODE END 3 */
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 |
                                  RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
    TIM_ClockConfigTypeDef clock_source = {0};
    TIM_MasterConfigTypeDef master_config = {0};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 7999U;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 9U;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    clock_source.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &clock_source) != HAL_OK)
    {
        Error_Handler();
    }

    master_config.MasterOutputTrigger = TIM_TRGO_RESET;
    master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &master_config) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOA,
                      led_0_5s_Pin | led_1s_Pin | led_1_5s_Pin | led_2s_Pin |
                      led_2_5s_Pin | led_button_Pin,
                      GPIO_PIN_RESET);

    /*Configure GPIO pins : LEDs */
    GPIO_InitStruct.Pin = led_0_5s_Pin | led_1s_Pin | led_1_5s_Pin |
                          led_2s_Pin | led_2_5s_Pin | led_button_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pin : button */
    GPIO_InitStruct.Pin = button_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(button_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        SCH_Update();
    }
}
/* USER CODE END 4 */

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif