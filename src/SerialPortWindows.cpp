/*!
 * This file is part of SmartServoFramework.
 * Copyright (c) 2014, INRIA, All rights reserved.
 *
 * SmartServoFramework is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software. If not, see <http://www.gnu.org/licenses/lgpl-3.0.txt>.
 *
 * \file SerialPortWindows.cpp
 * \date 05/03/2014
 * \author Emeric Grange <emeric.grange@inria.fr>
 */

#if defined(_WIN32) || defined(_WIN64)

#include "SerialPortWindows.h"

// C++ standard library
#include <iostream>
#include <string>

LPCWSTR stringToLPCWSTR(const std::string &s)
{
    int slength = static_cast<int>(s.length()) + 1;
    int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t *buf = new wchar_t[len];

    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring temp(buf);
    delete[] buf;

    LPCWSTR l = temp.c_str();
    return l;
}

int serialPortsScanner(std::vector <std::string> &availableSerialPorts)
{
    int retcode = 0;
    std::string basePort = "\\\\.\\COM";

    std::cout << "serialPortsScanner() [Windows variant]" << std::endl;

    // Serial ports
    for (int i = 16; i > 0; i--)
    {
        std::string port = basePort + std::to_string(i);

        HANDLE ghSerial_Handle = CreateFile(stringToLPCWSTR(port), GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (ghSerial_Handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(ghSerial_Handle);

            std::cout << "- Scanning for serial port on '" << port << "' > FOUND" << std::endl;
            availableSerialPorts.push_back(port);
            retcode++;
        }
    }

    return retcode;
}

SerialPortWindows::SerialPortWindows(std::string &deviceName, const int baud, const int serialDevice, const int servoDevices):
    SerialPort(serialDevice, servoDevices),
    ttyDeviceFileDescriptor(0)
{
    if (deviceName.empty() == 1 || deviceName == "auto")
    {
        ttyDeviceName = autoselectSerialPort();
    }
    else
    {
        ttyDeviceName = deviceName;
    }

    if (ttyDeviceName != "null")
    {
        std::cout << "- Device port has been set to: '" << ttyDeviceName << "'" << std::endl;

        setBaudRate(baud);
        std::cout << "- Device baud rate has been set to: '" << ttyDeviceBaudRate << "'" << std::endl;
    }
}

SerialPortWindows::~SerialPortWindows()
{
    closeLink();
}

void SerialPortWindows::setBaudRate(const int baud)
{
    // Get valid baud rate
    ttyDeviceBaudRate = checkBaudRate(baud);

    // Compute the time needed to transfert one byte through the serial interface
    // (1000 / baudrate(= bit per msec)) * 10(= start bit + 8 data bit + stop bit)
    byteTransfertTime = (1000.0 / static_cast<double>(ttyDeviceBaudRate)) * 10.0;
}

int SerialPortWindows::openLink()
{
    DCB Dcb;
    COMMTIMEOUTS Timeouts;
    DWORD dwError;

    // Make sure no tty connection is already running
    closeLink();

    // Open tty device
    ttyDeviceFileDescriptor = CreateFile(strToLPCWSTR(ttyDeviceName), GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (ttyDeviceFileDescriptor == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Unable to open device: '" << ttyDeviceName << "'" << std::endl;
        goto OPEN_LINK_ERROR;
    }

    // Setting communication property
    Dcb.DCBlength = sizeof(DCB);
    if (GetCommState(ttyDeviceFileDescriptor, &Dcb) == FALSE)
    {
        std::cerr << "Unable to get communication state on '" << ttyDeviceName << "'" << std::endl;
        goto OPEN_LINK_ERROR;
    }

    // Set baudrate
    Dcb.BaudRate            = (DWORD)ttyDeviceBaudRate;
    Dcb.ByteSize            = 8;                    // Data bit = 8bit
    Dcb.Parity              = NOPARITY;             // No parity
    Dcb.StopBits            = ONESTOPBIT;           // Stop bit = 1
    Dcb.fParity             = NOPARITY;             // No Parity check
    Dcb.fBinary             = 1;                    // Binary mode
    Dcb.fNull               = 0;                    // Get Null byte
    Dcb.fAbortOnError       = 1;
    Dcb.fErrorChar          = 0;
    // Not using XOn/XOff
    Dcb.fOutX               = 0;
    Dcb.fInX                = 0;
    // Not using H/W flow control
    Dcb.fDtrControl         = DTR_CONTROL_DISABLE;
    Dcb.fRtsControl         = RTS_CONTROL_DISABLE;
    Dcb.fDsrSensitivity     = 0;
    Dcb.fOutxDsrFlow        = 0;
    Dcb.fOutxCtsFlow        = 0;

    if (SetCommState(ttyDeviceFileDescriptor, &Dcb) == FALSE)
    {
        std::cerr << "Unable to set communication state on '" << ttyDeviceName << "'" << std::endl;
        goto OPEN_LINK_ERROR;
    }
    if (SetCommMask(ttyDeviceFileDescriptor, 0) == FALSE) // Not using Comm event
    {
        std::cerr << "Unable to set communication mask on '" << ttyDeviceName << "'" << std::endl;
        goto OPEN_LINK_ERROR;
    }
    if (SetupComm(ttyDeviceFileDescriptor, 4096, 4096) == FALSE) // Buffer size (Rx,Tx)
    {
        std::cerr << "Unable to setup communication on '" << ttyDeviceName << "'" << std::endl;
        goto OPEN_LINK_ERROR;
    }
    if (PurgeComm(ttyDeviceFileDescriptor, PURGE_TXABORT|PURGE_TXCLEAR|PURGE_RXABORT|PURGE_RXCLEAR) == FALSE) // Clear buffer
    {
        std::cerr << "Unable to purge communication on '" << ttyDeviceName << "'" << std::endl;
        goto OPEN_LINK_ERROR;
    }
    if (ClearCommError(ttyDeviceFileDescriptor, &dwError, NULL) == FALSE)
    {
        std::cerr << "Unable to clear communication errors on '" << ttyDeviceName << "'" << std::endl;
        goto OPEN_LINK_ERROR;
    }
    if (GetCommTimeouts(ttyDeviceFileDescriptor, &Timeouts) == FALSE)
    {
        std::cerr << "Unable to get communication timeouts on '" << ttyDeviceName << "'" << std::endl;
        goto OPEN_LINK_ERROR;
    }

    // Timeout (Not using timeout)
    // Immediatly return
    Timeouts.ReadIntervalTimeout         = 0;
    Timeouts.ReadTotalTimeoutMultiplier  = 0;
    Timeouts.ReadTotalTimeoutConstant    = 1; // Must not be zero
    Timeouts.WriteTotalTimeoutMultiplier = 0;
    Timeouts.WriteTotalTimeoutConstant   = 0;

    if (SetCommTimeouts(ttyDeviceFileDescriptor, &Timeouts) == FALSE)
    {
        std::cerr << "Unable to set communication timeouts on '" << ttyDeviceName << "'" << std::endl;
        goto OPEN_LINK_ERROR;
    }

    return 1;

OPEN_LINK_ERROR:
    closeLink();
    return 0;
}

bool SerialPortWindows::isOpen()
{
    bool status = false;

    DCB Dcb;
    Dcb.DCBlength = sizeof(DCB);
    if (GetCommState(ttyDeviceFileDescriptor, &Dcb) == TRUE)
    {
        status = true;
    }

    return status;
}

void SerialPortWindows::closeLink()
{
    if (ttyDeviceFileDescriptor != INVALID_HANDLE_VALUE)
    {
        this->flush();
        CloseHandle(ttyDeviceFileDescriptor);
        ttyDeviceFileDescriptor = INVALID_HANDLE_VALUE;
    }
}

int SerialPortWindows::tx(unsigned char *packet, int packetLength)
{
    int status = -1;

    if (ttyDeviceFileDescriptor != INVALID_HANDLE_VALUE)
    {
        if (packet != NULL && packetLength > 0)
        {
            DWORD dwToWrite = (DWORD)packetLength;
            DWORD dwWritten = 0;

            if (WriteFile(ttyDeviceFileDescriptor, packet, dwToWrite, &dwWritten, NULL) == TRUE)
            {
                status = static_cast<int>(dwWritten);
            }
        }
        else
        {
            std::cerr << "Cannot write to serial port '" << ttyDeviceName << "': invalid packet buffer or size!" << std::endl;
        }
    }
    else
    {
        std::cerr << "Cannot write to serial port '" << ttyDeviceName << "': invalid device!" << std::endl;
    }

    return status;
}

int SerialPortWindows::rx(unsigned char *packet, int packetLength)
{
    int status = -1;

    if (ttyDeviceFileDescriptor != INVALID_HANDLE_VALUE)
    {
        if (packet != NULL && packetLength > 0)
        {
            DWORD dwToRead = (DWORD)packetLength;
            DWORD dwRead = 0;

            if (ReadFile(ttyDeviceFileDescriptor, packet, dwToRead, &dwRead, NULL) == TRUE)
            {
                status = static_cast<int>(dwRead);
            }
        }
        else
        {
            std::cerr << "Cannot read from serial port '" << ttyDeviceName << "': invalid packet buffer or size!" << std::endl;
        }
    }
    else
    {
        std::cerr << "Cannot read from serial port '" << ttyDeviceName << "': invalid device!" << std::endl;
    }

    return status;
}

void SerialPortWindows::flush()
{
    if (ttyDeviceFileDescriptor != INVALID_HANDLE_VALUE)
    {
        PurgeComm(ttyDeviceFileDescriptor, PURGE_RXABORT | PURGE_RXCLEAR);
    }
}

double SerialPortWindows::getTime()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);

    return static_cast<double>(time.QuadPart);
}

void SerialPortWindows::setTimeOut(int packetLength)
{
    packetStartTime = getTime();
    packetWaitTime  = (byteTransfertTime * static_cast<double>(packetLength) + 2.0 * static_cast<double>(latencyTime));
}

void SerialPortWindows::setTimeOut(double msec)
{
    packetStartTime = getTime();
    packetWaitTime  = msec;
}

int SerialPortWindows::checkTimeOut()
{
    LARGE_INTEGER end, freq;
    int status = 0;
    double time_elapsed = 0.0;

    QueryPerformanceCounter(&end);
    QueryPerformanceFrequency(&freq);

    time_elapsed = static_cast<double>(end.QuadPart - packetStartTime) / static_cast<double>(freq.QuadPart);
    time_elapsed *= 1000.0;

    if (time_elapsed > packetWaitTime)
    {
        status = 1;
    }
    else if (time_elapsed < 0)
    {
        packetStartTime = getTime();
    }

    return status;
}

#endif /* _WIN32 || _WIN64 */