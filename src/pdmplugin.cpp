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

#include "pdmplugin.h"
#include "config.h"
#include "logging.h"
#include "errors.h"

using namespace pbnjson;
using namespace EventMonitor;

PmLogContext logContext;

static const std::string SETTINGS_ICON_URL =
    "/usr/palm/applications/com.palm.app.settings/icon.png";

static const std::string PDM_ATTACHED_DEVICES_QUERY = "luna://com.webos.service.pdm/getAttachedDeviceStatus";
static const std::string PDM_ATTACHED_STORAGE_DEVICES_QUERY = "luna://com.webos.service.pdm/getAttachedStorageDeviceList";
static const std::string PDM_ATTACHED_NONSTORAGE_DEVICES_QUERY = "luna://com.webos.service.pdm/getAttachedNonStorageDeviceList";

//notification Icons
const std::string DEVICE_CONNECTED_ICON_PATH = "/usr/share/physical-device-manager/usb_connect.png";

static const unsigned int TOAST_BOOT_BLOCK_TIME_MS = 7000;

const char *requiredServices[] = {"com.webos.service.pdm",
                                  nullptr
                                 };

PmLogContext pluginLogContext;

EventMonitor::Plugin *instantiatePlugin(int version,
                                        EventMonitor::Manager *manager)
{
	if (version != EventMonitor::API_VERSION)
	{
		return nullptr;
	}

	return new PdmPlugin(manager);
}

PdmPlugin::PdmPlugin(Manager *_manager):
	PluginBase(_manager, WEBOS_LOCALIZATION_PATH),
	toastsBlocked(false)
{
}

PdmPlugin::~PdmPlugin()
{
}

void PdmPlugin::blockToasts(unsigned int timeMs)
{
	this->toastsBlocked = true;
	LOG_DEBUG("Toast block on");

	auto clearBlock = [this](const std::string& timeoutId)
	{
		LOG_DEBUG("Toast block off");
		this->toastsBlocked = false;
	};

	//Will replace previous timeout if any
	this->manager->setTimeout("toastUnblock", timeMs, false, clearBlock);
}

void PdmPlugin::startMonitoring()
{
    LOG_INFO(MSGID_PDM_PLUGIN_INFO, 0, "Pdm monitoring starts");

    this->blockToasts(TOAST_BOOT_BLOCK_TIME_MS);

    JValue params = JObject {{}};

    this->manager->subscribeToMethod(
        "attachedDevices",
        PDM_ATTACHED_DEVICES_QUERY,
        params,
        std::bind(&PdmPlugin::attachedDeviceStatusCallback, this,
                  std::placeholders::_1, std::placeholders::_2));

    JValue params2 = JObject {{}};
    this->manager->subscribeToMethod(
        "attachedStorageDeviceList",
        PDM_ATTACHED_STORAGE_DEVICES_QUERY,
        params2,
        std::bind(&PdmPlugin::attachedStorageDeviceListCallback, this,
                  std::placeholders::_1, std::placeholders::_2));

    JValue params3 = JObject {{}};
    this->manager->subscribeToMethod(
        "attachedNonStorageDeviceList",
        PDM_ATTACHED_NONSTORAGE_DEVICES_QUERY,
        params3,
        std::bind(&PdmPlugin::attachedNonStorageDeviceListCallback, this,
                  std::placeholders::_1, std::placeholders::_2));
}

EventMonitor::UnloadResult PdmPlugin::stopMonitoring(
    const std::string& service)
{
	return UNLOAD_OK;
}

