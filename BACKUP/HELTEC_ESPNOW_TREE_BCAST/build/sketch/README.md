#line 1 "/mnt/6E9A84429A8408B3/_VSC/WEB_UPLOAD/HELTEC_ESPNOW_TREE_BCAST/README.md"
# ESP-NOW Tree Network & Distributed I/O System

This project implements a self-configuring, multi-hop mesh network on ESP32 devices using the ESP-NOW protocol. It features a dynamic tree topology, manual device configuration, and a simple distributed I/O system.

## 🚀 Core Features

-   **Dynamic Tree Network**: Devices form a hierarchical tree network without any hardcoded configurations.
-   **Manual Device Configuration**: Use the on-device menu to configure a device's position (HID) in the tree and its function (Bit Index).
-   **Distributed I/O System (32-Bit)**: The network collaborates to control a shared 32-bit data field. Each device can be manually assigned one bit to read from its input or write to its output.
-   **Multi-Hop Communication**: Messages are automatically routed up to the root or down to a specific device.
-   **ESP-NOW Long-Range Mode**: Utilizes ESP-NOW's LR mode for extended communication distance.
-   **On-Device UI**: An OLED display and a single push-button provide a complete user interface for configuration and monitoring.
-   **Persistent Configuration**: Device settings (HID and Bit Index) are saved to NVM and restored on boot.

## 🏗️ System Architecture

The system is built on several key components that work together. For a detailed explanation, see [`ARCHITECTURE.md`](./ARCHITECTURE.md).

1.  **DataManager**: A singleton that manages all device state, including HID, Bit Index, network stats, and the 32-bit shared I/O data.
2.  **MenuSystem**: A singleton that drives the OLED display and button UI. It uses a dynamic provider model to create complex configuration menus.
3.  **TreeNetwork**: High-level logic for tree operations like sending data reports.
4.  **IoDevice**: Manages the local device's GPIO pins, linking them to the distributed I/O system.
5.  **espnow\_wrapper**: A low-level wrapper for all ESP-NOW communication.

## 🎛️ Device Configuration

The entire system is configured using the on-device menu. Each device must be manually configured with both HID and Bit Index.

### Step 1: Configure the Hierarchical ID (HID)

The HID determines a device's position in the tree. The root is `1`, its children are `11`, `12`, etc.

1.  **Enter the Menu**: Double-click the button.
2.  **Start Configuration**:
    -   Select `Configure Device` (first option in main menu).
    -   Long-press to start the sequential configuration process.
3.  **Set the HID**:
    -   The HID configuration menu will appear.
    -   Use short presses to navigate and a long press to select.
    -   Navigate down the conceptual tree to build your HID (e.g., `Child: 1` -> `Child: 12` -> `Child: 121`).
    -   Once you've reached the desired HID, select `Set HID: ...`.

### Step 2: Configure the Bit Index

The Bit Index assigns the device to one of the 32 bits in the shared I/O data space.

1.  **Automatic Transition**: After setting the HID, the device automatically proceeds to the Bit Index configuration menu.
2.  **Select a Bit**:
    -   The menu displays the 32 bits in pages of 8.
    -   Use `Next Page` and `Prev Page` to navigate.
    -   Select any available bit for your device (e.g., `Bit 0`). A `*` indicates a bit is already assigned to the current device.
3.  **Confirmation**: Once a bit is selected, the device is fully configured and returns to the main status screen.

### Individual Configuration Options

For advanced users, individual configuration options are available:

-   Navigate to `Advanced` -> `Device Config` -> `Configure HID` to set HID only.
-   Navigate to `Advanced` -> `Device Config` -> `Configure Bit Index` to set bit index only.
-   Navigate to `Advanced` -> `Device Config` -> `Clear All Config` to wipe the device's HID and Bit Index from NVM.

## 🔧 Hardware & Setup

-   **ESP32 Development Board**
-   **128x64 SSD1306 OLED Display** (I2C: SDA=17, SCL=18)
-   **Push Button** (on GPIO 0 with internal pull-up)

## 💡 How the Distributed I/O Works

1.  **Input Reading**: Each device reads its own physical input pin (GPIO 0, the same as the button).
2.  **Data Reporting**: If the input is active (button is pressed), the device sends a `MSG_DEVICE_DATA_REPORT` up the tree to the root.
3.  **Aggregation at Root**: The root node receives reports from all devices in the network.
4.  **Shared State Computation**: The root computes the final 32-bit shared data state. For each device that has its input active, the root sets the corresponding bit in the `DistributedIOData` structure.
5.  **Broadcast**: The root broadcasts the updated 32-bit `DistributedIOData` to the entire network in a `MSG_DISTRIBUTED_IO_UPDATE` message.
6.  **Output Writing**: Every device in the network receives the shared data. Each device is responsible for reading its assigned bit from the shared data and setting its physical output pin (GPIO 23) accordingly.

