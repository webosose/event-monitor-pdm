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

#include "PdmUtils.h"

#include <event-monitor-api/pluginbase.hpp>

#include <sys/shm.h>

#include <set>
#include <map>
#include <unordered_map>

#define PDM_SHM_KEY 45697

using namespace PdmUtils;

class PdmPlugin: public EventMonitor::PluginBase {
public:
    PdmPlugin(EventMonitor::Manager *manager);
    virtual ~PdmPlugin();
    void startMonitoring();
    EventMonitor::UnloadResult stopMonitoring(const std::string &service);

private:
    void attachedStorageDeviceListCallback(pbnjson::JValue &previousValue,
            pbnjson::JValue &value);
    void attachedNonStorageDeviceListCallback(pbnjson::JValue &previousValue,
            pbnjson::JValue &value);
    void blockToasts(unsigned int timeMs);
    void handleEvent(Event event);
    void saveAlreadyConnectedDeviceList(pbnjson::JValue &previousValue,
            pbnjson::JValue &value, EventType);
    void processNewEntries(std::set<int> &deviceNums,
            std::unordered_multimap<int, Device> &newDevices,
            std::unordered_map<int, Device> &mCurrentDevices);
    std::string getDeviceType(std::string current, std::string received);
    static void signalHandler(int signum, siginfo_t *sig_info, void *ucontext);
    void handlePdmEvent(std::string payload);
    void createAlertForMaxUsbStorageDevices();
    void unMountMtpDeviceAlert(std::string driveName);
    void createAlertForUnmountedDeviceRemoval(std::string deviceNumber);
    void createAlertForUnsupportedFileSystem(std::string deviceNumber);
    void closeUnsupportedFsAlert(std::string deviceNumber);
    void createAlertForFsckTimeout(std::string deviceNumber,
            std::string deviceName);
    void showConnectingToast(int deviceType);
    void showFormatStartedToast(std::string driveInfo);
    void showFormatSuccessToast(std::string driveInfo);
    void showFormatFailToast(std::string driveInfo);
private:
    bool toastsBlocked;
    std::unordered_map<int, Device> mStorageDevices;
    std::unordered_map<int, Device> mNonStorageDevices;
    std::unordered_multimap<int, Device> mNewDevices;
};
