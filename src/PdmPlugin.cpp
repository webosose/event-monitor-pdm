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

#include "PdmPlugin.h"

#include "config.h"
#include "Errors.h"
#include "Logging.h"

#include <pbnjson.hpp>
#include <functional>

using namespace pbnjson;
using namespace EventMonitor;

PmLogContext logContext;

static const std::string SETTINGS_ICON_URL =
        "/usr/palm/applications/com.palm.app.settings/icon.png";

static const std::string PDM_ATTACHED_DEVICES_QUERY =
        "luna://com.webos.service.pdm/getAttachedDeviceStatus";
static const std::string PDM_ATTACHED_STORAGE_DEVICES_QUERY =
        "luna://com.webos.service.pdm/getAttachedStorageDeviceList";
static const std::string PDM_ATTACHED_NONSTORAGE_DEVICES_QUERY =
        "luna://com.webos.service.pdm/getAttachedNonStorageDeviceList";

//notification Icons
const std::string DEVICE_CONNECTED_ICON_PATH =
        "/usr/share/physical-device-manager/usb_connect.png";

static const unsigned int TOAST_BOOT_BLOCK_TIME_MS = 7000;

const char *requiredServices[] = { "com.webos.service.pdm", nullptr };

PmLogContext pluginLogContext;

std::function<void(std::string)> cppSignalHandler = NULL;

EventMonitor::Plugin* instantiatePlugin(int version,
        EventMonitor::Manager *manager) {
    if (version != EventMonitor::API_VERSION) {
        return nullptr;
    }

    return new PdmPlugin(manager);
}

PdmPlugin::PdmPlugin(Manager *_manager) :
        PluginBase(_manager, WEBOS_LOCALIZATION_PATH), toastsBlocked(false) {
    struct sigaction act;
    cppSignalHandler = std::bind(&PdmPlugin::handlePdmEvent, this,
            std::placeholders::_1);
    act.sa_sigaction = signalHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR2, &act, 0);
}

PdmPlugin::~PdmPlugin() {
}

void PdmPlugin::signalHandler(int signum, siginfo_t *sig_info, void *ucontext) {
    LOG_DEBUG("%s %d signal callback for signal number: %d", __FUNCTION__,
            __LINE__, signum);
    int shmId;
    char *sharedMem;
    unsigned int payloadLength;
    std::string signalPayload;

    if (sig_info) {
        payloadLength = sig_info->si_value.sival_int;
        LOG_DEBUG("%s %d payloadLength: %d", __FUNCTION__, __LINE__, payloadLength);
        shmId = shmget(PDM_SHM_KEY, payloadLength, 0);
        if (shmId != -1) {
            sharedMem = (char*) shmat(shmId, (void*) 0, 0);

            if (sharedMem != nullptr) {
                signalPayload = sharedMem;
                shmdt(sharedMem);
                LOG_DEBUG("%s %d payload: %s payloadLength:%d", __FUNCTION__, __LINE__, signalPayload.c_str(), payloadLength);
                cppSignalHandler(signalPayload);
            } else {
                LOG_DEBUG("%s no shared mem", __FUNCTION__);
            }
        }
    } else {
        LOG_DEBUG("%s signalinfo not found", __FUNCTION__);
    }
}

