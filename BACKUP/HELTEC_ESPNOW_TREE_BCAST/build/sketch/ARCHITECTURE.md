#line 1 "/mnt/6E9A84429A8408B3/_VSC/WEB_UPLOAD/HELTEC_ESPNOW_TREE_BCAST/ARCHITECTURE.md"
# ESP-NOW Tree Network Architecture

This document provides a high-level overview of the software architecture for the ESP-NOW Tree Network project.

## Core Principles

1.  **Manual Configuration**: Devices are configured manually using a menu system on the device itself. Configuration is stored in non-volatile memory.
2.  **Hierarchical ID (HID)**: A simple, decimal-based HID system (e.g., `1`, `11`, `12`, `111`) is used to define the network topology.
3.  **Manual Distributed I/O**: The system supports up to **32** unique devices, each manually assigned to control one bit in a shared 32-bit data field.
4.  **Single-File Data Management**: The `DataManager` class is a singleton responsible for all state management, including network status, device configuration, and data aggregation.
5.  **Extensible Menu System**: The `MenuSystem` uses a provider-based architecture (`IDynamicMenuProvider`) to allow for complex, stateful configuration menus without cluttering the core menu logic.

## Key Components

### 1. `DataManager` (Singleton)
- **File**: `DataManager.h`, `DataManager.cpp`
- **Purpose**: Central hub for all device state.
- **Responsibilities**:
    - Manages the device's HID and Bit Index (loading from/saving to NVM).
    - Manages network statistics (`NetworkStats`).
    - Aggregates data from child nodes if acting as a root/branch.
    - Holds the `DistributedIOData` structure, representing the 32 shared I/O bits.
    - Creates and validates all `TreeMessage` packets.

### 2. `MenuSystem` (Singleton)
- **File**: `MenuSystem.h`, `MenuSystem.cpp`
- **Purpose**: Handles all user interaction via the button and OLED display.
- **Architecture**:
    - Uses a static `MenuItem` structure for simple, stateless menus.
    - Implements the `IDynamicMenuProvider` interface for complex, dynamic menus like HID and Bit Index configuration.
    - `HidConfigMenuProvider`: Manages the UI logic for navigating the conceptual HID tree.
    - `BitIndexConfigMenuProvider`: Manages the UI logic for selecting a bit from the 32 available options.

### 3. `TreeNetwork`
- **File**: `TreeNetwork.h`, `TreeNetwork.cpp`
- **Purpose**: High-level logic for tree network operations.
- **Responsibilities**:
    - Initiates data reports to the parent node.
    - Provides a simple interface for sending commands down the tree.
    - Manages auto-reporting logic.

### 4. `espnow_wrapper`
- **File**: `espnow_wrapper.h`, `espnow_wrapper.cpp`
- **Purpose**: Low-level ESP-NOW communication abstraction layer.
- **Responsibilities**:
    - Initializes the ESP-NOW interface and Long-Range (LR) mode.
    - Handles the `esp_now_register_send_cb` and `esp_now_register_recv_cb` callbacks.
    - Forwards incoming data to the `DataManager` for processing.
    - Provides a generic `espnowSendData` function.

### 5. `IoDevice` (Singleton)
- **File**: `IoDevice.h`, `IoDevice.cpp`
- **Purpose**: Manages the device's own GPIO pins for input and output.
- **Responsibilities**:
    - Scans input pins and handles debouncing.
    - Updates output pins based on the shared `DistributedIOData`.
    - Populates the device's `DeviceSpecificData` structure with its current I/O state.

## Communication Protocol

- All communication uses a single `TreeMessage` format with a common `TreeMessageHeader`.
- The `TreeMessageType` enum defines the purpose of the message.
- Current implemented message types:
    - `MSG_DEVICE_DATA_REPORT` - Device reports its current state to parent/root
    - `MSG_DISTRIBUTED_IO_UPDATE` - Root broadcasts shared I/O state to all devices
    - `MSG_COMMAND_SET_OUTPUTS` - Commands to set device outputs
    - `MSG_ACKNOWLEDGEMENT` / `MSG_NACK` - Basic acknowledgment protocol

## Configuration Architecture

### Manual Configuration Process
1. **HID Configuration**: User navigates tree structure via menu to select device position
2. **Bit Index Configuration**: User manually selects from 32 available bits (0-31)
3. **NVM Storage**: Both HID and bit index are stored in non-volatile memory
4. **No Network Dependencies**: Configuration works offline, no network validation required

### Menu System Architecture
- **Static Menus**: Simple menu items with action functions
- **Dynamic Menu Providers**: Complex navigation for HID tree and bit selection
- **Sequential Configuration**: "Configure Device" action chains HID and bit configuration
- **Persistent Storage**: All configuration automatically saved to NVM

## Data Flow Architecture

### Upstream (Device → Root)
1. Device detects input change (button press)
2. Device creates `MSG_DEVICE_DATA_REPORT` with current state
3. Message routed through intermediate nodes to root
4. Root aggregates data from all devices
5. Root computes new shared I/O state based on all active devices

### Downstream (Root → All Devices)
1. Root creates `MSG_DISTRIBUTED_IO_UPDATE` with new shared state
2. Message broadcast to entire network
3. Each device receives update and checks its assigned bit
4. Device updates its output pins based on bit state

This architecture is designed to be simple, reliable, and user-controlled without the need for complex dynamic protocols or network dependencies. 