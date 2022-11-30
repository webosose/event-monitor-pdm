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
}

EventMonitor::UnloadResult PdmPlugin::stopMonitoring(
    const std::string& service)
{
	return UNLOAD_OK;
}

