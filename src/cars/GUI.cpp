#include "GUI.h"

using namespace irr;
using namespace core;
using namespace gui;

const wchar_t *c_menu_strings[] = {	L"Start new game",
									L"Exit",
									L"Continue",
									L"Exit from game"};

const wchar_t *c_main_menu_label = L"Main menu";
const wchar_t *c_pause_menu_label = L"Pause menu";
const wchar_t *c_select_race_label = L"Select race";
const wchar_t *c_select_car_label = L"Select car";

const wchar_t *c_car_folder = L"./cars";
const wchar_t *c_race_folder = L"./races";

ECmdItem c_main_menu_items[] = { CMD_ID_START_BUTTON, 
							      CMD_ID_EXIT_BUTTON};

ECmdItem c_pause_menu_items[] = { CMD_ID_CONTINUE_BUTTON,
								   CMD_ID_START_BUTTON, 
								   CMD_ID_EXIT_MAIN_MENU_BUTTON,
								   CMD_ID_EXIT_BUTTON};

CGUI::CGUI(irr::IrrlichtDevice* device) 
	: m_guienv(device->getGUIEnvironment()), 
	  m_fs(device->getFileSystem())
{
	// place menu in the left top part of screen
	for (int i = 0; i < 4; i++)
		m_menu_buttons[i] = m_guienv->addButton(rect<s32>(MENU_ITEM_X_OFFSET, 50 + i * (20), 
														  MENU_ITEM_X_OFFSET + MENU_ITEM_WIDTH, 50 + i * (20) + 20)); 

	m_list_box = m_guienv->addListBox(rect<irr::s32>(MENU_ITEM_X_OFFSET, 50, 
													 MENU_ITEM_X_OFFSET + MENU_ITEM_WIDTH, 110));

	m_prev_button = m_guienv->addButton(rect<irr::s32>(10, 110, 110, 130), 0, 0, L"Prev"); 
	m_next_button = m_guienv->addButton(rect<irr::s32>(120, 110, 220, 130), 0, 0, L"Next"); 

	m_gui_label = m_guienv->addStaticText(L"", rect<irr::s32>(10, 10, 210, 40));

	m_fps_label = m_guienv->addStaticText(L"", rect<irr::s32>(300, 10, 350, 40));

	hideAll();
}

void CGUI::showMenu(ECmdItem *items, int count)
{
	int i;

	for (i=0;i < count; i++)
	{
		// show menu item
		m_menu_buttons[i]->setVisible(true);

		// set menu ID for event handling
		m_menu_buttons[i]->setID(items[i]);

		// set menu text
		m_menu_buttons[i]->setText(c_menu_strings[items[i]]);
		m_menu_buttons[i]->setToolTipText(c_menu_strings[items[i]]);
	}

	// hide other menu items
	for (; i < 4; i++)
		m_menu_buttons[i]->setVisible(false);
}

void CGUI::fillFileList(const irr::io::path &folder)
{
	// save current working directory
	io::path start_working_dir = m_fs->getWorkingDirectory();

	// get files from specific folder
	m_fs->changeWorkingDirectoryTo(folder);
	m_file_list = m_fs->createFileList();

	m_list_box->clear();

	core::stringw s;

	// skip .. and . path
	for (u32 i = 2; i < m_file_list->getFileCount(); i++)
	{
		if (!m_file_list->isDirectory(i))
		{
			#ifndef _IRR_WCHAR_FILESYSTEM
			const c8 *cs = (const c8 *)m_file_list->getFileName(i).c_str();
			wchar_t *ws = new wchar_t[strlen(cs) + 1];
			int len = mbstowcs(ws,cs,strlen(cs));
			ws[len] = 0;
			s = ws;
			delete [] ws;
			#else
			s = FileList->getFileName(i).c_str();
			#endif

			m_list_box->addItem(s.c_str());
		}
	}
	
	m_fs->changeWorkingDirectoryTo(start_working_dir);

	setBrowserVisible(true);
}

void CGUI::showMainMenu() {
	setBrowserVisible(false);
	showLabel(c_main_menu_label);
	showMenu(c_main_menu_items, 2);
}

void CGUI::showPauseMenu() {
	setBrowserVisible(false);
	showLabel(c_pause_menu_label);
	showMenu(c_pause_menu_items, 4);
}

//! show car file browser
void CGUI::showCarBrowser()
{
	hideMenu();
	showLabel(c_select_car_label);
	fillFileList(c_car_folder);

	m_prev_button->setText(L"Back");
	m_prev_button->setID(CMD_ID_CHOOSE_RACE_BUTTON);

	m_next_button->setText(L"Play");
	m_next_button->setID(CMD_ID_PLAY_BUTTON);	
}

//! show race file browser
void CGUI::showRaceBrowser()
{
	hideMenu();
	showLabel(c_select_race_label);
	fillFileList(c_race_folder);

	m_prev_button->setText(L"Main menu");
	m_prev_button->setID(CMD_ID_EXIT_MAIN_MENU_BUTTON);

	m_next_button->setText(L"Next");
	m_next_button->setID(CMD_ID_CHOOSE_CAR_BUTTON);
}