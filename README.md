# ESP32 Real-Time Embedded Systems

An embedded software project developed using an **ESP32**, progressing from a conventional Arduino-style implementation to a **FreeRTOS real-time multitasking architecture**.

The project was completed as part of my university Embedded Software coursework and explores deterministic signal generation, microsecond timing, digital I/O, frequency measurement, interrupts, task scheduling, semaphores and software timers.

The repository contains two implementations representing different stages of the coursework:

1. **Arduino Implementation** — timed waveform generation using a conventional `setup()` / `loop()` architecture.
2. **FreeRTOS Implementation** — a multi-task real-time system using FreeRTOS scheduling and synchronisation primitives.

---

## Technologies

- ESP32
- C/C++
- Arduino Framework
- FreeRTOS
- GPIO
- Hardware interrupts
- Binary semaphores
- FreeRTOS software timers
- Periodic task scheduling
- Microsecond timing
- Frequency measurement
- Real-time embedded systems

---

## Repository Structure

```text
esp32-realtime-embedded-systems/
│
├── README.md
├── LICENSE
│
├── arduino/
│   └── Arduino_IDE_Implementation_Code.ino
│
└── freertos/
    └── Arduino_FreeRTOS_Implementation.ino
```

The source files preserve the original coursework implementations.

---

# Part 1 — Arduino Implementation

The first part of the project uses a conventional Arduino execution model:

```text
setup()
   |
   v
loop()
   |
   +--> Read Buttons
   |
   +--> Select Waveform
   |
   +--> Generate DATA Signal
   |
   +--> Generate SYNC Signal
   |
   +--> Repeat
```

The implementation generates a timed digital waveform on an ESP32 while allowing the user to enable/disable the output and select between two waveform patterns using physical push buttons.

---

## Hardware I/O

Four ESP32 GPIO pins are used:

```text
GPIO 2   -> Output Enable Button
GPIO 4   -> Waveform Select Button
GPIO 32  -> DATA Signal Output
GPIO 25  -> SYNC Signal Output
```

The two push buttons are configured using the ESP32's internal pull-down resistors.

The DATA and SYNC pins provide the generated output signals.

---

# Waveform Generation

The DATA waveform consists of a sequence of pulses whose ON duration increases between successive pulses.

The original timing parameters are:

```text
Initial ON time     = 1300 µs
OFF time            = 900 µs
Number of pulses    = 11
ON-time increment   = 50 µs
Idle time           = 1500 µs
SYNC ON time        = 50 µs
```

For each pulse, the ON duration is calculated from the initial pulse duration and a fixed increment.

Conceptually:

```text
Pulse 1    Pulse 2     Pulse 3           Pulse N

┌─────┐    ┌──────┐    ┌───────┐         ┌──────────┐
│     │    │      │    │       │         │          │
┘     └────┘      └────┘       └── ... ──┘          └──
  TON1       TON2         TON3                 TON(N)

      <TOFF>      <TOFF>        <TOFF>
```

The pulse width therefore increases as the waveform progresses.

---

## Normal Waveform

The normal waveform generates all configured pulses.

```text
Normal Mode

Pulse 1
   |
   v
Pulse 2
   |
   v
Pulse 3
   |
   .
   .
   .
   |
   v
Pulse 11
   |
   v
Idle
   |
   v
SYNC
```

The DATA output is driven HIGH for the calculated pulse duration and LOW for the configured OFF period.

---

## Alternate Waveform

The second push button selects an alternate waveform.

The alternate implementation removes the final three pulses from the normal sequence.

```text
Normal:
1 2 3 4 5 6 7 8 9 10 11

Alternate:
1 2 3 4 5 6 7 8
```

This provides two different output patterns without duplicating the complete waveform-generation logic.

---

# SYNC Signal

After the DATA waveform has completed, a separate synchronisation pulse is generated.

```text
DATA waveform
      |
      v
   Idle Time
      |
      v
  SYNC HIGH
      |
     50 µs
      |
      v
   SYNC LOW
```

The implementation also accounts for the practical timing limit of `delayMicroseconds()` on the target platform by selecting an alternative millisecond delay when the scaled timing exceeds the configured threshold.