void PdmPlugin::attachedDeviceStatusCallback(JValue &previousValue, JValue &value)
{
    LOG_DEBUG("%s", __FUNCTION__);

    if (!this->toastsBlocked) {
        if (previousValue.isNull()) {
            LOG_DEBUG("%s previousValue null", __FUNCTION__);
//        return;
        } else {
            LOG_DEBUG("%s previousValue: %s", __FUNCTION__, previousValue.stringify().c_str());
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
    }
}

void PdmPlugin::attachedStorageDeviceListCallback(pbnjson::JValue &previousValue,
                           pbnjson::JValue &value) {
    LOG_DEBUG("%s", __FUNCTION__);

    if (!this->toastsBlocked) {
        if (previousValue.isNull()) {
            LOG_DEBUG("%s previousValue null", __FUNCTION__);
//        return;
        } else {
            LOG_DEBUG("%s previousValue: %s", __FUNCTION__, previousValue.stringify().c_str());
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
    }
}

void PdmPlugin::attachedNonStorageDeviceListCallback(pbnjson::JValue &previousValue,
                           pbnjson::JValue &value) {
    LOG_DEBUG("%s", __FUNCTION__);

    if (!this->toastsBlocked) {
        if (previousValue.isNull()) {
            LOG_DEBUG("%s previousValue null", __FUNCTION__);
//        return;
        } else {
            LOG_DEBUG("%s previousValue: %s", __FUNCTION__, previousValue.stringify().c_str());
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
            LOG_DEBUG("%s deviceType: %s", __FUNCTION__, deviceType);
            event.devices.push_back(device);
            event.deviceNums.insert(deviceNum);
        }
        handleEvent(event);
    } else {
        LOG_DEBUG("%s toast is blocked now", __FUNCTION__);
    }
}

void PdmPlugin::handleEvent(Event event) {
    LOG_DEBUG("%s", __FUNCTION__);

    if(event.type == EventType::ATTACHED_DEVICE_STATUS_LIST) {
        //TODO: Handle it for fsck operations
        return;
    }

    auto &mDevices = mStorageDevices;
    if(event.type == EventType::ATTACHED_NONSTORAGE_DEVICE_LIST) {
        mDevices = mNonStorageDevices;
    }
    for(auto& device: event.devices) {
        auto foundDevice = mDevices.find (device.deviceNumber);
        if (foundDevice != mDevices.end()) {
            LOG_DEBUG("%s device entry found for deviceNum %d", __FUNCTION__, device.deviceNumber);
            //Check for change in state
            if(foundDevice->second.deviceState == device.deviceState) {
                //No action as state has not changed.
                LOG_DEBUG("%s deviceNum %d has no change in state", __FUNCTION__, device.deviceNumber);
            } else {
                //TODO: Handle fsck related state changes
                LOG_DEBUG("%s deviceNum %d handle fsck", __FUNCTION__, device.deviceNumber);
            }
        } else {
            //New device entry
            LOG_DEBUG("%s deviceNum %d new entry", __FUNCTION__, device.deviceNumber);
            mDevices.insert ({device.deviceNumber, device});
            //Send Connected Toast
            std::string message;
            getToastText(message, device.deviceType, "connnnected.........");
            this->manager->createToast(message, //TODO: Use localization
                    DEVICE_CONNECTED_ICON_PATH);
        }
    }
    if(event.devices.empty()) {
        //All connected devices are removed or no connected device exists so send Disconnected toast for all devices
        LOG_DEBUG("%s All connected devices are removed or no connected device exists", __FUNCTION__);
        for (auto device : mDevices) {
            std::string message;
            getToastText(message, device.second.deviceType, "disconnected");
            this->manager->createToast(message,  //TODO: Use localization
                    DEVICE_CONNECTED_ICON_PATH);
        }
        mDevices.clear();
    } else {
        //Check if any devices are removed/disconnected
        LOG_DEBUG("%s Check if any devices are removed/disconnected", __FUNCTION__);
        auto it = mDevices.cbegin();
        while (it != mDevices.cend())
        {
            if (event.deviceNums.find(it->first) == event.deviceNums.cend())
            {
                LOG_DEBUG("%s deviceNum %d Device has been disconnected", __FUNCTION__, it->first);
                //Device has been disconnected
                std::string message;
                getToastText(message, it->second.deviceType, "disconnnnected....");
                this->manager->createToast(message,  //TODO: Use localization
                        DEVICE_CONNECTED_ICON_PATH);
                it = mDevices.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}
