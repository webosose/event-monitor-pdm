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

#include <set>
#include <unordered_map>

#include <event-monitor-api/pluginbase.hpp>

typedef struct Drive {
    std::string driveName;
    std::string driveStatus;
} Drive;

typedef struct Device {
    std::string deviceType;
    std::string deviceNumber;
    std::string deviceStatus;
    std::set<Drive> drives;
} Device;

class PdmPlugin: public EventMonitor::PluginBase {
public:
    PdmPlugin(EventMonitor::Manager *manager);
    virtual ~PdmPlugin();
    void startMonitoring();
    EventMonitor::UnloadResult stopMonitoring(const std::string &service);

private:
    void attachedDeviceStatusCallback(pbnjson::JValue &previousValue,
            pbnjson::JValue &value);
    void attachedStorageDevicesStatusCallback(pbnjson::JValue &previousValue,
            pbnjson::JValue &value);
    void attachedNonStorageDevicesCallback(pbnjson::JValue &previousValue,
            pbnjson::JValue &value);
    void blockToasts(unsigned int timeMs);
    void getDeviceTypeString(std::string &text, std::string &deviceType);
    void getToastText(std::string &text, std::string deviceType,
            std::string deviceStatus);
private:
    bool toastsBlocked;
    std::unordered_map<int, Device> mDevices; //deviceNum to device
};