void PdmPlugin::handlePdmEvent(std::string payload) {
    LOG_DEBUG("%s", __FUNCTION__);
    pbnjson::JSchema parseSchema = pbnjson::JSchema::AllSchema();

    pbnjson::JDomParser parser;

    if (!parser.parse(payload, parseSchema)) {
        LOG_DEBUG("%s payload parsing failed", __FUNCTION__);
        return;
    }

    pbnjson::JValue eventObject = parser.getDom();
    if (!eventObject.hasKey("pdmEvent")) {
        LOG_DEBUG("%s incomplete payload received", __FUNCTION__);
        return;
    }

    if (!eventObject.hasKey("parameters")) {
        LOG_DEBUG("%s incomplete payload received no parameters", __FUNCTION__);
        return;
    }

    pbnjson::JValue params = eventObject["parameters"];

    auto pdmEvent = eventObject["pdmEvent"].asNumber<int>();
    LOG_DEBUG("%s pdmEvent: %d", __FUNCTION__, pdmEvent);

    switch (pdmEvent) {
    case CONNECTING_EVENT: {
        if (!params.hasKey("deviceType")) {
            LOG_DEBUG("%s incomplete payload received", __FUNCTION__);
        } else {
            auto deviceType = params["deviceType"].asNumber<int>();
            showConnectingToast(deviceType);
        }
        break;
    }
    case MAX_COUNT_REACHED_EVENT: {
        createAlertForMaxUsbStorageDevices();
        break;
    }
    case REMOVE_BEFORE_MOUNT_EVENT: {
        if (!params.hasKey("deviceNum")) {
            LOG_DEBUG("%s incomplete payload received", __FUNCTION__);
        } else {
            auto deviceNum = params["deviceNum"].asString();
            createAlertForUnmountedDeviceRemoval(deviceNum);
        }
        break;
    }
    case REMOVE_BEFORE_MOUNT_MTP_EVENT: {
        if (!params.hasKey("driveName")) {
            LOG_DEBUG("%s incomplete payload received", __FUNCTION__);
        } else {
            auto driveName = params["driveName"].asString();
            unMountMtpDeviceAlert(driveName);
        }
        break;
    }
    case UNSUPPORTED_FS_FORMAT_NEEDED_EVENT: {
        if (!params.hasKey("deviceNum")) {
            LOG_DEBUG("%s incomplete payload received", __FUNCTION__);
        } else {
            auto deviceNum = params["deviceNum"].asString();
            createAlertForUnsupportedFileSystem(deviceNum);
        }
        break;
    }
    case FSCK_TIMED_OUT_EVENT: {
        if (!params.hasKey("deviceNum") || !params.hasKey("mountName")) {
            LOG_DEBUG("%s incomplete payload received", __FUNCTION__);
        } else {
            auto deviceNum = params["deviceNum"].asString();
            auto deviceName = params["mountName"].asString();
            createAlertForFsckTimeout(deviceNum, deviceName);
        }
        break;
    }
    case FORMAT_STARTED_EVENT: {
        if (!params.hasKey("driveInfo")) {
            LOG_DEBUG("%s incomplete payload received", __FUNCTION__);
        } else {
            auto driveInfo = params["driveInfo"].asString();
            showFormatStartedToast(driveInfo);
        }
        break;
    }
    case FORMAT_SUCCESS_EVENT: {
        if (!params.hasKey("driveInfo")) {
            LOG_DEBUG("%s incomplete payload received", __FUNCTION__);
        } else {
            auto driveInfo = params["driveInfo"].asString();
            showFormatSuccessToast(driveInfo);
        }
        break;
    }
    case FORMAT_FAIL_EVENT: {
        if (!params.hasKey("driveInfo")) {
            LOG_DEBUG("%s incomplete payload received", __FUNCTION__);
        } else {
            auto driveInfo = params["driveInfo"].asString();
            showFormatFailToast(driveInfo);
        }
        break;
    }
    case REMOVE_UNSUPPORTED_FS_EVENT: {
        if (!params.hasKey("deviceNum")) {
            LOG_DEBUG("%s incomplete payload received", __FUNCTION__);
        } else {
            auto deviceNum = params["deviceNum"].asString();
            closeUnsupportedFsAlert(deviceNum);
        }
        break;
    }
    default: {
        LOG_DEBUG("%s unknown pdmEvent: %d", __FUNCTION__, pdmEvent);
    }
    }

}

void PdmPlugin::createAlertForMaxUsbStorageDevices() {
    LOG_DEBUG("%s", __FUNCTION__);

    JArray buttons = JArray { };
    std::string message = this->getLocString(MAX_USB_DEVICE_LIMIT_REACHED);
    buttons.append(JObject { { "label", this->getLocString("OK") }, {
            "position", "middle" }, { "params",
            JObject { { "action", "close" } } } });

    JValue onClose = JObject { };

    this->manager->createAlert(ALERT_ID_USB_MAX_STORAGE_DEVCIES, "", // No title
            message, false, "", // No icon
            buttons, onClose);
}

