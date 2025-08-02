# Device Configuration Guide

## Overview
The ESP-NOW Tree Network uses **manual device configuration** through an on-device menu system. Each device must be individually configured with a Hierarchical ID (HID) and a Bit Index to participate in the distributed I/O system.

## Key Features

### ✅ **Manual Configuration**
- On-device menu system for configuration
- No network dependencies during configuration
- Immediate feedback during setup process
- Configuration stored in non-volatile memory

### ✅ **Tree-Based HID Navigation** 
- Intuitive tree navigation for HID selection
- Mathematical HID relationships (Parent = HID / 10)
- Visual confirmation of selected HID
- Support for up to 64 devices in tree

### ✅ **Simple Bit Assignment**
- Manual selection from 32 available bits (0-31)
- Page-based navigation for bit selection
- Visual indicators for already assigned bits
- No conflict resolution - user responsibility

## Configuration Process

### Configuration Steps:
1. **Device powers on** showing "Device Not Configured"
2. **User enters menu** by double-clicking button
3. **User selects "Configure Device"** to start sequential configuration
4. **HID Configuration**: Navigate tree structure to select device position
5. **Bit Index Configuration**: Select from available bits 0-31
6. **Configuration complete**: Device shows HID and bit index on status screen

### Network Capacity:
- **Maximum devices**: 64 total in tree structure
- **Distributed I/O bits**: 32 bits (0-31)
- **Assignment method**: Manual selection by user
- **Conflict prevention**: User responsibility to avoid duplicates

## Configuration Examples

### Example 1: Factory Automation Network
```
Network Topology:
Root (HID=1) → Production Lines (HIDs 11,12) → Stations (HIDs 111,112,121,122)

Manual Configuration Sequence:
1. Configure Root: HID=1, manually select bit 0
2. Configure Line 1 Controller: HID=11, manually select bit 1
3. Configure Line 2 Controller: HID=12, manually select bit 2
4. Configure Station 1A: HID=111, manually select bit 3
5. Configure Station 1B: HID=112, manually select bit 4
6. Configure Station 2A: HID=121, manually select bit 5
7. Configure Station 2B: HID=122, manually select bit 6

Result:
- Bit 0: Root Controller (HID=1)
- Bit 1: Line 1 Controller (HID=11)
- Bit 2: Line 2 Controller (HID=12)
- Bit 3: Station 1A (HID=111)
- Bit 4: Station 1B (HID=112)
- Bit 5: Station 2A (HID=121)
- Bit 6: Station 2B (HID=122)
- Bits 7-31: Available for future devices
```

### Example 2: Building Management System
```
Network Topology:
Root (HID=1) → Floors (HIDs 11,12,13) → Zones (HIDs 111,112,121,122,131,132)

Manual Configuration Result:
- Bit 0: Building Controller (HID=1)
- Bit 1: Floor 1 Main (HID=11)
- Bit 2: Floor 2 Main (HID=12)
- Bit 3: Floor 3 Main (HID=13)
- Bit 4: Floor 1 Zone A (HID=111)
- Bit 5: Floor 1 Zone B (HID=112)
- Bit 6: Floor 2 Zone A (HID=121)
- Bit 7: Floor 2 Zone B (HID=122)
- Bit 8: Floor 3 Zone A (HID=131)
- Bit 9: Floor 3 Zone B (HID=132)
- Bits 10-31: Available for expansion
```

### Example 3: Step-by-Step Configuration

#### Device Configuration Process:
```
Target: Configure device with HID=121, Bit Index=3

Step 1: Enter Menu
- Device shows "Device Not Configured"
- Double-click button to enter menu

Step 2: Start Configuration
- Menu shows "Configure Device" as first option
- Long-press button to select

Step 3: Configure HID
- HID menu appears showing tree navigation
- Navigate: Child 1 → Child 12 → Child 121
- Select "Set HID: 121"
- HID configuration saved

Step 4: Configure Bit Index
- Bit index menu appears automatically
- Navigate to "Bit 3"
- Long-press to select
- Bit index configuration saved

Step 5: Configuration Complete
- Device returns to status screen
- Display shows "HID:121 B:3"
- Device ready for network operation
```

