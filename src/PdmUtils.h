// Copyright (c) 2022 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <unordered_set>
#include <vector>
#include <string>

namespace PdmUtils {
static const char *STORAGE_DEVICE_CONNECTED = "Storage device is connected.";
static const char *HID_DEVICE_CONNECTED = "HID device is connected.";
static const char *VIDEO_DEVICE_CONNECTED = "Camera device is connected.";
static const char *SOUND_DEVICE_CONNECTED = "Sound device is connected.";
static const char *GAMEPAD_DEVICE_CONNECTED = "XPAD device is connected.";
static const char *MTP_DEVICE_CONNECTED = "MTP device is connected.";
static const char *PTP_DEVICE_CONNECTED = "PTP device is connected.";
static const char *BLUETOOTH_DEVICE_CONNECTED = "Bluetooth device is connected.";
static const char *CDC_DEVICE_CONNECTED = "USB device is connected.";
static const char *UNKNOWN_DEVICE_CONNECTED = "Unknown device is connected.";
static const char *STORAGE_DEVICE_DISCONNECTED =
        "Storage device is disconnected.";
static const char *HID_DEVICE_DISCONNECTED = "HID device is disconnected.";
static const char *VIDEO_DEVICE_DISCONNECTED = "Camera device is disconnected.";
static const char *SOUND_DEVICE_DISCONNECTED = "Sound device is disconnected.";
static const char *GAMEPAD_DEVICE_DISCONNECTED = "XPAD device is disconnected.";
static const char *MTP_DEVICE_DISCONNECTED = "MTP device is disconnected.";
static const char *PTP_DEVICE_DISCONNECTED = "PTP device is disconnected.";
static const char *BLUETOOTH_DEVICE_DISCONNECTED =
        "Bluetooth device is disconnected.";
static const char *CDC_DEVICE_DISCONNECTED = "USB device is disconnected.";
static const char *UNKNOWN_DEVICE_DISCONNECTED =
        "Unknown device is disconnected.";
static const char *MAX_USB_STORAGE_DEVICES_REACHED =
        "Exceeded maximum number of allowable USB storage. You can connect up to 6 USB storages to your device";
static const char *REMOVE_USB_DEVICE_BEFORE_MOUNT =
        "After removing, please reconnect the usb device.";
static const char *USB_STORAGE_DEV_UNSUPPORTED_FS =
        "This USB storage has an unsupported system and cannot be read.";
static const char *USB_STORAGE_FSCK_TIME_OUT =
        "Some files may not be recognizable. Do you want to open device name now?";
static const char *STORAGE_DEV_FORMAT_STARTED = "Formatting {DRIVEINFO}...";
static const char *STORAGE_DEV_FORMAT_SUCCESS =
        "Formatting {DRIVEINFO} has been successfully completed.";
static const char *STORAGE_DEV_FORMAT_FAIL =
        "Formatting {DRIVEINFO} has not been successfully completed.";
static const char *DEVICE_FORMAT_FAILED = "Storage Device format failed";
static const char *MAX_USB_DEVICE_LIMIT_REACHED =
        "Exceeded maximum number of allowable USB storage. You can connect up to 6 USB storages to your device";
static const char *DEVICE_UNSUPPORTED_FILESYSTEM =
        "This USB storage has an unsupported system and cannot be read.";

//States that should match the state map entries
enum DeviceState {
    CONNECTED_STATE, MOUNTED_STATE, RECONNECTING_STATE, DISCONNECTED_STATE
};

enum ErrorCode {
    NO_ERROR = 0,
    FORMAT_FAILED,
    USB_DEVICE_LIMIT_REACHED,
    UNSUPPORTED_FILESYSTEM
};

enum EventType {
    ATTACHED_DEVICE_STATUS_LIST,
    ATTACHED_STORAGE_DEVICE_LIST,
    ATTACHED_NONSTORAGE_DEVICE_LIST
};

//Device details
struct Device {
    int deviceNumber;
    std::string deviceType;
    std::string deviceStatus;
    DeviceState deviceState = DeviceState::CONNECTED_STATE;
};

// Event details
struct Event {
    EventType type;
    std::vector<Device> devices;
    std::unordered_set<int> deviceNums;
};

inline const char* getErrorText(ErrorCode errorCode) {
    switch (errorCode) {
    case FORMAT_FAILED: {
        return DEVICE_FORMAT_FAILED;
    }
    case USB_DEVICE_LIMIT_REACHED: {
        return MAX_USB_DEVICE_LIMIT_REACHED;
    }
    case UNSUPPORTED_FILESYSTEM: {
        return DEVICE_UNSUPPORTED_FILESYSTEM;
    }
    default:
        return "";
    }
}

void getDeviceTypeString(std::string &text, std::string &deviceType) {

    if (0 == deviceType.compare("BLUETOOTH")) {
        text = "Bluetooth device";
    } else if (0 == deviceType.compare("HID")) {
        text = "HID device";
    } else if (0 == deviceType.compare("SOUND")) {
        text = "Sound device";
    } else if (0 == deviceType.compare("USB_STORAGE")) {
        text = "Storage device";
    } else if (0 == deviceType.compare("CAM")) {
        text = "Camera device";
    } else if (0 == deviceType.compare("XPAD")) {
        text = "XPAD device";
    } else if (0 == deviceType.compare("MTP")) {
        text = "MTP device";
    } else if (0 == deviceType.compare("PTP")) {
        text = "PTP device";
    } else if (0 == deviceType.compare("CDC")) {
        text = "USB device";
    } else {
        text = "Unknown device";
    }
}

void getToastText(std::string &text, std::string deviceType,
        std::string deviceStatus) {
    getDeviceTypeString(text, deviceType);
    text += (" is " + deviceStatus);
}
} // namespace PdmUtils