void PdmPlugin::unMountMtpDeviceAlert(std::string driveName) {
    LOG_DEBUG("%s", __FUNCTION__);

    JArray buttons = JArray { };
    std::string message = this->getLocString(REMOVE_USB_DEVICE_BEFORE_MOUNT);
    buttons.append(JObject { { "label", this->getLocString("OK") }, {
            "position", "middle" }, { "params",
            JObject { { "action", "close" } } } });

    JValue onClose = JObject { };

    this->manager->createAlert(ALERT_ID_USB_STORAGE_DEV_REMOVED + driveName, "", // No title
            message, false, "", // No icon
            buttons, onClose);
}

void PdmPlugin::createAlertForUnmountedDeviceRemoval(std::string deviceNumber) {
    LOG_DEBUG("%s", __FUNCTION__);
    std::string alertIdFsckTimeout = ALERT_ID_USB_STORAGE_FSCK_TIME_OUT
            + deviceNumber;
    this->manager->closeAlert(alertIdFsckTimeout);

    JArray buttons = JArray { };
    std::string message = this->getLocString(REMOVE_USB_DEVICE_BEFORE_MOUNT);
    buttons.append(JObject { { "label", this->getLocString("OK") }, {
            "position", "middle" }, { "params",
            JObject { { "action", "close" } } } });

    JValue onClose = JObject { };

    this->manager->createAlert(ALERT_ID_USB_STORAGE_DEV_REMOVED + deviceNumber,
            "", // No title
            message, false, "", // No icon
            buttons, onClose);
}

void PdmPlugin::createAlertForUnsupportedFileSystem(std::string deviceNumber) {
    LOG_DEBUG("%s", __FUNCTION__);

    JArray buttons = JArray { };
    std::string message = this->getLocString(USB_STORAGE_DEV_UNSUPPORTED_FS);
    buttons.append(JObject { { "label", this->getLocString("OK") }, {
            "position", "middle" }, { "params",
            JObject { { "action", "close" } } } });

    JValue onClose = JObject { };

    this->manager->createAlert(
            ALERT_ID_USB_STORAGE_DEV_UNSUPPORTED_FS + deviceNumber, "", // No title
            message, false, "", // No icon
            buttons, onClose);
}

void PdmPlugin::closeUnsupportedFsAlert(std::string deviceNumber) {
    LOG_DEBUG("%s", __FUNCTION__);
    this->manager->closeAlert(
            ALERT_ID_USB_STORAGE_DEV_UNSUPPORTED_FS + deviceNumber);
}

void PdmPlugin::showConnectingToast(int deviceType) {
    LOG_DEBUG("%s", __FUNCTION__);
    std::string message = getDeviceTypeString(deviceType) + " is connecting.";
    LOG_DEBUG("%s sending toast for connecting device", __FUNCTION__);
    this->manager->createToast(message, //TODO: Use localization
            DEVICE_CONNECTED_ICON_PATH);
}

void PdmPlugin::createAlertForFsckTimeout(std::string deviceNumber,
        std::string deviceName) {
    LOG_DEBUG("%s", __FUNCTION__);

    std::string message = this->getLocString(USB_STORAGE_FSCK_TIME_OUT);
    pbnjson::JValue buttons = pbnjson::JArray { pbnjson::JObject { { "label",
            this->getLocString("CHECK & REPAIR") }, { "onclick",
            "luna://com.webos.service.pdm/mountandFullFsck" },
            { "params", pbnjson::JObject { { "needFsck", true }, { "mountName",
                    deviceName } } } }, pbnjson::JObject { { "label",
            this->getLocString("OPEN NOW") }, { "onclick",
            "luna://com.webos.service.pdm/mountandFullFsck" }, { "params",
            pbnjson::JObject { { "needFsck", false },
                    { "mountName", deviceName } } } } };

    JValue onClose = JObject { };

    this->manager->createAlert(
            ALERT_ID_USB_STORAGE_FSCK_TIME_OUT + deviceNumber, "", // No title
            message, false, "", // No icon
            buttons, onClose);
}

