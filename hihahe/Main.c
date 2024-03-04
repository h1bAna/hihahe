#include<stdio.h>
#include<windows.h>
#include<signal.h>

int ABORT = 0;

void sig_handler(int signal)
{
	fprintf(stderr, "> exiting..\n");
	if (signal == SIGABRT || signal == SIGINT)
		ABORT = 1;
}

// window procedure
LRESULT CALLBACK wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	switch (message) {
	case WM_DESTROY:
		return 0;
	case WM_INPUT:
		char rid_buffer[64];
		UINT rid_size = sizeof(rid_buffer);

		if(GetRawInputData((HRAWINPUT)lparam, RID_INPUT, rid_buffer, &rid_size, sizeof(RAWINPUTHEADER))) {
			RAWINPUT* raw = (RAWINPUT*)rid_buffer;
			if (raw->header.dwType == RIM_TYPEKEYBOARD) {
				// print the key event
				printf("key event: %s, vkCode: %d, flags: %d\n", raw->data.keyboard.Message == WM_KEYDOWN ? "down" : "up", raw->data.keyboard.VKey, raw->data.keyboard.Flags);
			}
			// mouse event
			else if (raw->header.dwType == RIM_TYPEMOUSE){
				// Only log mouse button presses
				if (raw->data.mouse.usButtonFlags > 0) {
					printf("mouse event: button: %d, flags: %d\n", raw->data.mouse.usButtonData, raw->data.mouse.usButtonFlags);
				}
			}
		}
		break;
	}
	return DefWindowProc(window, message, wparam, lparam);
}


int main(int ac, char** av) {
	fprintf(stderr, "[+] start spy\n");

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