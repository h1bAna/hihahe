#include <Windows.h>
#include <WinUser.h>
#include <stdio.h>

BOOL CALLBACK Monitorenumproc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    // variables declaration
    int width = 0;
    int height = 0;
    HDC hdcMemDC = NULL;
    HBITMAP hbMonitor = NULL;
    BITMAP bmpMonitor = { 0 };
    HANDLE hDIB = NULL;
    char* lpbitmap = NULL;
    HANDLE hFile = NULL;
    wchar_t filename[100] = { 0 };
    DWORD dwSizeofDIB = 0;
    DWORD dwBytesWritten = 0;
    int x = 0;
    int y = 0;
	// Print the display resolution
	width = lprcMonitor->right - lprcMonitor->left;
    height = lprcMonitor->bottom - lprcMonitor->top;
    x = lprcMonitor->left;
    y = lprcMonitor->top;

    // create a compatible DC, which is used in a BitBlt from the window DC.
    hdcMemDC = CreateCompatibleDC(hdcMonitor);
    if (!hdcMemDC) {
        printf("[-] CreateCompatibleDC has failed\n");
        return -5;
    }

    // create a bitmap
    hbMonitor = CreateCompatibleBitmap(hdcMonitor, width, height);
    if (!hbMonitor) {
		printf("[-] CreateCompatibleBitmap Failed\n");
		return -6;
	}

    // Select the compatible bitmap into the compatible memory DC.
	SelectObject(hdcMemDC, hbMonitor);

	// Bit block transfer into our compatible memory DC.
	if (!BitBlt(hdcMemDC,
		0,0,
		width, height,
		hdcMonitor,
		x, y,
		SRCCOPY))
	{
		printf("[-] BitBlt has failed\n");
		return -7;
	}

    // Get the BITMAP from the HBITMAP.
    ZeroMemory(&bmpMonitor, sizeof(BITMAP));
    GetObject(hbMonitor, sizeof(BITMAP), &bmpMonitor);

    BITMAPFILEHEADER   bmfHeader;
    BITMAPINFOHEADER   bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmpMonitor.bmWidth;
    bi.biHeight = bmpMonitor.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((bmpMonitor.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpMonitor.bmHeight;

    // Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
    // call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
    // have greater overhead than HeapAlloc.

    hDIB = GlobalAlloc(GHND, dwBmpSize);
    lpbitmap = (char*)GlobalLock(hDIB);

    // Gets the "bits" from the bitmap, and copies them into a buffer 
    // that's pointed to by lpbitmap.
    GetDIBits(hdcMonitor, hbMonitor, 0,
        (UINT)bmpMonitor.bmHeight,
        lpbitmap,
        (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // A file is created, this is where we will save the screen capture.

    // time string with getTimeFormatEx with the format HH-mm-ss
    wchar_t time[100] = { 0 };
    GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, NULL, L"HH-mm-ss", time, 100);
    

    swprintf(filename, 100, L"screenshot\\%x-%s.bmp", (DWORD)hdcMonitor, time);
    
    hFile = CreateFile(filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);

    // Add the size of the headers to the size of the bitmap to get the total file size.
    dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // Offset to where the actual bitmap bits start.
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

    // Size of the file.
    bmfHeader.bfSize = dwSizeofDIB;

    // bfType must always be BM for Bitmaps.
    bmfHeader.bfType = 0x4D42; // BM.

    WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

    // Unlock and Free the DIB from the heap.
    GlobalUnlock(hDIB);
    GlobalFree(hDIB);

    // Close the handle for the file that was created.
    CloseHandle(hFile);

    // Clean up.

    DeleteObject(hbMonitor);
    DeleteObject(hdcMemDC);
    ReleaseDC(NULL, hdcMonitor);
    


    return TRUE;
}

int CaptureAnImage(HWND hWnd)
{
    HDC hdc = GetDC(NULL);
    if(!EnumDisplayMonitors(hdc, NULL, Monitorenumproc, 0))
	{
		printf("[-] EnumDisplayMonitors has failed\n");
		return -4;
	}
    ReleaseDC(NULL, hdc);
    return 0;
}