void PdmPlugin::showFormatStartedToast(std::string driveInfo) {
    LOG_DEBUG("%s", __FUNCTION__);

    std::map < std::string, std::string > values;
    values.insert(pair<string, string>("DRIVEINFO", driveInfo));

    std::string message = format(STORAGE_DEV_FORMAT_STARTED, values);
    LOG_DEBUG("%s sending toast for format started..", __FUNCTION__);
    this->manager->createToast(message, //TODO: Use localization
            DEVICE_CONNECTED_ICON_PATH);
}

void PdmPlugin::showFormatSuccessToast(std::string driveInfo) {
    LOG_DEBUG("%s", __FUNCTION__);

    std::map < std::string, std::string > values;
    values.insert(pair<string, string>("DRIVEINFO", driveInfo));

    std::string message = format(STORAGE_DEV_FORMAT_SUCCESS, values);
    LOG_DEBUG("%s sending toast for format success..", __FUNCTION__);
    this->manager->createToast(message, //TODO: Use localization
            DEVICE_CONNECTED_ICON_PATH);
}

void PdmPlugin::showFormatFailToast(std::string driveInfo) {
    LOG_DEBUG("%s", __FUNCTION__);

    std::map < std::string, std::string > values;
    values.insert(pair<string, string>("DRIVEINFO", driveInfo));

    std::string message = format(STORAGE_DEV_FORMAT_FAIL, values);
    LOG_DEBUG("%s sending toast for format fail..", __FUNCTION__);
    this->manager->createToast(message, //TODO: Use localization
            DEVICE_CONNECTED_ICON_PATH);
}

void PdmPlugin::blockToasts(unsigned int timeMs) {
    this->toastsBlocked = true;
    LOG_DEBUG("Toast block on");

    auto clearBlock = [this](const std::string &timeoutId) {
        LOG_DEBUG("Toast block off");
        this->toastsBlocked = false;
    };

    //Will replace previous timeout if any
    this->manager->setTimeout("toastUnblock", timeMs, false, clearBlock);
}

void PdmPlugin::startMonitoring() {
    LOG_INFO(MSGID_PDM_PLUGIN_INFO, 0, "Pdm monitoring starts");

    this->blockToasts(TOAST_BOOT_BLOCK_TIME_MS);

    JValue params = JObject { { } };

    this->manager->subscribeToMethod("attachedDevices",
            PDM_ATTACHED_DEVICES_QUERY, params,
            std::bind(&PdmPlugin::attachedDeviceStatusCallback, this,
                    std::placeholders::_1, std::placeholders::_2));

    this->manager->subscribeToMethod("attachedStorageDeviceList",
            PDM_ATTACHED_STORAGE_DEVICES_QUERY, params,
            std::bind(&PdmPlugin::attachedStorageDeviceListCallback, this,
                    std::placeholders::_1, std::placeholders::_2));

    this->manager->subscribeToMethod("attachedNonStorageDeviceList",
            PDM_ATTACHED_NONSTORAGE_DEVICES_QUERY, params,
            std::bind(&PdmPlugin::attachedNonStorageDeviceListCallback, this,
                    std::placeholders::_1, std::placeholders::_2));
}

EventMonitor::UnloadResult PdmPlugin::stopMonitoring(
        const std::string &service) {
    return UNLOAD_OK;
}

