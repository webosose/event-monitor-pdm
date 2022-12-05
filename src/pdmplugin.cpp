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

//Alert IDs
const std::string ALERT_ID_USB_STORAGE_DEV_REMOVED = "usbStorageDevRemoved";
const std::string ALERT_ID_USB_STORAGE_DEV_UNSUPPORTED_FS = "usbStorageDevUnsupportedFs";
const std::string ALERT_ID_USB_MAX_STORAGE_DEVCIES = "usbMaxStorageDevices";
const std::string ALERT_ID_USB_STORAGE_FSCK_TIME_OUT = "usbStorageDevicesFsckTimeOut";

// Alert strings
const std::string ALERT_STRING_USB_STORAGE_DEV_UNSUPPORTED_FS = "This USB storage has an unsupported system and cannot be read.";

static const unsigned int TOAST_BOOT_BLOCK_TIME_MS = 7000;
static const unsigned int TOAST_RESUME_BLOCK_TIME_MS = 5000;
static const unsigned int TOAST_SUSPEND_BLOCK_TIME_MS = 5000;


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

void PdmPlugin::attachedDeviceStatusCallback(JValue &previousValue, JValue &value)
{
    LOG_INFO(MSGID_PDM_PLUGIN_INFO, 0, "attachedDeviceStatusCallback");

    if (previousValue.isNull()) {
        LOG_DEBUG("%s previousValue null", __FUNCTION__);
        return;
    }

    if (value.isNull()) {
        LOG_DEBUG("%s value null", __FUNCTION__);
        return;
    }

    if (!value.hasKey("deviceStatusList"))
        return;

    JValue deviceStatusListObj = Array();
    deviceStatusListObj = value["deviceStatusList"];
    int deviceStatusListObjLength = deviceStatusListObj.arraySize();

    for (auto i = 0; i < deviceStatusListObjLength; i++)
    {
        if (!deviceStatusListObj[i].hasKey("deviceNum"))
            continue;

        auto deviceNum = deviceStatusListObj[i]["deviceNum"].asNumber<int>();
        LOG_DEBUG("%s deviceNum: %d", __FUNCTION__, deviceNum);

        if (!deviceStatusListObj[i].hasKey("deviceStatus"))
            continue;

        auto deviceStatus = deviceStatusListObj[i]["deviceStatus"].asString();
        LOG_DEBUG("%s deviceNum: %s", __FUNCTION__, deviceStatus);

        if (!deviceStatusListObj[i].hasKey("deviceType"))
            continue;

        auto deviceType = deviceStatusListObj[i]["deviceType"].asString();
        LOG_DEBUG("%s deviceType: %s", __FUNCTION__, deviceType);

        if (!deviceStatusListObj[i].hasKey("driveStatusList")) {

            JValue driveStatusListObj = Array();
            driveStatusListObj = deviceStatusListObj[i]["driveStatusList"];
            int driveStatusListObjLength = driveStatusListObj.arraySize();

            for (auto j = 0; j < driveStatusListObjLength; j++)
            {
                if (!driveStatusListObj[j].hasKey("driveName"))
                            continue;
                auto driveName = deviceStatusListObj[i]["driveName"].asString();
                LOG_DEBUG("%s driveName: %s", __FUNCTION__, driveName);

                if (!driveStatusListObj[j].hasKey("driveStatus"))
                            continue;
                auto driveStatus = deviceStatusListObj[i]["driveStatus"].asString();
                LOG_DEBUG("%s driveStatus: %s", __FUNCTION__, driveStatus);
            }
        }
        //TODO: Check is device added. removed or status changed and then create notification
        std::string message;
        getToastText(message, deviceType, deviceStatus);
        this->manager->createToast(
                this->getLocString(message),
                DEVICE_CONNECTED_ICON_PATH);
    }
}

