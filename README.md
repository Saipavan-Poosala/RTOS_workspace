# STM32F401 + FreeRTOS practice projects

Personal learning projects, written while learning the base layer under my day job.
I work as a software system designer on automotive ECUs (BSW/AUTOSAR, CAN, UDS, diagnostics).
These projects are how I get hands-on with an RTOS, interrupts and peripherals on a real MCU.

They are practice code, not production firmware.

## Hardware and tools

- Board: RaayanMini v4.0/4.1 (STM32F401RB, Cortex-M4F, 16 MHz HSI, no PLL)
- IDE: STM32CubeIDE, GCC for ARM
- RTOS: FreeRTOS (GCC/ARM_CM4F port, `heap_4`)
- HAL time base moved to TIM5 so SysTick belongs to FreeRTOS only

## Projects

### 001Tasks: two tasks, same priority
Two tasks of priority 2 print a string and call `taskYIELD()`.
Used to trace the scheduler by hand: `vTaskStartScheduler` -> `xPortStartScheduler` -> `svc 0`
-> `SVC_Handler`, then every 1 ms `SysTick_Handler` -> `xTaskIncrementTick` -> `PendSV_Handler`.

### 002LED_Tasks: task notifications
Four tasks, driven by the user button.

| Task | Priority | Output | Period |
|---|---|---|---|
| Button | 4 | PC10 (polled, pull-up, pressed = 0) | 10 ms |
| Green LED | 3 | PB14 (active low) | 1000 ms |
| Buzzer | 2 | PB12 | 800 ms |
| Red LED | 1 | PB13 (active low) | 400 ms |

Each output task waits with `xTaskNotifyWait()` instead of a plain delay. A timeout toggles the output.
A notification makes the task hand over to the next one and delete itself.
Presses go green, then buzzer, then red. After the third press the button task deletes itself as well.
`next_task_handle` is shared data, so it is only changed while the scheduler is suspended.

Things I tried along the way: `HAL_Delay` (busy wait) vs `vTaskDelay` vs `vTaskDelayUntil`.

## Layout

```
001Tasks/        CubeIDE project
002LED_Tasks/    CubeIDE project
common/          FreeRTOS sources shared by the projects (linked into each project)
```

Build output (`Debug/`) is not committed. Open a project in STM32CubeIDE and build the Debug configuration.

## Not verified

- The buzzer is assumed active-high (pin high = on).
- No debouncing beyond the 10 ms poll.

## Licenses

FreeRTOS is MIT licensed. The STM32 HAL and CMSIS files are from STMicroelectronics and keep their own licenses.
