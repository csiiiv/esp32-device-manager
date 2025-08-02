#line 1 "/mnt/6E9A84429A8408B3/_VSC/WEB_UPLOAD/HELTEC_ESPNOW_TREE_BCAST/TREE_NETWORK_TEST.md"
# Tree Network Testing Guide

This document provides procedures for testing the functionality of the ESP-NOW Tree Network.

## Test 1: Device Configuration

**Objective**: To verify that a new device can be fully configured with a Hierarchical ID (HID) and a Bit Index using the on-device menu.

**Procedure**:

1.  **Flash Firmware**: Flash the latest firmware onto three ESP32 devices (A, B, C).
2.  **Power On**: Power on all three devices. They should all display "Device Not Configured".

3.  **Configure Device A (Root)**:
    -   On Device A, double-click the button to enter the menu.
    -   Select `Configure Device` (it should be the first option).
    -   Long-press to start the configuration sequence.
    -   In the HID menu, navigate to `Set HID: 1` and long-press to select.
    -   The device will transition to the Bit Index menu.
    -   Select `Bit 0` and long-press to confirm.
    -   **Expected Result**: Device A's display will update to show `HID:1(R) B:0`.

4.  **Configure Device B (Child)**:
    -   On Device B, enter the configuration sequence by selecting `Configure Device`.
    -   In the HID menu, select `Child: 1`, then `Set HID: 11`.
    -   In the Bit Index menu, select `Bit 1` and confirm.
    -   **Expected Result**: Device B's display will update to show `HID:11 B:1`.

5.  **Configure Device C (Child)**:
    -   On Device C, enter the configuration sequence.
    -   In the HID menu, select `Child: 1`, then `Child: 12`, then `Set HID: 12`.
    -   In the Bit Index menu, select `Bit 2` and confirm.
    -   **Expected Result**: Device C's display will update to show `HID:12 B:2`.

## Test 2: Distributed I/O Functionality

**Objective**: To verify the closed-loop communication of the distributed I/O system.

**Procedure**:

1.  **Use the configured devices** from Test 1.
2.  **Press the Button on Device B (HID 11)**:
    -   **Expected Result**: The LED on Device B should turn ON. The LEDs on Devices A and C should remain OFF.
    -   **Verification**: This confirms that Device B sent a report, the Root (A) processed it, set bit 1 in the shared data, broadcast the update, and Device B correctly acted on its assigned bit.
3.  **Release the Button on Device B**:
    -   **Expected Result**: The LED on Device B should turn OFF.
4.  **Press the Button on Device C (HID 12)**:
    -   **Expected Result**: The LED on Device C should turn ON. The LEDs on A and B remain OFF.
5.  **Press the Button on Device A (Root)**:
    -   **Expected Result**: The LED on Device A should turn ON.

## Test 3: Network Statistics

**Objective**: To verify that the root node is correctly tracking network activity.

**Procedure**:

1.  Perform several button presses on child devices (B and C) as in Test 2.
2.  **On the Root Node (Device A)**, enter the menu by double-clicking.
3.  Navigate to `Advanced` -> `Tree Network` -> `Show Tree Stats`.
4.  **Expected Result**: The status display will show the number of messages received (e.g., `RX: 4`).
5.  Navigate to `Advanced` -> `Info` -> `Show Last Sender`.
6.  **Expected Result**: The status display will show the MAC address of the last device that sent a message.

## Test 4: Configuration Persistence

**Objective**: To verify that device configurations are correctly saved to and loaded from NVM.

**Procedure**:

1.  Configure a device with a specific HID and Bit Index.
2.  **Power Cycle the Device**: Disconnect and reconnect power.
3.  **Expected Result**: After booting, the device should immediately display its configured state (e.g., `HID:11 B:1`) without needing any user interaction.
4.  **Clear Configuration**:
    -   Enter the menu and select `Advanced` -> `Device Config` -> `Clear All Config`.
    -   Power cycle the device again.
    -   **Expected Result**: The device should boot to the "Device Not Configured" state.