void PdmPlugin::attachedDeviceStatusCallback(JValue &previousValue,
        JValue &value) {
    LOG_DEBUG("%s", __FUNCTION__);

    if (!this->toastsBlocked) {
        if (previousValue.isNull()) {
            LOG_DEBUG("%s previousValue null", __FUNCTION__);
            saveAlreadyConnectedDeviceList(previousValue, value,
                    EventType::ATTACHED_DEVICE_STATUS_LIST);
            return;
        } else {
            LOG_DEBUG("%s previousValue: %s", __FUNCTION__,
                    previousValue.stringify().c_str());
        }

        if (value.isNull()) {
            LOG_DEBUG("%s value null", __FUNCTION__);
            return;
        } else {
            LOG_DEBUG("%s value: %s", __FUNCTION__, value.stringify().c_str());
        }

        if (!value.hasKey("deviceStatusList"))
            return;

        Event event;
        event.type = EventType::ATTACHED_DEVICE_STATUS_LIST;

        JValue deviceStatusListObj = Array();
        deviceStatusListObj = value["deviceStatusList"];
        int deviceStatusListObjLength = deviceStatusListObj.arraySize();

        for (auto i = 0; i < deviceStatusListObjLength; i++) {
            Device device;
            if (!deviceStatusListObj[i].hasKey("deviceNum"))
                continue;

            auto deviceNum =
                    deviceStatusListObj[i]["deviceNum"].asNumber<int>();
            LOG_DEBUG("%s deviceNum: %d", __FUNCTION__, deviceNum);
            device.deviceNumber = deviceNum;

            if (!deviceStatusListObj[i].hasKey("deviceStatus"))
                continue;

            auto deviceStatus =
                    deviceStatusListObj[i]["deviceStatus"].asString();
            LOG_DEBUG("%s deviceStatus: %s", __FUNCTION__, deviceStatus);
            device.deviceStatus = deviceStatus;

            if (!deviceStatusListObj[i].hasKey("deviceType"))
                continue;

            auto deviceType = deviceStatusListObj[i]["deviceType"].asString();
            LOG_DEBUG("%s deviceType: %s", __FUNCTION__, deviceType);
            device.deviceType = deviceType;
            event.devices.push_back(device);
            event.deviceNums.insert(deviceNum);
        }
        handleEvent(event);
    } else {
        LOG_DEBUG("%s toast is blocked now", __FUNCTION__);
        saveAlreadyConnectedDeviceList(previousValue, value,
                EventType::ATTACHED_DEVICE_STATUS_LIST);
    }
}

void PdmPlugin::attachedStorageDeviceListCallback(
        pbnjson::JValue &previousValue, pbnjson::JValue &value) {
    LOG_DEBUG("%s", __FUNCTION__);

    if (!this->toastsBlocked) {
        if (previousValue.isNull()) {
            LOG_DEBUG("%s previousValue null", __FUNCTION__);
            saveAlreadyConnectedDeviceList(previousValue, value,
                    EventType::ATTACHED_STORAGE_DEVICE_LIST);
            return;
        } else {
            LOG_DEBUG("%s previousValue: %s", __FUNCTION__,
                    previousValue.stringify().c_str());
        }

        if (value.isNull()) {
            LOG_DEBUG("%s value null", __FUNCTION__);
            return;
        } else {
            LOG_DEBUG("%s value: %s", __FUNCTION__, value.stringify().c_str());
        }

        if (!value.hasKey("storageDeviceList"))
            return;

        Event event;
        event.type = EventType::ATTACHED_STORAGE_DEVICE_LIST;

        JValue storageDeviceListObj = Array();
        storageDeviceListObj = value["storageDeviceList"];
        int storageDeviceListObjLength = storageDeviceListObj.arraySize();

        for (auto i = 0; i < storageDeviceListObjLength; i++) {
            Device device;
            if (!storageDeviceListObj[i].hasKey("deviceNum"))
                continue;

            auto deviceNum =
                    storageDeviceListObj[i]["deviceNum"].asNumber<int>();
            device.deviceNumber = deviceNum;
            LOG_DEBUG("%s deviceNum: %d", __FUNCTION__, deviceNum);

            if (!storageDeviceListObj[i].hasKey("deviceType"))
                continue;

            auto deviceType = storageDeviceListObj[i]["deviceType"].asString();
            device.deviceType = deviceType;
            LOG_DEBUG("%s deviceType: %s", __FUNCTION__, deviceType);
            event.devices.push_back(device);
            event.deviceNums.insert(deviceNum);
        }
        handleEvent(event);
    } else {
        LOG_DEBUG("%s toast is blocked now", __FUNCTION__);
        saveAlreadyConnectedDeviceList(previousValue, value,
                EventType::ATTACHED_STORAGE_DEVICE_LIST);
    }
}

