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
#include <map>
#include <string>

namespace PdmUtils {
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
static const char *MAX_USB_DEVICE_LIMIT_REACHED =
        "Exceeded maximum number of allowable USB storage. You can connect up to 6 USB storages to your device";

//Alert IDs
const std::string ALERT_ID_USB_STORAGE_DEV_REMOVED = "usbStorageDevRemoved";
const std::string ALERT_ID_USB_STORAGE_DEV_UNSUPPORTED_FS =
        "usbStorageDevUnsupportedFs";
const std::string ALERT_ID_USB_MAX_STORAGE_DEVCIES = "usbMaxStorageDevices";
const std::string ALERT_ID_USB_STORAGE_FSCK_TIME_OUT =
        "usbStorageDevicesFsckTimeOut";

enum EventType {
    ATTACHED_STORAGE_DEVICE_LIST, ATTACHED_NONSTORAGE_DEVICE_LIST
};

enum PdmEventType {
    CONNECTING_EVENT = 0,
    MAX_COUNT_REACHED_EVENT,
    REMOVE_BEFORE_MOUNT_EVENT,
    REMOVE_BEFORE_MOUNT_MTP_EVENT,
    UNSUPPORTED_FS_FORMAT_NEEDED_EVENT,
    FSCK_TIMED_OUT_EVENT,
    FORMAT_STARTED_EVENT,
    FORMAT_SUCCESS_EVENT,
    FORMAT_FAIL_EVENT,
    REMOVE_UNSUPPORTED_FS_EVENT
};

enum DeviceEventType {
    STORAGE_DEVICE = 0,
    NON_STORAGE_DEVICE,
    ALL_DEVICE,
    SOUND_DEVICE,
    HID_DEVICE,
    VIDEO_DEVICE,
    GAMEPAD_DEVICE,
    MTP_DEVICE,
    PTP_DEVICE,
    BLUETOOTH_DEVICE,
    CDC_DEVICE,
    AUTO_ANDROID_DEVICE,
    NFC_DEVICE,
    UNKNOWN_DEVICE
};

//Device details
struct Device {
    int deviceNumber;
    std::string deviceType;
    std::string deviceStatus;
};

// Event details
struct Event {
    EventType type;
    std::vector<Device> devices;
    std::unordered_set<int> deviceNums;
};

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

std::string getDeviceTypeString(int deviceType) {

    std::string device;
    switch (deviceType) {
    case STORAGE_DEVICE: {
        device = "Storage device";
        break;
    }
    case NON_STORAGE_DEVICE: {
        device = "";
        break;
    }
    case SOUND_DEVICE: {
        device = "Sound device";
        break;
    }
    case HID_DEVICE: {
        device = "HID device";
        break;
    }
    case VIDEO_DEVICE: {
        device = "Camera device";
        break;
    }
    case GAMEPAD_DEVICE: {
        device = "XPAD device";
        break;
    }
    case MTP_DEVICE: {
        device = "MTP device";
        break;
    }
    case PTP_DEVICE: {
        device = "PTP device";
        break;
    }
    case BLUETOOTH_DEVICE: {
        device = "Bluetooth device";
        break;
    }
    case CDC_DEVICE: {
        device = "USB device";
        break;
    }
    case AUTO_ANDROID_DEVICE: {
        device = "Android device";
        break;
    }
    case NFC_DEVICE: {
        device = "NFC device";
        break;
    }
    default: {
        device = "Unknown device";
    }
    }

    return device;
}

void getToastText(std::string &text, std::string deviceType,
        std::string deviceStatus) {
    getDeviceTypeString(text, deviceType);
    text += (" is " + deviceStatus);
}

std::string format(std::string text,
        std::map<std::string, std::string> values) {
    std::string formatted = text;
    if (!values.empty()) {
        std::string keyInBraces;

        for (std::map<std::string, std::string>::iterator it = values.begin();
                it != values.end(); ++it) {
            keyInBraces = "{" + it->first + "}";
            if (formatted.find(keyInBraces) != std::string::npos)
                formatted.replace(formatted.find(keyInBraces),
                        keyInBraces.length(), it->second);
        }
    }

    return formatted;
}
} // namespace PdmUtils
