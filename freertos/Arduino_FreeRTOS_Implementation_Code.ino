// ====== Include Libraries ======
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/semphr.h>
#include <B31DGMonitor.h>

// ====== Pin Definitions ======
#define SIGNAL1_PIN 16 // Output pin for Task 1
#define SIGNAL2_PIN 17 // Output pin for Task 2
#define FREQ1_PIN 34 // Input pin for Task 3
#define FREQ2_PIN 35 // Input pin for Task 4
#define LED_PIN 2 // Status LED for Task 6
#define BUTTON_PIN 4 // Button Input for Task 7
#define LED_TOGGLE_PIN 15 // LED Toggle by button for Task 7

// ====== Monitor Instance ======
B31DGCyclicExecutiveMonitor monitor;

// ====== Frequency Tracking ======
volatile unsigned long freq1 = 0;
volatile unsigned long freq2 = 0;

// ====== Button Task Debounce & Synchronisation ======
#define DEBOUNCE_DELAY_MS 50
TimerHandle_t debounceTimer;
SemaphoreHandle_t buttonSemaphore;
volatile bool buttonPressed = false;

// ====== Function Prototypes ======
void Task1(void *pvParameters);
void Task2(void *pvParameters);
void Task3(void *pvParameters);
void Task4(void *pvParameters);
void Task5(void *pvParameters);
void ButtonISR();
void TaskButtonMonitor(void *pvParameters);
void StartMonitorTask(void *pvParameters);

// ====== Setup Function ======
void setup() {
  Serial.begin(115200);

  // ====== Pin COnfigurations ======
  pinMode(SIGNAL1_PIN, OUTPUT);
  pinMode(SIGNAL2_PIN, OUTPUT);
  pinMode(FREQ1_PIN, INPUT);
  pinMode(FREQ2_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_TOGGLE_PIN, OUTPUT);

  // ====== Initialise Semaphores and attach Interrupt ======
  buttonSemaphore = xSemaphoreCreateBinary();
  // Create debounce timer 
  debounceTimer = xTimerCreate("DebounceTimer", pdMS_TO_TICKS(DEBOUNCE_DELAY_MS), pdFALSE, (void *)0, debounceTimerCallback);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), ButtonISR, FALLING);
  //attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), ButtonISR, FALLING);

  // ====== Creat Real-time tasks with appropriate priorities and assign to core ======

  xTaskCreatePinnedToCore(Task1, "Task1", 2048, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(Task2, "Task2", 2048, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(Task3, "Task3", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(Task4, "Task4", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(Task5, "Task5", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(Task6, "Task6", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskButtonMonitor, "ButtonTask", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(StartMonitorTask, "StartMonitor", 2048, NULL, 1, NULL, 1);

}

// ====== Main Loop ======
void loop() {
  //Serial.printf("[DEBUG] freq1: %lu | freq2: %lu | sum: %lu\n", freq1, freq2, freq1 + freq2);
 
}

// ====== Start Monitor Task ======
void StartMonitorTask(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(100));
  monitor.startMonitoring();
  vTaskDelete(NULL);
}

// ====== Task 1 (Output Digital Signal #1) ======
void Task1(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(20)); // warm up delay
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(4);
  for (;;) {
    monitor.jobStarted(1);
    digitalWrite(SIGNAL1_PIN, HIGH);
    delayMicroseconds(250);
    digitalWrite(SIGNAL1_PIN, LOW);
    delayMicroseconds(50);
    digitalWrite(SIGNAL1_PIN, HIGH);
    delayMicroseconds(300);
    digitalWrite(SIGNAL1_PIN, LOW);
    monitor.jobEnded(1);
    vTaskDelayUntil(&xLastWakeTime, xFrequency); // 4ms period
  }
}

// ====== Task 2 (Output Digital Signal #2) ======
void Task2(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(20)); // warm up delay
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(3);

  for (;;) {
    monitor.jobStarted(2);
    digitalWrite(SIGNAL2_PIN, HIGH);
    delayMicroseconds(100);
    digitalWrite(SIGNAL2_PIN, LOW);
    delayMicroseconds(50);
    digitalWrite(SIGNAL2_PIN, HIGH);
    delayMicroseconds(200);
    digitalWrite(SIGNAL2_PIN, LOW);
    monitor.jobEnded(2);
    vTaskDelayUntil(&xLastWakeTime, xFrequency); // 3ms period
  }
}

// ====== Task 3 (Frequency Measurement #1)======
void Task3(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(20));
  for (;;) {
    monitor.jobStarted(3);
    unsigned long duration = pulseIn(FREQ1_PIN, HIGH, 200);
    if (duration > 100 && duration < 2000) {
      freq1 = 1000000 / (2*duration);
    }
    monitor.jobEnded(3);
    vTaskDelay(pdMS_TO_TICKS(10)); // 10ms period
  }
}

// ====== Task 4 (Frequency Measurement #2)======
void Task4(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(20));
  for (;;) {
    monitor.jobStarted(4);
    unsigned long duration = pulseIn(FREQ2_PIN, HIGH, 400);
    if (duration > 100 && duration < 2000) {
      freq2 = 1000000 / (2*duration);
    }
    monitor.jobEnded(4);
    vTaskDelay(pdMS_TO_TICKS(10)); // 10ms period
  }
}