void PdmPlugin::attachedNonStorageDeviceListCallback(
        pbnjson::JValue &previousValue, pbnjson::JValue &value) {
    LOG_DEBUG("%s", __FUNCTION__);

    if (!this->toastsBlocked) {
        if (previousValue.isNull()) {
            LOG_DEBUG("%s previousValue null", __FUNCTION__);
            saveAlreadyConnectedDeviceList(previousValue, value,
                    EventType::ATTACHED_NONSTORAGE_DEVICE_LIST);
            return;
        } else {
            LOG_DEBUG("%s previousValue: %s", __FUNCTION__,
                    previousValue.stringify().c_str());
        }

        if (value.isNull()) {
            LOG_DEBUG("%s value null", __FUNCTION__);
            return;
        } else {
            LOG_DEBUG("%s value: %s", __FUNCTION__, value.stringify().c_str());
        }

        if (!value.hasKey("nonStorageDeviceList"))
            return;

        Event event;
        event.type = EventType::ATTACHED_NONSTORAGE_DEVICE_LIST;

        JValue nonStorageDeviceListObj = Array();
        nonStorageDeviceListObj = value["nonStorageDeviceList"];
        int nonStorageDeviceListObjLength = nonStorageDeviceListObj.arraySize();

        for (auto i = 0; i < nonStorageDeviceListObjLength; i++) {
            Device device;
            if (!nonStorageDeviceListObj[i].hasKey("deviceNum"))
                continue;

            auto deviceNum = nonStorageDeviceListObj[i]["deviceNum"].asNumber<
                    int>();
            device.deviceNumber = deviceNum;
            LOG_DEBUG("%s deviceNum: %d", __FUNCTION__, deviceNum);

            if (!nonStorageDeviceListObj[i].hasKey("deviceType"))
                continue;

            auto deviceType =
                    nonStorageDeviceListObj[i]["deviceType"].asString();
            device.deviceType = deviceType;
            LOG_DEBUG("%s deviceType: %s", __FUNCTION__, device.deviceType);
            event.devices.push_back(device);
            event.deviceNums.insert(deviceNum);
        }
        handleEvent(event);
    } else {
        LOG_DEBUG("%s toast is blocked now", __FUNCTION__);
        saveAlreadyConnectedDeviceList(previousValue, value,
                EventType::ATTACHED_NONSTORAGE_DEVICE_LIST);
    }
}

