e# Distributed I/O Demo Walkthrough

This document explains the functionality of the 32-bit distributed I/O system.

## Objective

To demonstrate how multiple devices in the tree network can collaboratively control a shared 32-bit data state. Each device can be manually configured to control one bit.

**Key Idea**: Pressing a button on any device sends a signal to the root, which then updates a shared state that is broadcast back to all devices. Each device then acts on this shared state based on its configured bit index.

## How It Works

1.  **Input Trigger**: Each device uses its built-in button (GPIO 0) as its primary input.
2.  **Report to Root**: When you press the button on a device, its `IoDevice` module detects the state change and triggers a data report (`MSG_DEVICE_DATA_REPORT`) that is sent up the tree to the root node.
3.  **Root Aggregation**: The root's `DataManager` receives these reports and maintains a table of all devices in the network.
4.  **Compute Shared State**: The root node's `DataManager::computeSharedDataFromInputs()` function iterates through all known devices. If a device is reporting an active input, the root sets that device's assigned bit in the 32-bit `DistributedIOData` structure.
5.  **Broadcast Shared State**: If the computed shared data has changed, the root broadcasts it to the entire network using a `MSG_DISTRIBUTED_IO_UPDATE` message.
6.  **Act on Shared State**: Every device in the network receives this update. The `IoDevice` on each device checks the shared data. If the bit assigned to it is `1`, it turns on its output (GPIO 23). If the bit is `0`, it turns the output off.

## Configuration Requirements

Before running the demo, each device must be configured with:

1. **Hierarchical ID (HID)**: Defines the device's position in the tree network
2. **Bit Index**: Assigns the device to one of the 32 available bits (0-31)

Both configurations are done manually using the on-device menu system.

## Demo Scenario

**Setup**:
-   **Device A**: Configured with `HID: 1` (Root) and `Bit Index: 0`.
-   **Device B**: Configured with `HID: 11` and `Bit Index: 1`.
-   **Device C**: Configured with `HID: 12` and `Bit Index: 2`.
-   Each device has an LED connected to GPIO 23.

**Configuration Steps**:

1. **Configure Device A (Root)**:
   - Double-click to enter menu
   - Select `Configure Device`
   - Navigate HID tree menu to set HID `1`
   - Select bit index `0` from bit selection menu

2. **Configure Device B**:
   - Double-click to enter menu
   - Select `Configure Device`
   - Navigate HID tree menu to set HID `11`
   - Select bit index `1` from bit selection menu

3. **Configure Device C**:
   - Double-click to enter menu
   - Select `Configure Device`
   - Navigate HID tree menu to set HID `12`
   - Select bit index `2` from bit selection menu

**Execution Flow**:

1.  **Initial State**: All buttons are released. The shared data is `0x00000000`. All LEDs are OFF.

2.  **Press Button on Device B**:
    -   Device B's input becomes active.
    -   Device B sends a data report to the root (Device A).
    -   Root (A) receives the report. It computes the new shared state, setting bit `1` (Device B's bit). The new state is `0x00000002`.
    -   Root (A) broadcasts the update: `[Shared Data: 0x00000002]`.
    -   All devices receive the update:
        -   Device A checks bit `0` (it's 0) → Its LED stays OFF.
        -   Device B checks bit `1` (it's 1) → Its LED turns ON.
        -   Device C checks bit `2` (it's 0) → Its LED stays OFF.

3.  **Press Button on Device C (while B is still pressed)**:
    -   Device C sends a report to the root.
    -   Root computes the new state. It sees B's input is still active (from its last report) and C's is now active. It sets bits `1` and `2`. The new state is `0x00000006`.
    -   Root broadcasts the update: `[Shared Data: 0x00000006]`.
    -   All devices receive the update:
        -   Device B checks bit `1` (it's 1) → Its LED stays ON.
        -   Device C checks bit `2` (it's 1) → Its LED turns ON.

4.  **Release Button on Device B**:
    -   Device B's input goes inactive. It sends a new report.
    -   Root computes the new state. It clears bit `1` but keeps bit `2` set. The new state is `0x00000004`.
    -   Root broadcasts the update.
    -   Device B's LED turns OFF. Device C's LED stays ON.

## Current Implementation Notes

- Configuration is **manual** through menu system - there is no automatic bit assignment
- Each device must be individually configured with both HID and bit index
- The system supports up to 32 devices (bits 0-31)
- All configuration is stored in non-volatile memory and persists across reboots

This demonstrates a complete, closed-loop, distributed control system built on the tree network. 