// ====== Task 5 (Call monitors' doWork() Method) ======
void Task5(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(20));
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(5);

  for (;;) {
    monitor.jobStarted(5);
    monitor.doWork();
    monitor.jobEnded(5);
    vTaskDelayUntil(&xLastWakeTime, xFrequency); // 5ms period
  }
}

// // ====== Task 6 (Monitor freq1 + freq2 and toggle LED_PIN) ======
void Task6(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(20)); // warm up delay
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(5);

  for (;;) {
    //monitor.jobStarted(6);

    if ((freq1 + freq2) > 1500) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }

    //monitor.jobEnded(6);
    vTaskDelayUntil(&xLastWakeTime, xFrequency); // 5ms period
  }
}

// ====== ISR Button Press Handler ======
void IRAM_ATTR ButtonISR() {
  buttonPressed = true;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  // Disable furthe interrupts
  detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));
  // Start debounce timer
  xTimerStartFromISR(debounceTimer, &xHigherPriorityTaskWoken);
  // Give semaphore to signal button press
  xSemaphoreGiveFromISR(buttonSemaphore, &xHigherPriorityTaskWoken);

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// ====== Debounce Timer Function ======
// To re-enable the interrupt after the debounce period
void debounceTimerCallback(TimerHandle_t xTimer) {
  // Re-attach the interrupt
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), ButtonISR, FALLING);
}

// ====== Task 7 (Button Monitoring & Toggle LED)
void TaskButtonMonitor(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(20));
  for (;;) {
    if (xSemaphoreTake(buttonSemaphore, portMAX_DELAY)) {
      digitalWrite(LED_TOGGLE_PIN, !digitalRead(LED_TOGGLE_PIN));
    }
  }
// 
// void TaskButtonMonitor(void *pvParameters) {
//   vTaskDelay(pdMS_TO_TICKS(20)); // startup delay
//   static TickType_t lastDebounceTime = 0;
//   const TickType_t debounceDelay = pdMS_TO_TICKS(50); // 50ms debounce

//   for (;;) {
//     if (buttonPressed) {
//       buttonPressed = false;  // clear the flag immediately

//       TickType_t now = xTaskGetTickCount();
//       if ((now - lastDebounceTime) >= debounceDelay) {
//         // Toggle the LED
//         digitalWrite(LED_TOGGLE_PIN, !digitalRead(LED_TOGGLE_PIN));
//         lastDebounceTime = now;
//       }
//     }

//     vTaskDelay(pdMS_TO_TICKS(1)); // slight delay to yield CPU
//   }
}