void PdmPlugin::handleEvent(Event event) {
    LOG_DEBUG("%s", __FUNCTION__);

    if (event.type == EventType::ATTACHED_DEVICE_STATUS_LIST) {
        //TODO: Handle it for fsck operations
        return;
    }

    auto &mDevices =
            (event.type == EventType::ATTACHED_STORAGE_DEVICE_LIST) ?
                    mStorageDevices : mNonStorageDevices;
    std::set<int> deviceNums;
    for (auto &device : event.devices) {
        auto foundDevice = mDevices.find(device.deviceNumber);
        if (foundDevice != mDevices.end()) {
            LOG_DEBUG("%s device entry found for deviceNum %d", __FUNCTION__,
                    device.deviceNumber);
            //Check for change in state
            if (foundDevice->second.deviceState == device.deviceState) {
                //No action as state has not changed.
                LOG_DEBUG("%s deviceNum %d has no change in state",
                        __FUNCTION__, device.deviceNumber);
            } else {
                //TODO: Handle fsck related state changes
                LOG_DEBUG("%s deviceNum %d handle fsck", __FUNCTION__,
                        device.deviceNumber);
            }
        } else {
            //New device entry
            //Handle multiple device entries for same device number
            LOG_DEBUG("%s deviceNum %d deviceType %s new entry", __FUNCTION__,
                    device.deviceNumber, device.deviceType);
            device.deviceStatus = "connected.";
            mNewDevices.insert( { device.deviceNumber, device });
            deviceNums.insert(device.deviceNumber);
        }
    }
    if (event.devices.empty()) {
        //All connected devices are removed or no connected device exists so send Disconnected toast for all devices
        LOG_DEBUG(
                "%s All connected devices are removed or no connected device exists",
                __FUNCTION__);
        for (auto device : mDevices) {
            std::string message;
            getToastText(message, device.second.deviceType, "disconnected.");
            LOG_DEBUG("%s sending toast for disconnected device num: %d",
                    __FUNCTION__, device.second.deviceNumber);
            this->manager->createToast(message,  //TODO: Use localization
                    DEVICE_CONNECTED_ICON_PATH);
        }
        mDevices.clear();
    } else {
        //Check if any devices are removed/disconnected
        LOG_DEBUG("%s Check if any devices are removed/disconnected",
                __FUNCTION__);
        auto it = mDevices.begin();
        while (it != mDevices.end()) {
            if (event.deviceNums.find(it->first) == event.deviceNums.cend()) {
                LOG_DEBUG("%s deviceNum %d device has been disconnected",
                        __FUNCTION__, it->first);
                //Device has been disconnected
                std::string message;
                getToastText(message, it->second.deviceType, "disconnected.");
                LOG_DEBUG("%s sending toast for disconnected devicenumber: %d",
                        __FUNCTION__, it->second.deviceNumber);
                this->manager->createToast(message, //TODO: Use localization
                        DEVICE_CONNECTED_ICON_PATH);
                it = mDevices.erase(it);
            } else {
                ++it;
            }
        }
    }
    processNewEntries(deviceNums, mNewDevices, mDevices);
}

void PdmPlugin::processNewEntries(std::set<int> &deviceNums,
        std::unordered_multimap<int, Device> &newDevices,
        std::unordered_map<int, Device> &mDevices) {
    LOG_DEBUG("%s", __FUNCTION__);

    for (auto x : deviceNums) {
        auto count = newDevices.count(x);
        LOG_DEBUG("%s deviceNum %d count %d ", __FUNCTION__, x, count);

        if (count) {
            Device device = newDevices.find(x)->second;
            //If count is 1, then deviceType is always proper
            if (count >= 2) {
                auto range = newDevices.equal_range(device.deviceNumber);
                std::string deviceType;
                for (auto it = range.first; it != range.second; ++it) {
                    deviceType = getDeviceType(deviceType,
                            it->second.deviceType);
                }
                device.deviceType = deviceType;
                LOG_DEBUG("%s actual deviceType: %s", __FUNCTION__,
                        deviceType.c_str());
            }

            LOG_DEBUG("%s updated deviceType: %s", __FUNCTION__,
                    device.deviceType.c_str());
            mDevices.insert( { device.deviceNumber, device }); //save device entry
            //Display device toast
            std::string message;
            getToastText(message, device.deviceType, device.deviceStatus);
            LOG_DEBUG(
                    "%s sending toast for connected devicenumber: %d type: %s msg: %s",
                    __FUNCTION__, device.deviceNumber,
                    device.deviceType.c_str(), message);
            this->manager->createToast(message, //TODO: Use localization
                    DEVICE_CONNECTED_ICON_PATH);
        }
    }
    newDevices.clear();
}

std::string PdmPlugin::getDeviceType(std::string current,
        std::string received) {
    LOG_DEBUG("%s current: %s received: %s", __FUNCTION__, current, received);
    if (current.empty()) {
        current = received;
    } else if ((0 == current.compare("HID"))
            && (0 != received.compare("HID"))) {
        current = received;
    } else if ((0 == current.compare("SOUND"))
            && (0 == received.compare("CAM"))) {
        current = received;
    }
    LOG_DEBUG("%s new current: %s", __FUNCTION__, current);
    return current;
}

