#pragma once
#include <windows.h>
#include <string>


class Window
{
public:
	Window() = default;


	bool Create(const std::wstring& title, UINT width, UINT height)
	{
		m_width = width;
		m_height = height;


		WNDCLASSEX wc{};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.lpfnWndProc = &Window::WndProcStatic;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.lpszClassName = L"DX12ReadableWindow";
		if (!RegisterClassEx(&wc)) return false;


		RECT rc{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);


		m_hwnd = CreateWindow(
			wc.lpszClassName,
			title.c_str(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			rc.right - rc.left,
			rc.bottom - rc.top,
			nullptr, nullptr, wc.hInstance, this);


		if (!m_hwnd) return false;
		ShowWindow(m_hwnd, SW_SHOW);
		UpdateWindow(m_hwnd);
		return true;
	}

	void SetTitle(const std::wstring& title)
	{
		SetWindowTextW(m_hwnd, title.c_str());
	}


	bool PumpMessages()
	{
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) return false;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return true;
	}


	HWND GetHwnd() const { return m_hwnd; }
	UINT GetWidth() const { return m_width; }
	UINT GetHeight()const { return m_height; }


	bool WasResized()
	{
		bool r = m_resized; m_resized = false; return r;
	}


private:
	static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
	{
		if (msg == WM_NCCREATE)
		{
			auto cs = reinterpret_cast<CREATESTRUCT*>(lp);
			auto self = static_cast<Window*>(cs->lpCreateParams);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
		}
		Window* self = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (self) return self->WndProc(hWnd, msg, wp, lp);
		return DefWindowProc(hWnd, msg, wp, lp);
	}


	LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
	{
		switch (msg)
		{
		case WM_SIZE:
			m_width = LOWORD(lp);
			m_height = HIWORD(lp);
			m_resized = true;
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		default:
			return DefWindowProc(hWnd, msg, wp, lp);
		}
	}


private:
	HWND m_hwnd = nullptr;
	UINT m_width = 0, m_height = 0;
	bool m_resized = false;
};