void PdmPlugin::attachedStorageDevicesStatusCallback(pbnjson::JValue &previousValue,
                           pbnjson::JValue &value) {
    LOG_INFO(MSGID_PDM_PLUGIN_INFO, 0, "attachedStorageDevicesStatusCallback");

    if (previousValue.isNull()) {
        LOG_DEBUG("%s previousValue null", __FUNCTION__);
        return;
    }

    if (value.isNull()) {
        LOG_DEBUG("%s value null", __FUNCTION__);
        return;
    }

    if (!value.hasKey("storageDeviceList"))
        return;

    JValue storageDeviceListObj = Array();
    storageDeviceListObj = value["storageDeviceList"];
    int storageDeviceListObjLength = storageDeviceListObj.arraySize();

    for (auto i = 0; i < storageDeviceListObjLength; i++)
    {
        if (!storageDeviceListObj[i].hasKey("deviceNum"))
            continue;

        auto deviceNum = storageDeviceListObj[i]["deviceNum"].asNumber<int>();
        LOG_DEBUG("%s deviceNum: %d", __FUNCTION__, deviceNum);

        if (!storageDeviceListObj[i].hasKey("deviceType"))
            continue;

        auto deviceType = storageDeviceListObj[i]["deviceType"].asString();
        LOG_DEBUG("%s deviceType: %s", __FUNCTION__, deviceType);

        //TODO: Check is device added. removed or status changed and then create notification
        std::string message;
        getToastText(message, deviceType, deviceType + " is connected");
        this->manager->createToast(
                this->getLocString(message),
                DEVICE_CONNECTED_ICON_PATH);
    }
}
void PdmPlugin::attachedNonStorageDevicesCallback(pbnjson::JValue &previousValue,
                           pbnjson::JValue &value) {
    LOG_INFO(MSGID_PDM_PLUGIN_INFO, 0, "attachedNonStorageDevicesCallback");

    if (previousValue.isNull()) {
        LOG_DEBUG("%s previousValue null", __FUNCTION__);
        return;
    }

    if (value.isNull()) {
        LOG_DEBUG("%s value null", __FUNCTION__);
        return;
    }

    if (!value.hasKey("nonStorageDeviceList"))
        return;

    JValue nonStorageDeviceListObj = Array();
    nonStorageDeviceListObj = value["deviceStatusList"];
    int nonStorageDeviceListObjLength = nonStorageDeviceListObj.arraySize();

    for (auto i = 0; i < nonStorageDeviceListObjLength; i++)
    {
        if (!nonStorageDeviceListObj[i].hasKey("deviceNum"))
            continue;

        auto deviceNum = nonStorageDeviceListObj[i]["deviceNum"].asNumber<int>();
        LOG_DEBUG("%s deviceNum: %d", __FUNCTION__, deviceNum);

        if (!nonStorageDeviceListObj[i].hasKey("deviceType"))
            continue;

        auto deviceType = nonStorageDeviceListObj[i]["deviceType"].asString();
        LOG_DEBUG("%s deviceType: %s", __FUNCTION__, deviceType);

        //TODO: Check is device added. removed or status changed and then create notification
        std::string message;
        getToastText(message, deviceType, deviceType + " is connected");
        this->manager->createToast(
                this->getLocString(message),
                DEVICE_CONNECTED_ICON_PATH);
    }
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
        "pdm",
        PDM_ATTACHED_DEVICES_QUERY,
        params,
        std::bind(&PdmPlugin::attachedDeviceStatusCallback, this,
                  std::placeholders::_1, std::placeholders::_2));
    this->manager->subscribeToMethod(
        "pdm",
        PDM_ATTACHED_STORAGE_DEVICES_QUERY,
        params,
        std::bind(&PdmPlugin::attachedStorageDevicesStatusCallback, this,
                  std::placeholders::_1, std::placeholders::_2));
    this->manager->subscribeToMethod(
        "pdm",
        PDM_ATTACHED_NONSTORAGE_DEVICES_QUERY,
        params,
        std::bind(&PdmPlugin::attachedNonStorageDevicesCallback, this,
                  std::placeholders::_1, std::placeholders::_2));
}

EventMonitor::UnloadResult PdmPlugin::stopMonitoring(
    const std::string& service)
{
	return UNLOAD_OK;
}

void PdmPlugin::getDeviceTypeString(std::string& text, std::string& deviceType) {

    if(0 == deviceType.compare("BLUETOOTH")) {
        text = "Bluetooth device";
    } else if(0 == deviceType.compare("HID")) {
        text = "HID device";
    } else if(0 == deviceType.compare("SOUND")) {
        text = "Audio device";
    } else if(0 == deviceType.compare("USB_STORAGE")) {
        text = "USB device";
    } else {
        text = "Unknown device";
    }
}

void PdmPlugin::getToastText(std::string& text, std::string deviceType, std::string deviceStatus)
{
    LOG_DEBUG("%s ", __FUNCTION__);

    getDeviceTypeString(text, deviceType);
    text += (" is " + deviceStatus);
}