**Example**:
-   Device with Bit Index `5` presses its button.
-   It reports this to the root.
-   The root sets bit `5` of the shared data to `1` and broadcasts it.
-   Device with Bit Index `12` sees the broadcast, reads bit `12` (which is `0`), and keeps its output OFF.
-   Device with Bit Index `5` sees the broadcast, reads bit `5` (which is `1`), and turns its output ON.
-   **Result**: Pressing the button on any device will turn on the LED on that *same* device, but the signal travels all the way to the root and back. This demonstrates the full communication loop.

## 🛠️ Code Configuration

Key settings can be adjusted in the header files:

-   `IoDevice.h`: `ENABLE_IO_DEVICE_PINS`, `ENABLE_DISTRIBUTED_IO`.
-   `espnow_wrapper.h`: `ENABLE_LONG_RANGE_MODE`.
-   `oled.h`: `ENABLE_OLED`.

## 🌐 **Tree Network Overview**

### **Network Architecture**
- **Multi-hop wireless network** utilizing ESP-NOW as the Layer 2 protocol
- **Hierarchical ID (HID) system** for logical tree topology
- **Broadcast-based routing** with application-layer filtering  
- **Maximum 64 devices** with up to 8 children per node
- **Upstream data aggregation** at Root Node
- **Downstream command distribution** from Root to specific devices

### **Hierarchical ID System**
- **Root Node ID**: `1`
- **Child ID Generation**: `Child_HID = (Parent_HID * 10) + Child_Sequence_Number`
- **Parent Derivation**: `Parent_HID = My_HID / 10` (integer division)
- **Examples**: Root=1, Children=11,12,13..., Grandchildren=111,112,121,122...

### **Communication Model**
- **All messages use ESP-NOW broadcasts** for simplicity
- **Application-layer filtering** based on HIDs determines packet relevance
- **Structured message protocol** with headers, CRC, and sequence numbers
- **Automatic forwarding** by intermediate nodes
- **Acknowledgment support** for reliable delivery

## 🏗️ **System Components**

### **1. DataManager - Core System**
- **Centralized data management** with singleton pattern
- **HID configuration and validation**
- **Message routing logic** (upstream/downstream)
- **Device data aggregation** (root node)
- **NVM storage** for configuration persistence

### **2. Tree Message Protocol**
```
Message Frame Structure (12-byte overhead):
[SOH][FrameLen][DestHID][SrcHID][BroadcasterHID][MsgType][SeqNum][Payload][CRC][EOT]
```

**Key Fields:**
- **SrcHID**: Original message source (preserved during forwarding)
- **BroadcasterHID**: Immediate broadcaster (updated at each hop for validation)
- **DestHID**: Final destination (updated for upstream, preserved for downstream)

**Message Types:**
- `MSG_DEVICE_DATA_REPORT` (0x01) - Upstream data reports
- `MSG_ACKNOWLEDGEMENT` (0x02) - ACK responses  
- `MSG_NACK` (0x03) - NACK with reason codes
- `MSG_COMMAND_SET_OUTPUTS` (0x10) - Set output states
- `MSG_DISTRIBUTED_IO_UPDATE` (0x22) - Broadcast shared I/O state

### **3. Device Data Structure (12 bytes)**
```cpp
typedef struct {
    uint8_t  input_states;     // 8 input pins bitmap
    uint8_t  output_states;    // 8 output pins bitmap  
    uint16_t memory_states;    // 16 memory states bitmap
    uint16_t analog_values[2]; // 2x 16-bit analog readings
    uint16_t integer_values[2];// 2x 16-bit general purpose values
    uint8_t  bit_index;        // Device's assigned bit index
    uint8_t  reserved;         // Reserved for future use
} DeviceSpecificData;
```

## 🚀 **Quick Start Guide**

### **Step 1: Configure Root Node**
1. Upload firmware to ESP32 designated as root
2. Double-click button to enter menu
3. Select `Configure Device`
4. Navigate HID tree to select HID `1` (Root)
5. Select bit index `0` (or any available bit)
6. Device becomes root and is ready to manage network

### **Step 2: Configure Child Nodes**  
1. Upload firmware to child ESP32 devices
2. Double-click button to enter menu
3. Select `Configure Device`
4. Navigate HID tree to select appropriate HID (11, 12, 13... for root children)
5. Select an available bit index
6. Devices will auto-calculate parent relationships

### **Step 3: Test Communication**
1. Child nodes: Press button to trigger I/O reports
2. Root node: Check `Advanced` -> `Tree Network` -> `Show Tree Stats`
3. Enable auto-reporting: `Advanced` -> `Tree Network` -> `Toggle Auto Reporting`

## 🌐 **Routing Logic**

### **Upstream Data Flow (Device → Root)**
Devices send data reports to the root through multi-hop routing:

1. **Originating Device** (e.g., HID 1211):
   - Calculates parent HID: `1211 / 10 = 121`
   - Creates message: `[DestHID=121, SrcHID=1211, BroadcasterHID=1211, DATA_REPORT, DeviceData]`
   - Broadcasts message

