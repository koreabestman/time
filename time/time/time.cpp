#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <fstream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define NTP_TIMESTAMP_DELTA 2208988800ull

const char* ntp_servers[] = {
    "129.6.15.28",
    "132.163.97.1",
    "216.239.35.0",
    "1.1.1.1"
};

void WriteLog(const char* message) {
    const size_t MAX_LOG_SIZE = 5 * 1024 * 1024; // 5MB

    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    *strrchr(path, '\\') = '\0';
    strcat_s(path, "\\timesync.log");

    // 파일 크기 확인
    DWORD fileSize = 0;
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        fileSize = GetFileSize(hFile, NULL);
        CloseHandle(hFile);
    }

    // 파일 열기 모드 결정
    std::ofstream log;
    if (fileSize > MAX_LOG_SIZE) {
        log.open(path, std::ios::out); // 덮어쓰기
    }
    else {
        log.open(path, std::ios::app); // 추가
    }

    if (log.is_open()) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        log << "[" << st.wYear << "-" << st.wMonth << "-" << st.wDay << " "
            << st.wHour << ":" << st.wMinute << ":" << st.wSecond << "] "
            << message << "\n";
        log.close();
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    const int max_rounds = 5;
    const int server_count = sizeof(ntp_servers) / sizeof(ntp_servers[0]);

    for (int round = 1; round <= max_rounds; ++round) {
        WriteLog(("count " + std::to_string(round) + " start").c_str());

        for (int i = 0; i < server_count; ++i) {
            const char* server_ip = ntp_servers[i];
            WriteLog(("try server: " + std::string(server_ip)).c_str());

            SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (s == INVALID_SOCKET) {
                WriteLog("socket failed");
                continue;
            }

            int timeout = 5000;
            setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

            struct sockaddr_in server = { 0 };
            server.sin_family = AF_INET;
            server.sin_port = htons(123);
            server.sin_addr.s_addr = inet_addr(server_ip);

            unsigned char packet[48] = { 0 };
            packet[0] = 0x1B;

            sendto(s, (char*)packet, 48, 0, (struct sockaddr*)&server, sizeof(server));

            int server_len = sizeof(server);
            int recv_len = recvfrom(s, (char*)packet, 48, 0, (struct sockaddr*)&server, &server_len);
            closesocket(s);

            if (recv_len < 0) {
                WriteLog(("no response : " + std::string(server_ip)).c_str());
                Sleep(1000);
                continue;
            }

            unsigned long long secs = ((unsigned long long)packet[40] << 24) |
                ((unsigned long long)packet[41] << 16) |
                ((unsigned long long)packet[42] << 8) |
                ((unsigned long long)packet[43]);

            time_t timestamp = (time_t)(secs - NTP_TIMESTAMP_DELTA);
            struct tm gmt = { 0 };
            gmtime_s(&gmt, &timestamp);

            SYSTEMTIME st;
            st.wYear = gmt.tm_year + 1900;
            st.wMonth = gmt.tm_mon + 1;
            st.wDay = gmt.tm_mday;
            st.wHour = gmt.tm_hour;
            st.wMinute = gmt.tm_min;
            st.wSecond = gmt.tm_sec;
            st.wMilliseconds = 0;

            if (SetSystemTime(&st)) {
                char success_msg[128];
                sprintf_s(success_msg, "time sucessed (%s): %04d-%02d-%02d %02d:%02d:%02d UTC",
                    server_ip, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
                WriteLog(success_msg);
                WSACleanup();
                return 0;
            }
            else {
                DWORD err = GetLastError();
                char fail_msg[128];
                sprintf_s(fail_msg, "system fail (%s): error code %ld", server_ip, err);
                WriteLog(fail_msg);
            }

            Sleep(1000);
        }
    }

    WriteLog("fail");
    WSACleanup();
    return 1;
}