## User Interface

### Menu Structure:
```
Main Menu (after double-click)
├── Configure Device → Sequential HID + Bit configuration
├── Show Device Status → Display current configuration
├── Advanced → Advanced menu options
│   ├── Device Config → Individual configuration options
│   │   ├── Configure HID → HID-only configuration
│   │   ├── Configure Bit Index → Bit-only configuration
│   │   ├── Clear HID → Reset HID configuration
│   │   ├── Clear Bit Index → Reset bit configuration
│   │   └── Clear All Config → Reset all configuration
│   ├── Tree Network → Network operations and statistics
│   ├── I/O Device → I/O control and monitoring
│   └── Info → System information and statistics
└── Exit Menu → Return to status display
```

### HID Configuration Menu:
```
HID Tree Navigation:
├── Set HID: [current] → Confirm current HID
├── Go Up → Move to parent level (if not at root)
├── Child: X1 → Navigate to child X1
├── Child: X2 → Navigate to child X2
├── Child: X3 → Navigate to child X3
├── Child: X4 → Navigate to child X4
└── Back → Return to previous menu

Navigation follows mathematical rules:
- Current HID 1: Children are 11, 12, 13, 14
- Current HID 12: Children are 121, 122, 123, 124
- Parent HID = Current HID / 10 (integer division)
```

### Bit Index Configuration Menu:
```
Bit Selection (Pages of 8 bits):
Page 0 (Bits 0-7):
├── Bit 0 → Select bit 0
├── Bit 1 → Select bit 1
├── ...
├── Bit 7 → Select bit 7
├── Next Page → Go to page 1
└── Back → Return to previous menu

Note: Bits already assigned to current device show with "*"
```

## Configuration Management

### Status Display:
```
Device Status Screen:
Line 1: "HID:121 B:3" (HID and Bit Index)
Line 2: "Status: Ready" (Current system status)
Line 3: "[Network activity info]" (Dynamic status)

Unconfigured Device:
Line 1: "Device Not Configured"
Line 2: "Double-click to configure"
Line 3: "[System status]"
```

### Configuration Persistence:
- **HID Configuration**: Stored in NVM preferences
- **Bit Index Configuration**: Stored in NVM preferences
- **Automatic Restore**: Configuration restored on power-up
- **Manual Reset**: Configuration can be cleared via menu

### Configuration Validation:
- **HID Validation**: Must be valid hierarchical ID (1-9999)
- **Bit Index Validation**: Must be 0-31
- **No Network Validation**: Configuration doesn't require network connectivity
- **User Responsibility**: Avoiding duplicate assignments

## Implementation Benefits

### For Developers:
- **Simple implementation**: No complex protocols or validation
- **No network dependencies**: Configuration works offline
- **Deterministic behavior**: No dynamic assignment conflicts
- **Easy debugging**: All configuration is explicit and visible

### For Users:
- **Intuitive interface**: Tree-based menu matches network structure
- **Immediate feedback**: Instant configuration confirmation
- **Offline configuration**: No need for network connectivity
- **Visual confirmation**: Current configuration always displayed

### For System Integration:
- **Predictable assignments**: All bit assignments are explicit
- **Simple management**: No dynamic state to track
- **Easy troubleshooting**: Configuration is always visible
- **Maintenance friendly**: Clear manual process

## Best Practices

### Configuration Planning:
1. **Plan HID structure** before deploying devices
2. **Document bit assignments** for easy management
3. **Use sequential bit assignment** for easier tracking
4. **Reserve bits** for future expansion
5. **Test configuration** before final deployment

### Troubleshooting:
- **Check HID relationships**: Ensure parent-child relationships are correct
- **Verify bit uniqueness**: Ensure no duplicate bit assignments
- **Test network connectivity**: Verify devices can communicate
- **Monitor status displays**: Check for configuration issues

### Network Management:
- **Keep configuration records**: Document all HID and bit assignments
- **Use consistent naming**: Follow logical naming conventions
- **Plan for expansion**: Reserve HIDs and bits for growth
- **Regular testing**: Verify network functionality periodically

This manual configuration system provides **simple, reliable, and user-controlled** device management for your 32-device distributed I/O network!