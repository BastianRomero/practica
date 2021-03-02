#include "framework.h"
#include "Project1.h"


#define BTN_ENVIAR 1000
#define CM_RESIZEX 1001
#define CM_RESIZEY 1002
#define STATIC_WINDOWS 1003

void STATIC(HWND hWnd, HINSTANCE instancia);

void windosCreates(HWND hWnd, LPARAM lParam, WPARAM wParam) {

    static HINSTANCE instancia;
    instancia = ((LPCREATESTRUCT)lParam)->hInstance;

    STATIC(hWnd,instancia);

    CreateWindowEx(0, _T("BUTTON"), _T("Enviar"), BS_PUSHBUTTON | BS_CENTER | WS_CHILD | WS_VISIBLE,
        10, 40, 250, 25, hWnd, (HMENU)BTN_ENVIAR, instancia, NULL);

    CreateWindowEx(0, _T("COMBOBOX"), _T(""), CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE,
        10, 120, 100, 120, hWnd, (HMENU)CM_RESIZEX, instancia, NULL);

    CreateWindowEx(0, _T("COMBOBOX"), _T(""), CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE,
        120, 120, 100, 120, hWnd, (HMENU)CM_RESIZEY, instancia, NULL);


    const wchar_t* horizontal[] = { L"808",L"32",L"64",L"128",L"16" };
    const wchar_t* vertical[] = { L"616", L"16", L"24", L"32",L"8" };

    int largo = sizeof(vertical) / sizeof(wchar_t*);

    for (int i = 0; i < largo; i++)
    {
        SendDlgItemMessage(hWnd, CM_RESIZEX, CB_ADDSTRING, 0, (LPARAM)horizontal[i]);
        SendDlgItemMessage(hWnd, CM_RESIZEX, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
        SendDlgItemMessage(hWnd, CM_RESIZEY, CB_ADDSTRING, 0, (LPARAM)vertical[i]);
        SendDlgItemMessage(hWnd, CM_RESIZEY, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
    }

}

void STATIC(HWND hWnd, HINSTANCE instancia) {

    CreateWindowEx(0, _T("STATIC"), _T("Inicio programa"), BS_PUSHBUTTON | BS_CENTER | WS_CHILD | WS_VISIBLE,
        10, 10, 250, 25, hWnd, (HMENU)-1, instancia, NULL);
    CreateWindowEx(0, _T("STATIC"), _T("Horizontal"), BS_PUSHBUTTON | BS_CENTER | WS_CHILD | WS_VISIBLE,
        10, 90, 100, 25, hWnd, (HMENU)-1, instancia, NULL);
    CreateWindowEx(0, _T("STATIC"), _T("Vertical"), BS_PUSHBUTTON | BS_CENTER | WS_CHILD | WS_VISIBLE,
        120, 90, 100, 25, hWnd, (HMENU)-1, instancia, NULL);
    CreateWindowEx(0, _T("STATIC"), _T("---"), BS_PUSHBUTTON | BS_CENTER | WS_CHILD | WS_VISIBLE,
        10, 300, 200, 25, hWnd, (HMENU)STATIC_WINDOWS, instancia, NULL);
}