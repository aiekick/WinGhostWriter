#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

std::string GetLastErrorAsString() {
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) return "";

    LPSTR messageBuffer = nullptr;

    size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&messageBuffer, 0, NULL
    );

    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    return message;
}

std::string getPhysicalDriveFromLetter(const std::string& driveLetter) {
    std::string volumePath = "\\\\.\\" + driveLetter;
    if (volumePath.back() != ':') {
        volumePath += ':';
    }

    HANDLE hVolume = CreateFileA(
            volumePath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
    );

    if (hVolume == INVALID_HANDLE_VALUE) {
        std::cerr << "Err : cant open volume path " << volumePath << std::endl;
        return {};
    }

    VOLUME_DISK_EXTENTS extents;
    DWORD bytesReturned;

    if (!DeviceIoControl(
            hVolume,
            IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
            NULL,
            0,
            &extents,
            sizeof(extents),
            &bytesReturned,
            NULL)) {
        std::cerr << "Erreur : DeviceIoControl a échoué." << std::endl;
        CloseHandle(hVolume);
        return {};
    }

    CloseHandle(hVolume);

    int diskNumber = extents.Extents[0].DiskNumber;

    return "\\\\.\\PhysicalDrive" + std::to_string(diskNumber);
}

bool writeImageToDisk(const std::string& imagePath, const std::string& physicalPath) {
    std::ifstream image(imagePath, std::ios::binary);
    if (!image) {
        std::cerr << "Err : cant open source ghost image" << std::endl;
        return false;
    }

    HANDLE hDevice = CreateFileA(
            physicalPath.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        std::cerr << "Err : Cant open the device : " << physicalPath << std::endl;
        std::cerr << "Code : " << GetLastError() << " : " << GetLastErrorAsString() << std::endl;
        return false;
    }

    const size_t bufferSize = 1024 * 1024;
    std::vector<char> buffer(bufferSize);
    DWORD bytesWritten;

    while (image.read(buffer.data(), bufferSize) || image.gcount() > 0) {
        if (!WriteFile(hDevice, buffer.data(), static_cast<DWORD>(image.gcount()), &bytesWritten, NULL)) {
            std::cerr << "Writing error at offset " << image.tellg() << std::endl;
            CloseHandle(hDevice);
            return false;
        }
    }

    std::cout << "Ghost Writing succeeded " << physicalPath << std::endl;
    CloseHandle(hDevice);
    return true;
}

int main() {
    std::string driveLetter = "G";
    std::string imagePath = "../ghosts/ghost_usb_1.6go.ghost";

    std::string physicalPath = getPhysicalDriveFromLetter(driveLetter);
    if (physicalPath.empty()) {
        std::cerr << "Err : disk not found for the letter " << driveLetter << std::endl;
        return 1;
    }

    if (!writeImageToDisk(imagePath, physicalPath)) {
        std::cerr << "Err : ghost writing failed." << std::endl;
        return 2;
    }

    return 0;
}

