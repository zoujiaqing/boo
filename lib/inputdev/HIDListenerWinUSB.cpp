#define _CRT_SECURE_NO_WARNINGS 1 /* STFU MSVC */
#include "boo/inputdev/IHIDListener.hpp"
#include "boo/inputdev/DeviceFinder.hpp"
#include <string.h>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>

#include <initguid.h>
#include <SetupAPI.h>
#include <Cfgmgr32.h>
#include <Usbiodef.h>
#include <Devpkey.h>

namespace boo
{

class HIDListenerWinUSB final : public IHIDListener
{
    DeviceFinder& m_finder;

    bool m_scanningEnabled;

    /*
     * Reference: https://github.com/pbatard/libwdi/blob/master/libwdi/libwdi.c
     */

    void _pollDevices(const char* pathFilter)
    {

        /* Don't ask */
        static const LPCSTR arPrefix[3] = {"VID_", "PID_", "MI_"};
        unsigned i, j;
        CONFIGRET r;
        ULONG devpropType;
        DWORD reg_type;
        HDEVINFO hDevInfo = 0;
        SP_DEVINFO_DATA DeviceInfoData = {0};
        DeviceInfoData.cbSize = sizeof(DeviceInfoData);
        SP_DEVICE_INTERFACE_DATA DeviceInterfaceData = {0};
        DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
        union {
            SP_DEVICE_INTERFACE_DETAIL_DATA_A wtf;
            CHAR alloc[2048];
        } DeviceInterfaceDetailData; /* Stack allocation should be fine for this */
        DeviceInterfaceDetailData.wtf.cbSize = sizeof(DeviceInterfaceDetailData);
        CHAR szDeviceInstanceID[MAX_DEVICE_ID_LEN];
        LPSTR pszToken, pszNextToken;
        CHAR szVid[MAX_DEVICE_ID_LEN], szPid[MAX_DEVICE_ID_LEN], szMi[MAX_DEVICE_ID_LEN];

        /* List all connected USB devices */
        hDevInfo = SetupDiGetClassDevs(NULL, 0, 0, DIGCF_PRESENT | DIGCF_ALLCLASSES | DIGCF_DEVICEINTERFACE);
        if (hDevInfo == INVALID_HANDLE_VALUE)
            return;

        for (i=0 ; ; ++i)
        {

            if (!SetupDiEnumDeviceInterfaces(hDevInfo,
                                             NULL,
                                             &GUID_DEVINTERFACE_USB_DEVICE,
                                             i,
                                             &DeviceInterfaceData))
                break;

            DeviceInterfaceDetailData.wtf.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            if (!SetupDiGetDeviceInterfaceDetailA(hDevInfo,
                                                  &DeviceInterfaceData,
                                                  &DeviceInterfaceDetailData.wtf,
                                                  sizeof(DeviceInterfaceDetailData),
                                                  NULL,
                                                  &DeviceInfoData))
                continue;

            r = CM_Get_Device_IDA(DeviceInfoData.DevInst, szDeviceInstanceID , MAX_PATH, 0);
            if (r != CR_SUCCESS)
                continue;

            /* Retreive the device description as reported by the device itself */
            pszToken = strtok_s(szDeviceInstanceID , "\\#&", &pszNextToken);
            szVid[0] = '\0';
            szPid[0] = '\0';
            szMi[0] = '\0';
            while (pszToken != NULL)
            {
                for (j=0 ; j<3 ; ++j)
                {
                    if (strncmp(pszToken, arPrefix[j], 4) == 0)
                    {
                        switch (j)
                        {
                            case 0:
                                strcpy_s(szVid, MAX_DEVICE_ID_LEN, pszToken);
                                break;
                            case 1:
                                strcpy_s(szPid, MAX_DEVICE_ID_LEN, pszToken);
                                break;
                            case 2:
                                strcpy_s(szMi, MAX_DEVICE_ID_LEN, pszToken);
                                break;
                            default:
                                break;
                        }
                    }
                }
                pszToken = strtok_s(NULL, "\\#&", &pszNextToken);
            }

            if (!szVid[0] || !szPid[0])
                continue;

            unsigned vid = strtol(szVid+4, NULL, 16);
            unsigned pid = strtol(szPid+4, NULL, 16);

            WCHAR productW[1024] = {0};
            CHAR product[1024] = {0};
            DWORD productSz = 0;
            if (!SetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_BusReportedDeviceDesc,
                                           &devpropType, (BYTE*)productW, 1024, &productSz, 0)) {
                /* fallback to SPDRP_DEVICEDESC (USB hubs still use it) */
                SetupDiGetDeviceRegistryPropertyA(hDevInfo, &DeviceInfoData, SPDRP_DEVICEDESC,
                                                  &reg_type, (BYTE*)productW, 1024, &productSz);
            }
            wcstombs(product, productW, productSz / 2);

            WCHAR manufW[1024] = L"Someone"; /* Windows Vista and earlier will use this as the vendor */
            CHAR manuf[1024] = {0};
            DWORD manufSz = 0;
            SetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_Manufacturer,
                                      &devpropType, (BYTE*)manufW, 1024, &manufSz, 0);
            wcstombs(manuf, manufW, manufSz / 2);

            /* Store as a shouting string (to keep hash-lookups consistent) */
            CharUpperA(DeviceInterfaceDetailData.wtf.DevicePath);

            /* Filter to specific device (provided by hotplug event) */
            if (pathFilter && strcmp(pathFilter, DeviceInterfaceDetailData.wtf.DevicePath))
                continue;

            /* Whew!! that's a single device enumerated!! */
            if (!m_finder._hasToken(DeviceInterfaceDetailData.wtf.DevicePath))
                m_finder._insertToken(DeviceToken(DeviceToken::DeviceType::USB,
                                                  vid, pid, manuf, product,
                                                  DeviceInterfaceDetailData.wtf.DevicePath));

        }

        SetupDiDestroyDeviceInfoList(hDevInfo);

    }

public:
    HIDListenerWinUSB(DeviceFinder& finder)
    : m_finder(finder)
    {
        /* Initial HID Device Add */
        _pollDevices(NULL);
    }

    ~HIDListenerWinUSB()
    {}

    /* Automatic device scanning */
    bool startScanning()
    {
        m_scanningEnabled = true;
        return true;
    }
    bool stopScanning()
    {
        m_scanningEnabled = false;
        return true;
    }

    /* Manual device scanning */
    bool scanNow()
    {
        _pollDevices(NULL);
        return true;
    }

    bool _extDevConnect(const char* path)
    {
        char upperPath[1024];
        strcpy_s(upperPath, 1024, path);
        CharUpperA(upperPath);
        if (m_scanningEnabled && !m_finder._hasToken(upperPath))
            _pollDevices(upperPath);
        return true;
    }

    bool _extDevDisconnect(const char* path)
    {
        char upperPath[1024];
        strcpy_s(upperPath, 1024, path);
        CharUpperA(upperPath);
        m_finder._removeToken(upperPath);
        return true;
    }

};

IHIDListener* IHIDListenerNew(DeviceFinder& finder)
{
    return new HIDListenerWinUSB(finder);
}

}
