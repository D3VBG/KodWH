#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include "Offsets.h"

#define EP 0x000000
HBRUSH EnemyBrush = CreateSolidBrush(0x000000);

int screenX = GetSystemMetrics(SM_CXSCREEN);
int screenY = GetSystemMetrics(SM_CYSCREEN);


DWORD GPI(const wchar_t* pN)
{
    DWORD pd = 0;
    HANDLE hS = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hS != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 pE;
        pE.dwSize = sizeof(pE);

        if (Process32First(hS, &pE))
        {
            do
            {
                if (!_wcsicmp(pE.szExeFile, pN))
                {
                    pd = pE.th32ProcessID;
                    break;
                }
            } while (Process32Next(hS, &pE));

        }
    }
    CloseHandle(hS);
    return pd;
}

uintptr_t GetModuleBaseAddress(DWORD pd, const wchar_t* mN)
{
    uintptr_t mBA = 0;
    HANDLE hS = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pd);
    if (hS != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hS, &modEntry))
        {
            do
            {
                if (!_wcsicmp(modEntry.szModule, mN))
                {
                    mBA = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hS, &modEntry));
        }
    }
    CloseHandle(hS);
    return mBA;
}

uintptr_t moduleBase = GetModuleBaseAddress(GPI(L"csgo.exe"), L"client_panorama.dll");
HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, GPI(L"csgo.exe"));
HDC hdc = GetDC(FindWindowA(NULL, "Counter-Strike: Global Offensive"));

template<typename T> T RPM(SIZE_T address) {
    //The buffer for data that is going to be read from memory
    T b;

    //The actual RPM
    ReadProcessMemory(hProcess, (LPCVOID)address, &b, sizeof(T), NULL);

    //Return our buffer
    return b;
}

struct view_matrix_t {
    float* operator[ ](int index) {
        return matrix[index];
    }

    float matrix[4][4];
};

struct Vector3
{
    float x, y, z;
};

Vector3 WTS(const Vector3 pos, view_matrix_t matrix) {
    float _x = matrix[0][0] * pos.x + matrix[0][1] * pos.y + matrix[0][2] * pos.z + matrix[0][3];
    float _y = matrix[1][0] * pos.x + matrix[1][1] * pos.y + matrix[1][2] * pos.z + matrix[1][3];

    float w = matrix[3][0] * pos.x + matrix[3][1] * pos.y + matrix[3][2] * pos.z + matrix[3][3];

    float inv_w = 1.f / w;
    _x *= inv_w;
    _y *= inv_w;

    float x = screenX * .5f;
    float y = screenY * .5f;

    x += 0.5f * _x * screenX + 0.5f;
    y -= 0.5f * _y * screenY + 0.5f;

    return { x,y,w };
}

void DrawFilledRect(int x, int y, int w, int h)
{
    RECT rect = { x, y, x + w, y + h };
    FillRect(hdc, &rect, EnemyBrush);
}

void DrawBorderBox(int x, int y, int w, int h, int tn)
{
    DrawFilledRect(x, y, w, tn); //Top horiz line
    DrawFilledRect(x, y, tn, h); //Left vertical line
    DrawFilledRect((x + w), y, tn, h); //right vertical line
    DrawFilledRect(x, y + h, w + tn, tn); //bottom horiz line
}

void DrawLine(float StartX, float StartY, float EndX, float EndY)
{
    int a, b = 0;
    HPEN hO;
    HPEN hN = CreatePen(PS_SOLID, 2, EP);// penstyle, width, color
    hO = (HPEN)SelectObject(hdc, hN);
    MoveToEx(hdc, StartX, StartY, NULL); //start
    a = LineTo(hdc, EndX, EndY); //end
    DeleteObject(SelectObject(hdc, hO));
}

int main()
{
    SetConsoleTitle(L"KodWH For CSGO");
    std::cout << "Developed by D3VBG" << std::endl << "Real name: TARIK MUSTAFA" << std::endl << "https://kodbulgaria.com" << std::endl;
    std::cout << "Copyright 2020 KodBulgaria" << std::endl << "THIS PROGRAM IS FREE!";

    
    while (true)
    {
        view_matrix_t vmt = RPM<view_matrix_t>(moduleBase + dwViewMatrix);
        int localteam = RPM<int>(RPM<DWORD>(moduleBase + dwEntityList) + m_iTeamNum);

        for (int i = 1; i < 64; i++)
        {
            uintptr_t pEnt = RPM<DWORD>(moduleBase + dwEntityList + (i * 0x10));

            int h = RPM<int>(pEnt + m_iHealth);
            int t = RPM<int>(pEnt + m_iTeamNum);

            Vector3 p = RPM<Vector3>(pEnt + m_vecOrigin);
            Vector3 head;
            head.x = p.x;
            head.y = p.y;
            head.z = p.z + 75.f;
            Vector3 sp = WTS(p, vmt);
            Vector3 sh = WTS(head, vmt);
            float height = sh.y - sp.y;
            float width = height / 2.4f;

            if (sp.z >= 0.01f && t != localteam && h > 0 && h < 101) {
                DrawBorderBox(sp.x - (width / 2), sp.y, width, height, 1);
                DrawLine(screenX / 2, screenY, sp.x, sp.y);
            }
        }
    }
}