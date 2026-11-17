#include "gui_context.hpp"

#include "core.hpp"

#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

namespace rotm
{
	GuiContext::GuiContext(Core& coreRef) :
		core(coreRef),
		logger(core.logger)
	{
		logger.print_info()("Initializing GUI...");
		
		rlImGuiSetup(USE_DARK_THEME);

		io = &ImGui::GetIO();

		io->IniFilename = nullptr;	// disable ImGui persistence
		io->ConfigWindowsMoveFromTitleBarOnly = true;

		m_fonts =
		{
			.regular = io->Fonts->AddFontFromFileTTF("resources/iosevka.ttf", 21.5f),
			.regularSmall = io->Fonts->AddFontFromFileTTF("resources/iosevka.ttf", 16.0f),
			.monospace = io->Fonts->AddFontFromFileTTF("resources/consolas.ttf", 21.5f),
			.monospaceSmall = io->Fonts->AddFontFromFileTTF("resources/consolas.ttf", 16.0f)
		};
	}

	GuiContext::~GuiContext()
	{
		logger.print_info()("Shutting down GUI...");
		
		rlImGuiShutdown();
	}

	void GuiContext::begin_frame()
	{
		rlImGuiBeginDelta(static_cast<float>(core.get_delta_time()));
		ImGui::PushFont(m_fonts.regular);
	}

	void GuiContext::end_frame()
	{
		ImGui::PopFont();
		rlImGuiEnd();
	}
}