void PdmPlugin::saveAlreadyConnectedDeviceList(pbnjson::JValue &previousValue,
        pbnjson::JValue &value, EventType eventType) {
    LOG_DEBUG("%s", __FUNCTION__);
    if (previousValue.isNull()) {
        LOG_DEBUG("%s previousValue null", __FUNCTION__);
        if (value.isNull()) {
            LOG_DEBUG("%s value null, nothing to save", __FUNCTION__);
            return;
        } else {
            LOG_DEBUG("%s value: %s", __FUNCTION__, value.stringify().c_str());

            switch (eventType) {
            case EventType::ATTACHED_NONSTORAGE_DEVICE_LIST: {
                if (!value.hasKey("nonStorageDeviceList"))
                    return;
                JValue nonStorageDeviceListObj = Array();
                nonStorageDeviceListObj = value["nonStorageDeviceList"];
                int nonStorageDeviceListObjLength =
                        nonStorageDeviceListObj.arraySize();

                for (auto i = 0; i < nonStorageDeviceListObjLength; i++) {
                    Device device;
                    if (!nonStorageDeviceListObj[i].hasKey("deviceNum"))
                        continue;

                    auto deviceNum =
                            nonStorageDeviceListObj[i]["deviceNum"].asNumber<int>();
                    device.deviceNumber = deviceNum;
                    LOG_DEBUG("%s deviceNum: %d", __FUNCTION__, deviceNum);

                    if (!nonStorageDeviceListObj[i].hasKey("deviceType"))
                        continue;

                    auto deviceType =
                            nonStorageDeviceListObj[i]["deviceType"].asString();
                    device.deviceType = deviceType;
                    LOG_DEBUG("%s deviceType: %s", __FUNCTION__, deviceType);
                    auto found = mNonStorageDevices.find(deviceNum);
                    if (found != mNonStorageDevices.end()) {
                        found->second.deviceType = getDeviceType(
                                found->second.deviceType, device.deviceType);
                        LOG_DEBUG("%s proper deviceType: %s", __FUNCTION__,
                                found->second.deviceType);
                    } else {
                        mNonStorageDevices.insert(
                                { device.deviceNumber, device });
                        LOG_DEBUG("%s new deviceType: %s", __FUNCTION__,
                                device.deviceType);
                    }
                }
                break;
            }
            case EventType::ATTACHED_STORAGE_DEVICE_LIST: {
                if (!value.hasKey("storageDeviceList"))
                    return;
                JValue storageDeviceListObj = Array();
                storageDeviceListObj = value["storageDeviceList"];
                int storageDeviceListObjLength =
                        storageDeviceListObj.arraySize();

                for (auto i = 0; i < storageDeviceListObjLength; i++) {
                    Device device;
                    if (!storageDeviceListObj[i].hasKey("deviceNum"))
                        continue;

                    auto deviceNum =
                            storageDeviceListObj[i]["deviceNum"].asNumber<int>();
                    device.deviceNumber = deviceNum;
                    LOG_DEBUG("%s deviceNum: %d", __FUNCTION__, deviceNum);

                    if (!storageDeviceListObj[i].hasKey("deviceType"))
                        continue;

                    auto deviceType =
                            storageDeviceListObj[i]["deviceType"].asString();
                    device.deviceType = deviceType;
                    LOG_DEBUG("%s deviceType: %s", __FUNCTION__, deviceType);

                    auto found = mStorageDevices.find(deviceNum);
                    if (found != mStorageDevices.end()) {
                        found->second.deviceType = getDeviceType(
                                found->second.deviceType, device.deviceType);
                        LOG_DEBUG("%s proper deviceType: %s", __FUNCTION__,
                                found->second.deviceType);
                    } else {
                        mStorageDevices.insert(
                                { device.deviceNumber, device });
                    }
                }
                break;
            }
            default: {
                LOG_DEBUG("%s Unknown event type %s", __FUNCTION__,
                        value.stringify().c_str());
            }
            }
        }
    } else {
        LOG_DEBUG(
                "%s previousValue not null, hence not on boot device status : %s",
                __FUNCTION__, previousValue.stringify().c_str());
    }
}
