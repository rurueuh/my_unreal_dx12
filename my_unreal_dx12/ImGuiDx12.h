#pragma once

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "ImGuiLayer.h"
#include "Renderer.h"
#include "ImGuiItem.h"
#include <vector>
#include <functional>
#include <iostream>
#include <algorithm>

class TextItem : public ImGuiItem {
public:
	TextItem(const char* fmt, ...) {
		va_list args;
		va_start(args, fmt);
		vsnprintf_s(buffer, sizeof(buffer), fmt, args);
		va_end(args);
	}
	void DrawImGui() override {
		ImGui::TextUnformatted(buffer);
	}

	void setText(const char* fmt, ...) {
		va_list args;
		va_start(args, fmt);
		vsnprintf_s(buffer, sizeof(buffer), fmt, args);
		va_end(args);
	}
private:
	char buffer[256];
};

class ButtonItem : public ImGuiItem {
public:
	ButtonItem(const char* lbl, std::function<void()> cb) : label(lbl), callback(cb) {}
	void DrawImGui() override {
		if (ImGui::Button(label)) {
			callback();
		}
	}
private:
	const char* label;
	std::function<void()> callback;
};

class SeparatorItem : public ImGuiItem {
public:
	void DrawImGui() override {
		ImGui::Separator();
	}
};

class FPSItem : public ImGuiItem {
public:
	void DrawImGui() override {
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	}
};

class SliderFloatItem : public ImGuiItem {
public:
	SliderFloatItem(const char* lbl, float* val, float mn, float mx, std::function<void(float)> o)
		: label(lbl), value(val), onChange(std::move(o)) {
		min = mn; max = mx;
		if (!value) {
			value = &dummy;
		}
	}
	void DrawImGui() override {
		if (ImGui::SliderFloat(label, value, min, max)) {
			if (onChange) onChange(*value);
		}
	}

private:
	const char* label;
	std::function<void(float)> onChange;
	float dummy = 0.0f;
	float* value;
	float min;
	float max;
};

class ImGuiDx12
{
public:
	void Draw(Renderer& );
	void Init(HWND hwnd, GraphicsDevice& gd, SwapChain& sc) {
		m_imgui.Init(hwnd, gd, sc);
	}
	void NewFrame() {
		m_imgui.NewFrame();
	}

	std::shared_ptr<ButtonItem> AddButton(const char* label, std::function<void()> onClick) {
		
		auto t = AddItem<ButtonItem>(label, onClick);
		return t;
	}

	std::shared_ptr<SeparatorItem> addSeparator() {
		
		auto t = AddItem<SeparatorItem>();
		return t;
	}

	std::shared_ptr<TextItem> addText(const char* fmt, ...) {
		va_list args;
		va_start(args, fmt);
		auto t = AddItem<TextItem>(fmt, args);
		va_end(args);
		return t;
	}

	std::shared_ptr<FPSItem> addFPS() {
		
		auto t = AddItem<FPSItem>();
		return t;
	}

	std::shared_ptr<SliderFloatItem> addSliderFloat(const char* label, float* value, float min, float max, std::function<void(float)> onChange = nullptr) {
		
		auto t = AddItem<SliderFloatItem>(label, value, min, max, onChange);
		return t;
	}

	template<typename T, typename... Args>
	std::shared_ptr<T> AddItem(Args&&... args) {
		assert((std::is_base_of_v<ImGuiItem, T>));
		auto t = std::make_shared<T>(std::forward<Args>(args)...);
		m_items.push_back(t);
		return t;
	}
private:
	ImGuiLayer m_imgui;

	std::vector<std::shared_ptr<ImGuiItem>> m_items;
};

