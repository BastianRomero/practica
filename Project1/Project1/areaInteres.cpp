#include "framework.h"
#include "Project1.h"

#define CB_RESIZEX 1001
#define CB_RESIZEY 1002
#define STATIC_WINDOWS 1003

void areaInteres(HWND hWnd) {

    //int setHorizontal[20] = { 16,32,64,128,808 };
    //int setVertical[20] = { 8,32,64,128,616};

    wchar_t setHorizontal[5] = { 0 };
    wchar_t setVertical[5] = { 0 };


    unsigned long ve;
    unsigned long ho;

    int indexX = SendDlgItemMessage(hWnd, CB_RESIZEX, CB_GETCURSEL, 0, 0);
    int indexY = SendDlgItemMessage(hWnd, CB_RESIZEY, CB_GETCURSEL, 0, 0);

    SendDlgItemMessage(hWnd,CB_RESIZEX,CB_GETLBTEXT,indexX,(LPARAM)setHorizontal);
    SendDlgItemMessage(hWnd, CB_RESIZEY, CB_GETLBTEXT, indexY, (LPARAM)setVertical);

 
    wchar_t buf[100];
    swprintf_s(buf, L"%s x %s", setHorizontal, setVertical);

    SetDlgItemText(hWnd, STATIC_WINDOWS, buf);

    ho = _wtoi(setHorizontal);
    ve = _wtoi(setVertical);

    //EVT_SetCameraInt(&camera[cam],"Heigth",indeX);
    //EVT_SetCameraInt(&camera[cam],"Width",indeY);
}