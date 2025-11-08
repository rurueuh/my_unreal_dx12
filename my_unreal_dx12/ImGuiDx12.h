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

	void AddButton(const char* label, std::function<void()> onClick) {
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
		AddItem<ButtonItem>(label, onClick);
	}

	void addSeparator() {
		class SeparatorItem : public ImGuiItem {
		public:
			void DrawImGui() override {
				ImGui::Separator();
			}
		};
		AddItem<SeparatorItem>();
	}

	void addText(const char* fmt, ...) {
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
		private:
			char buffer[256];
		};
		va_list args;
		va_start(args, fmt);
		AddItem<TextItem>(fmt, args);
		va_end(args);
	}

	void addFPS() {
		class FPSItem : public ImGuiItem {
		public:
			void DrawImGui() override {
				ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
			}
		};
		AddItem<FPSItem>();
	}

	void addSliderFloat(const char* label, float* value, float min, float max, std::function<void(float)> onChange = nullptr) {
		class SliderFloatItem : public ImGuiItem {
		public:
			SliderFloatItem(const char* lbl, float* val, float mn, float mx, std::function<void(float)> o)
				: label(lbl), value(val), onChange(std::move(o)) {
				min = mn; max = mx;
			}
			void DrawImGui() override {
				if (ImGui::SliderFloat(label, value, min, max)) {
					if (onChange) onChange(*value);
				}
			}

		private:
			const char* label;
			std::function<void(float)> onChange;
			float* value;
			float min;
			float max;
		};
		AddItem<SliderFloatItem>(label, value, min, max, onChange);
	}

	template<typename T, typename... Args>
	void AddItem(Args&&... args) {
		assert((std::is_base_of_v<ImGuiItem, T>));
		m_items.push_back(std::make_shared<T>(std::forward<Args>(args)...));
	}
private:
	ImGuiLayer m_imgui;

	std::vector<std::shared_ptr<ImGuiItem>> m_items;
};

