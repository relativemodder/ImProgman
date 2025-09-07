#pragma once
#include <imgui.h>
#include <time.h>
#include "clockwidget.h"


void RenderClockWidget()
{
	static float winX = ImGui::GetIO().DisplaySize.x / 2.0f + 100.f;
	static float winY = ImGui::GetIO().DisplaySize.y / 2.0f - 350.f;

	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::SetNextWindowPos(ImVec2(winX, winY), ImGuiCond_Always);

	ImGui::Begin("Clock Widget", nullptr, 
		ImGuiWindowFlags_AlwaysAutoResize 
		| ImGuiWindowFlags_NoMove 
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoBackground
	);

	time_t now = time(0);
	struct tm tstruct;

	localtime_s(&tstruct, &now);

	char timebuf[80];
	strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &tstruct);

	ImGui::PushFont(g_FontLarge);
	ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(timebuf).x) * 0.5f);
	ImGui::Text("%s", timebuf);
	ImGui::PopFont();

	char datebuf[80];
	strftime(datebuf, sizeof(datebuf), "%A, %B %d, %Y", &tstruct);
	ImGui::PushFont(g_FontMedium);
	ImGui::Text("%s", datebuf);
	ImGui::PopFont();

	ImGui::End();
}