## 🧪 **Test Setup**

### **Required Hardware**
- **3+ ESP32 devices** (for meaningful tree topology)
- **OLED displays** (optional but recommended for visual feedback)
- **Serial monitors** for each device

### **Test Network Topology**
```
        Root (HID: 1)
             |
    ┌────────┼────────┐
    │        │        │
  Node11   Node12   Node121
    |        |        |
  Child   Child    Child
```

## 🔧 **Test Scenario 1: Basic Network Setup**

### **Step 1: Configure Root Node**
1. Upload firmware to Device A
2. Open Serial Monitor (115200 baud)
3. Double-click button to enter menu
4. Select `Configure Device`
5. Navigate HID tree to set HID `1` (Root)
6. Select bit index `0`
7. Verify status shows "HID:1(R) B:0"

**Expected Results:**
- Root node initialized with HID 1
- Status shows "Root Node Ready"
- Configuration persists across reboots

### **Step 2: Configure Child Nodes**
1. **Device B (HID 11):**
   - Upload firmware
   - `Configure Device` → Navigate to HID 11
   - Select bit index 1
   - Verify status shows "HID:11 B:1"
   
2. **Device C (HID 12):**
   - Upload firmware  
   - `Configure Device` → Navigate to HID 12
   - Select bit index 2
   - Verify status shows "HID:12 B:2"

3. **Device D (HID 121):**
   - Upload firmware
   - `Configure Device` → Navigate to HID 121
   - Select bit index 3
   - Verify status shows "HID:121 B:3"

**Expected Results:**
- All devices show correct HID configuration
- Parent relationships auto-calculated (11→1, 12→1, 121→12)

## 📡 **Test Scenario 2: Data Reporting**

### **Step 3: Manual Data Reports**
1. **On Device D (HID 121):**
   - Enter menu: `Advanced` -> `Tree Network` -> `Send Data Report`
   - Verify status shows "Data report sent"

2. **On Device B (HID 12):**
   - Monitor Serial output for forwarding message
   - Should see: "Forwarding upstream to parent 1"

3. **On Root (HID 1):**
   - Check: `Advanced` -> `Tree Network` -> `Show Tree Stats`
   - Should show incremented data reports received

**Expected Results:**
- Data flows: 121 → 12 → 1
- Intermediate forwarding logged
- Root receives and stores data

### **Step 4: Automatic Data Reporting**
1. **On Device C (HID 12):**
   - `Advanced` -> `Tree Network` -> `Toggle Auto Reporting`
   - Verify status shows "Auto Report ON"
   - Wait 5+ seconds for automatic reports

2. **Monitor Root Node:**
   - Check periodic data reception
   - Verify tree stats increment automatically

**Expected Results:**
- Automatic reports every 5 seconds
- Data includes button state, analog values, counters

## 🎯 **Test Scenario 3: Command Distribution**

### **Step 5: Downstream Commands**
1. **On Root (HID 1):**
   - `Advanced` -> `Tree Network` -> `Send Test Command`
   - Target will be first child (11 or 12)

2. **On Target Device:**
   - Monitor Serial for command reception
   - Verify status shows command execution

**Expected Results:**
- Commands routed correctly through tree
- Target devices execute commands

## 🔍 **Test Scenario 4: Error Conditions**

### **Step 6: Invalid Configurations**
1. **Unconfigured Device:**
   - Reset a device's HID: `Advanced` -> `Device Config` -> `Clear HID`
   - Try to send data report: `Advanced` -> `Tree Network` -> `Send Data Report`
   - Verify error: "HID not configured"

2. **Root Node Restrictions:**
   - On root node, try `Advanced` -> `Tree Network` -> `Send Data Report`
   - Verify error: "Root doesn't send reports"

**Expected Results:**
- System handles errors gracefully
- Appropriate error messages displayed
- No crashes or undefined behavior

## 📊 **Test Scenario 5: Performance Validation**