2. **Intermediate Node** (e.g., HID 121):
   - Receives broadcast, filters: `DestHID(121) == My_HID(121)` ✓
   - **Security Check**: `BroadcasterHID(1211) / 10 == My_HID(121)` ✓ (valid child)
   - Processes data report locally
   - **Preserves Original Source**: Creates forwarding message: `[DestHID=12, SrcHID=1211, BroadcasterHID=121, DATA_REPORT, OriginalData]`
   - Broadcasts to next hop

3. **Root Node** (HID 1):
   - Receives message with `DestHID=1, SrcHID=1211, BroadcasterHID=12`
   - **Knows original source**: Data came from device 1211
   - **Validates immediate sender**: Broadcaster 12 is valid direct child

**Key Point**: Original `SrcHID` is preserved so root knows the true data source, while `BroadcasterHID` enables hop-by-hop validation.

### **Downstream Command Flow (Root → Device)**
Root sends commands to specific devices through cascaded forwarding:

1. **Root Node** (HID 1):
   - Creates command: `[DestHID=1211, SrcHID=1, BroadcasterHID=1, COMMAND, Payload]`
   - Broadcasts message

2. **Intermediate Node** (e.g., HID 12):
   - **Filter 1**: `BroadcasterHID(1) == My_Parent(1)` ✓ (from my parent)
   - **Filter 2**: `DestHID(1211) != My_HID(12)` ✓ (not for me)
   - **Filter 3**: `"1211".startsWith("12")` ✓ (target is my descendant)
   - **Updates Broadcaster**: `[DestHID=1211, SrcHID=1, BroadcasterHID=12, COMMAND, Payload]`
   - Broadcasts to descendants

3. **Final Intermediate Node** (HID 121):
   - **Filter 1**: `BroadcasterHID(12) == My_Parent(12)` ✓ (from my parent)
   - **Filter 2**: `DestHID(1211) != My_HID(121)` ✓ (not for me)
   - **Filter 3**: `"1211".startsWith("121")` ✓ (target is my descendant)
   - **Updates Broadcaster**: `[DestHID=1211, SrcHID=1, BroadcasterHID=121, COMMAND, Payload]`
   - Broadcasts to descendants

4. **Target Device** (HID 1211):
   - **Filter 1**: `BroadcasterHID(121) == My_Parent(121)` ✓ (from my parent)
   - **Filter 2**: `DestHID(1211) == My_HID(1211)` ✓ (for me!)
   - **Knows original source**: Command originated from root (SrcHID=1)
   - Processes command

**Critical Feature**: Dual-ID system enables both source attribution and hop-by-hop security validation.

## 🎛️ **Menu System**

### **Main Menu**
- **Configure Device** - Sequential HID + Bit Index configuration
- **Show Device Status** - Display current configuration
- **Advanced** - Access to advanced menus
- **Exit Menu** - Return to status display

### **Advanced Menu Options**
- **Device Config** - Individual configuration options
  - **Configure HID** - Set HID only
  - **Configure Bit Index** - Set bit index only
  - **Clear HID** - Reset HID configuration
  - **Clear Bit Index** - Reset bit index configuration
  - **Clear All Config** - Reset all configuration
- **Tree Network** - Network operations and statistics
- **Show HID Info** - Display current HID and parent
- **Send Data Report** - Manual data report to parent
- **Show Tree Stats** - Display data reports, commands, forwards
- **Toggle Auto Reporting** - Enable/disable automatic data reports
- **I/O Device** - I/O control and monitoring
- **Info** - System information and statistics

### **Enhanced Status Display**
- **Title**: Shows HID and bit index (e.g., "HID:12 B:3")
- **Status Line**: Real-time system status
- **Network Activity**: Current network operations
- **Serial Fallback**: Full status when OLED disabled

## ⚙️ **Configuration Options**

### **🖥️ OLED Display Control**
```cpp
// In oled.h - comment out to disable OLED
#define ENABLE_OLED 1
```

### **📡 ESP-NOW Long Range Mode**
```cpp
// In espnow_wrapper.h - enable for extended range
#define ENABLE_LONG_RANGE_MODE 1
```

### **🔌 I/O Device Control**
```cpp
// In IoDevice.h - disable for debugging
#define ENABLE_IO_DEVICE_PINS 1
#define ENABLE_DISTRIBUTED_IO 1
```

## 📝 **Configuration Management**

### **Manual Configuration Required**
- Each device must be individually configured
- No automatic bit assignment or network discovery
- User responsible for avoiding duplicate bit assignments
- Configuration stored in NVM and persists across reboots

### **Best Practices**
- Plan HID structure before deployment
- Document bit assignments for management
- Use sequential bit assignment for easier tracking
- Test configuration before final deployment

## 🔧 **Development & Debugging**

### **Serial Output**
- **115200 baud** for all serial communications
- **Detailed logging** available through debug settings
- **Network activity** logged with HID tracking
- **Error reporting** for configuration and communication issues

### **Status Monitoring**
- **Real-time display** of system status
- **Network statistics** tracking
- **Configuration validation** feedback
- **RSSI monitoring** for signal strength

This system provides a **robust, manually-configured, and user-controlled** distributed I/O network for up to 32 devices over a 64-device tree topology! 