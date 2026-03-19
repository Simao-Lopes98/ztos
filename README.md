# ZTOS
Project to learn and develop a small RTOS along with other bare-metal contents.

Currently only supports - ARM Cortex M3

## Getting Started

### Prerequisites

Install the following tools to compile and program STM32 devices via STLink:

```bash
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch openocd stlink-tools make
```

## Project Structure

```
ZTOS/
├── linker.ld
├── Makefile
├── README.md
├── app/
│   ├── include/
│   └── src/
├── build/
│   ├── app/
│   ├── drivers/
│   ├── HAL/
│   └── startup/
├── docs/
├── drivers/
│   ├── include/
│   └── src/
├── HAL/
│   ├── include/
│   └── src/
├── startup/
│   ├── include/
│   └── src/
└── ZTOS
    ├── include/
    └── src/
```

## Roadmap for ZTOS

- [X] Initialize System Clock and MCU Configuration
- [X] Set up Hardware Timers for Tick Generation
- [X] Implement Tick Interrupt Handler
- [X] Create Task Control Block (TCB) Structure
- [ ] Implement Context Switching Mechanism
- [X] Develop Task Scheduler (Round Robin / Priority-based)
- [X] Add Task Creation and Deletion Functions
- [X] Implement Task States (Ready, Running, Blocked, Suspended)
- [ ] Create Synchronization Primitives (Semaphores)
- [ ] Implement Mutexes for Resource Protection
- [ ] Add Intertask Communication (Message Queues)
- [ ] Develop Memory Management (Heap/Stack Allocation)
- [ ] Add Task Priority Management
- [ ] Implement System Time and Delay Functions
- [ ] Add Error Handling and Exception Management
- [ ] Create Debugging and Logging Utilities

---

## Roadmap for HAL

- [ ] Add GPIO library
- [X] Add Timer library
- [ ] Update USART library

## Roadmap for drivers

- [ ] Add DMA support
- [ ] Add Singly Linked List Lib

### Build

To build use the following:
```bash
make
```
This will create the object files and the .elf file. Additionally, dependency and .map files are generated.

### Flash

To flash to the MCU use the following:
```bash
make flash
```

### Clean

To clear all object and elf files:
```bash
make clean
```

## GBD

On Terminal 1. Run the following command to "talk" to the SoC through the programmer:
```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg
```
On Terminal 2. Run the following command to load the .elf on the build/ dir
```bash
gdb-multiarch arm_dev.elf
```
The following commands are done on Terminal 2.

* Connect: target remote :3333
* Reset: monitor reset halt
* Flash: load arm_dev.elf

### Documentation

Refer to the project documentation for system clarification.