---

# Push-Button Control

Two physical buttons provide user control.

```text
Button 1
   |
   v
Enable / Disable
DATA Output


Button 2
   |
   v
Normal / Alternate
Waveform
```

Boolean state variables maintain the current system mode:

```text
outputEnabled
normalWaveform
```

This allows button presses to toggle system behaviour rather than requiring the button to remain held.

---

# Button Debouncing

The Arduino implementation includes software debouncing using `millis()`.

A configured debounce period prevents a single physical button press from being interpreted as multiple events due to contact bounce.

Conceptually:

```text
Button Edge
    |
    v
Has debounce
period elapsed?
   /       \
 No         Yes
 |           |
Ignore     Process
            |
            v
       Reset Timer
```

---

# Debug Timing Mode

The first implementation also contains a compile-time debugging option.

When:

```cpp
#define DEBUG
```

is enabled, waveform timing can be scaled by a large factor.

This allows signals that would normally occur over microseconds to be slowed sufficiently for visual observation using LEDs during development.

```text
Production
TIMING_FACTOR = 1

Debug
TIMING_FACTOR = 1000
```

This provides a useful distinction between development/debug behaviour and normal operating timing.

---

# Part 2 — FreeRTOS Implementation

The second part of the coursework moves from the sequential Arduino architecture to a **real-time multitasking implementation using FreeRTOS**.

Instead of executing all functionality sequentially inside `loop()`, individual system responsibilities are separated into independent tasks.

```text
                        FreeRTOS
                           |
      +--------------------+--------------------+
      |                    |                    |
      v                    v                    v
Signal Generation    Frequency Input       Monitoring
      |                    |                    |
   Task 1               Task 3               Task 5
   Task 2               Task 4               Task 6
                                                |
                                                v
                                           Button Task
```

The Arduino `loop()` is effectively unused because the application behaviour is handled by FreeRTOS tasks.

---

# FreeRTOS Tasks

The implementation creates several tasks with different priorities and execution periods.

Conceptually:

```text
Task 1
Digital Signal #1
Priority 3
Period: 4 ms

Task 2
Digital Signal #2
Priority 3
Period: 3 ms

Task 3
Frequency Measurement #1
Priority 2
Period: ~10 ms

Task 4
Frequency Measurement #2
Priority 2
Period: ~10 ms

Task 5
Monitoring Work
Priority 2
Period: 5 ms

Task 6
Frequency Threshold / LED
Priority 1
Period: 5 ms

Button Task
Semaphore-driven
Priority 1

Monitor Startup
One-shot startup task
Priority 1
```

This demonstrates the separation of time-critical and lower-priority functionality within a real-time system.

---

# Task Priorities

The two output waveform tasks are assigned the highest priority used by the application:

```text
Priority 3
├── Task 1
└── Task 2
```

Frequency measurement and monitoring tasks operate at:

```text
Priority 2
├── Task 3
├── Task 4
└── Task 5
```

Lower-priority functionality uses:

```text
Priority 1
├── Task 6
├── Button Task
└── Monitor Startup Task
```

This provides practical experience with one of the central concepts of an RTOS: deciding which work should receive processor time first when several tasks are ready to execute.

---

# Periodic Scheduling

Several tasks use:

```cpp
vTaskDelayUntil()
```

rather than a simple delay.

Conceptually:

```text
Expected Release Times

0 ms      4 ms      8 ms      12 ms
 |---------|---------|---------|
 Task 1    Task 1    Task 1    Task 1
```

`vTaskDelayUntil()` allows a task to execute relative to a defined periodic schedule rather than delaying for a fixed amount of time after its previous execution finishes.

This is useful for periodic real-time activities where maintaining the requested execution period is important.

---

# Digital Signal Generation

Two independent FreeRTOS tasks generate digital output patterns.

## Signal 1

Task 1 drives:

```text
GPIO 16
```

and produces a sequence containing:

```text
HIGH -> 250 µs
LOW  -> 50 µs
HIGH -> 300 µs
LOW
```

The task executes with an intended period of approximately:

```text
4 ms
```

---

## Signal 2

Task 2 drives:

```text
GPIO 17
```

with:

```text
HIGH -> 100 µs
LOW  -> 50 µs
HIGH -> 200 µs
LOW
```

with an intended period of approximately:

```text
3 ms
```

The two waveforms therefore operate independently as separate scheduled tasks.

---

# Frequency Measurement

Two further tasks measure incoming signals on:

```text
GPIO 34
GPIO 35
```

The implementation measures the duration of an incoming HIGH pulse using `pulseIn()`.

An estimated frequency is then calculated from the measured pulse duration.

```text
Incoming Signal
      |
      v
Measure HIGH Duration
      |
      v
Validate Measurement
      |
      v
Calculate Frequency
      |
      v
Store freq1 / freq2
```

The measured values are stored in volatile global variables because they are shared between tasks.

---

# Frequency Monitoring

Task 6 monitors the combined frequency:

```text
freq1 + freq2
```

If the total exceeds:

```text
1500
```

a status LED is switched ON.

Otherwise it remains OFF.

```text
freq1 + freq2
      |
      v
   > 1500?
   /     \
 Yes      No
  |        |
  v        v
LED ON   LED OFF
```

This demonstrates how data produced by separate real-time tasks can be consumed by another task to make a system-level decision.

---

# Interrupt-Driven Button Input

The FreeRTOS implementation handles the push button using a hardware interrupt rather than continuously polling it.

```text
Button Press
     |
     v
Hardware Interrupt
     |
     v
ButtonISR()
```

The interrupt service routine performs minimal processing and transfers the event into the FreeRTOS task environment.

This is an important embedded-systems design pattern because interrupt handlers should generally remain short and avoid unnecessary processing.

---

# ISR-to-Task Synchronisation

A FreeRTOS binary semaphore is used to communicate between the button interrupt and the task responsible for processing the button event.

```text
Physical Button
      |
      v
     ISR
      |
      v
Give Semaphore
      |
      v
Button Task Unblocks
      |
      v
Toggle LED
```

The ISR uses:

```cpp
xSemaphoreGiveFromISR()
```

while the button task waits using:

```cpp
xSemaphoreTake()
```

This demonstrates synchronisation between interrupt context and normal scheduled task context.

---

# FreeRTOS Software Timer

The button input also uses a FreeRTOS software timer for debouncing.

When an interrupt occurs:

```text
Button Interrupt
      |
      v
Disable Interrupt
      |
      v
Start Debounce Timer
      |
      v
Wait 50 ms
      |
      v
Timer Callback
      |
      v
Re-enable Interrupt
```

This avoids performing the debounce wait inside the interrupt handler.

It also demonstrates an improvement over blocking debounce approaches by delegating the timing operation to the RTOS.

---

# Task Monitoring

The project uses the coursework-provided:

```cpp
B31DGMonitor
```

library.

Several tasks call:

```cpp
monitor.jobStarted(...)
monitor.jobEnded(...)
```

around their execution.

A dedicated startup task waits briefly before calling:

```cpp
monitor.startMonitoring()
```

This provides instrumentation around the execution of the real-time tasks.

The monitoring component was provided as part of the coursework environment rather than being implemented within this repository.

---

# Architecture Comparison

One of the most useful aspects of the project is the progression between the two implementations.

## Arduino Implementation

```text
             loop()
               |
      +--------+--------+
      |                 |
      v                 v
Read Buttons       Generate Signal
                        |
                        v
                   Generate SYNC
                        |
                        v
                      Repeat
```

The application is primarily sequential.

---

## FreeRTOS Implementation

```text
                   FreeRTOS Scheduler
                          |
       +------------------+------------------+
       |                  |                  |
       v                  v                  v
 Output Tasks       Measurement Tasks     Monitoring
       |                  |                  |
       |                  |                  v
       |                  |             Decision Task
       |                  |
       +------------------+------------------+
                          |
                          v
                  Shared System State

Button
   |
   v
Interrupt
   |
   v
Semaphore
   |
   v
Button Task
```

The application is decomposed into independently scheduled activities.

---

# Concepts Demonstrated

This project provided practical experience with:

- ESP32 programming
- C/C++
- Arduino
- FreeRTOS
- Real-time embedded software
- GPIO
- Digital waveform generation
- Microsecond timing
- Periodic scheduling
- Task priorities
- Multitasking
- Hardware interrupts
- Interrupt service routines
- Binary semaphores
- ISR-to-task communication
- Software timers
- Button debouncing
- Frequency measurement
- Shared state
- Embedded debugging
- Real-time system monitoring

---

# Original Implementation

The source code in this repository preserves the original university coursework implementations.

The files have not been extensively rewritten to make them appear representative of my current embedded-software development practices.

They are retained as evidence of my progression from conventional microcontroller programming toward concurrent and real-time embedded software.

---

# Known Limitations

Reviewing the original implementation highlights several areas that could be improved.

### Timing Precision

The waveform tasks combine FreeRTOS millisecond scheduling with `delayMicroseconds()` for shorter signal timing.

For applications requiring strict deterministic timing, dedicated ESP32 hardware peripherals or timers could provide greater precision and reduce CPU occupation.

### Blocking Frequency Measurement

The frequency-measurement tasks use `pulseIn()`.

Although a timeout is configured, this remains a blocking measurement approach.

A more robust real-time implementation could use:

- Hardware interrupts
- Input capture
- ESP32 pulse-counter peripherals
- Dedicated timer peripherals

to measure frequency without blocking a scheduled task.

### Shared Data

`freq1` and `freq2` are declared `volatile`, allowing the compiler to account for asynchronous modification.

For a larger concurrent system, access to shared multi-task state would be designed more explicitly using appropriate RTOS synchronisation mechanisms where required.

### Button State

The implementation retains a `buttonPressed` variable from an alternative debounce approach, while the final button handling uses the binary semaphore and software timer.

This could be removed during code cleanup.

### Error Checking

A production implementation would check whether FreeRTOS resources were successfully created.

For example:

```text
Task Creation
Semaphore Creation
Timer Creation
```

would all be validated before continuing execution.

### Core Allocation

The recovered implementation pins the application tasks to ESP32 core 1.

A larger design would explicitly evaluate task affinity based on timing requirements, system services and workload rather than assigning all tasks to the same core by default.

---

# How I Would Approach It Today

With my current embedded and robotics experience, I would retain the task-based architecture but make greater use of hardware-supported timing.

For example:

```text
                ESP32
                  |
     +------------+------------+
     |            |            |
     v            v            v
 Hardware      FreeRTOS      Interrupts
 Timers         Tasks
     |            |            |
     v            v            v
Waveforms     Decisions      Events
```

Hardware timers or waveform peripherals would handle highly deterministic signals, while FreeRTOS tasks would perform higher-level processing.

---

## Event-Driven Frequency Measurement

Instead of:

```text
Task
 |
 v
pulseIn()
 |
 v
Wait for Signal
```

I would favour:

```text
Signal Edge
    |
    v
Hardware Capture / ISR
    |
    v
Timestamp
    |
    v
Queue / Notification
    |
    v
Processing Task
    |
    v
Frequency Estimate
```

This reduces blocking and provides a cleaner separation between time-critical acquisition and higher-level processing.

---

## RTOS Communication

For a larger application, communication between tasks could use explicit FreeRTOS mechanisms such as:

```text
Queues
Semaphores
Mutexes
Event Groups
Task Notifications
```

depending on the type of information being exchanged.

This would minimise reliance on shared global state.

---

# Portfolio Context

This project demonstrates an important progression in my embedded-software experience.

```text
Early Arduino
     |
     v
Sequential Embedded Control
     |
     v
ESP32 Timing / GPIO
     |
     v
Interrupts
     |
     v
FreeRTOS
     |
     v
Concurrent Real-Time Systems
```

The first implementation demonstrates direct microcontroller control and timing.

The second introduces a fundamentally different software architecture based on:

- Independent tasks
- Priorities
- Periodic scheduling
- Interrupts
- Synchronisation
- Software timers
- Shared real-time data

These concepts are directly applicable to larger robotics and autonomous systems, where perception, communication, control and hardware interfaces frequently need to execute concurrently under timing constraints.