### **Step 7: Message Statistics**
1. **Enable Debug Logging:**
   - On all devices: `Advanced` -> `Info` -> `Toggle Debug`
   - Monitor Serial outputs for detailed logs

2. **Traffic Generation:**
   - Enable auto reporting on multiple devices
   - Monitor statistics on all nodes

3. **Verify Metrics:**
   - Data reports sent/received
   - Commands sent/received  
   - Messages forwarded
   - RSSI values

**Expected Results:**
- Statistics accurately reflect traffic
- RSSI values within expected range
- No excessive packet loss

## 🌐 **Test Scenario 6: Long Range Mode**

### **Step 8: Range Testing**
1. **Enable Long Range Mode:**
   - On all devices: `Advanced` -> `Info` -> `Toggle LR Mode`
   - Verify: `Advanced` -> `Info` -> `Show LR Status` shows "LR:ON"

2. **Distance Testing:**
   - Gradually increase distance between nodes
   - Monitor RSSI degradation
   - Test maximum reliable range

3. **Performance Comparison:**
   - Compare message success rates: Standard vs LR mode
   - Document range improvements

**Expected Results:**
- Extended range in LR mode
- Maintained connectivity at greater distances
- PHY rate shows "Long Range (LR)"

## 🔧 **Test Scenario 7: Network Persistence**

### **Step 9: Power Cycle Testing**
1. **Configuration Persistence:**
   - Power cycle all devices
   - Verify HIDs and bit indices restored from NVM
   - Verify network configuration preserved

2. **Network Recovery:**
   - Test network functionality after reboot
   - Verify data flows resume automatically

**Expected Results:**
- All configuration survives power cycles
- Network automatically resumes operation
- No manual reconfiguration required

## 📋 **Test Validation Checklist**

### **Basic Functionality**
- [ ] Root node configured (HID 1)
- [ ] Child nodes configured with valid HIDs
- [ ] Parent relationships calculated correctly
- [ ] Bit indices manually assigned

### **Data Flow**
- [ ] Manual data reports work
- [ ] Automatic data reporting functions
- [ ] Multi-hop data forwarding
- [ ] Data received and stored at root

### **Command Distribution**
- [ ] Commands sent from root
- [ ] Commands executed at target devices

### **Error Handling**
- [ ] Invalid HID detection
- [ ] Unconfigured device handling
- [ ] Root node restrictions enforced

### **Performance**
- [ ] Message statistics accurate
- [ ] RSSI monitoring functional
- [ ] No excessive packet loss
- [ ] Reasonable latency (<100ms/hop)

### **Long Range**
- [ ] LR mode toggle works
- [ ] Extended range achieved
- [ ] PHY rate correctly reported

### **Persistence**
- [ ] HID configuration survives reboot
- [ ] Bit index configuration survives reboot
- [ ] Automatic network recovery

## 🚨 **Troubleshooting Common Issues**

### **No Data Reception**
- Verify all devices have same firmware
- Check HID configuration on all nodes
- Ensure devices are in range
- Verify ESP-NOW initialization

### **Forwarding Not Working**
- Check parent-child relationships
- Verify intermediate nodes configured
- Monitor Serial logs for routing decisions

### **Configuration Lost**
- Check NVM preferences functionality
- Verify preferences.begin() calls
- Test with fresh ESP32 flash

### **Poor Performance**
- Monitor RSSI values
- Check for WiFi interference
- Verify Long Range mode consistency
- Test with fewer devices initially

## 📈 **Performance Benchmarks**

### **Expected Metrics**
- **Message Latency**: <50ms single hop, <150ms three hops
- **Packet Success Rate**: >95% in good conditions
- **Range Standard Mode**: 50-200m typical
- **Range LR Mode**: 200-800m typical
- **Memory Usage**: <10KB per device on root
- **Power Consumption**: <100mA typical (excluding OLED)

### **Stress Testing**
- Sustained data reporting from all devices
- Rapid command distribution testing
- Network resilience with node failures
- Maximum topology depth validation

This comprehensive test suite validates all aspects of the tree network implementation and provides a solid foundation for production deployment. 