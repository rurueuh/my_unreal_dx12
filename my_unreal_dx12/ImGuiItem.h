#pragma once

class ImGuiItem
{
public:
	virtual ~ImGuiItem() = default;
	virtual void DrawImGui() = 0;
};

