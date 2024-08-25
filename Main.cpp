/*
	Triggerbot Mitigations
	Copyright (C) 2024 Unstoppable

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.
*/
#include "General.h"
#include "Main.h"
#include "engine_io.h"
#include "gmgame.h"
#include "HashTemplateIterator.h"
#include "RawFileClass.h"

// These are the HUD/reticle colors from an original objects.ddb.
#define COLOR_ENEMY .78431373f, .0f, .0f
#define COLOR_FRIENDLY .0f, .88235294f, .0f
#define COLOR_NEUTRAL .49019607f, .58823529f, .49019607f

TriggerbotMitigationsPlugin::TriggerbotMitigationsPlugin() : Level(MITIGATION_OFF), MinColorThreshold(20.f), MaxColorThreshold(80.f) {
	RegisterEvent(EVENT_MAP_INI,this);
	RegisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
	RegisterEvent(EVENT_PLAYER_JOIN_HOOK,this);
	RegisterEvent(EVENT_THINK_HOOK,this);

	ConsoleFunctionList.Add(new AddMitigationExceptionConsoleFunctionClass);
	ConsoleFunctionList.Add(new RemoveMitigationExceptionConsoleFunctionClass);
	ConsoleFunctionList.Add(new PrintMitigationExceptionsConsoleFunctionClass);
	ConsoleFunctionList.Add(new FlushMitigationExceptionsConsoleFunctionClass);
	Sort_Function_List();
	Verbose_Help_File();
}

TriggerbotMitigationsPlugin::~TriggerbotMitigationsPlugin() {
	UnregisterEvent(EVENT_MAP_INI,this);
	UnregisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
	UnregisterEvent(EVENT_PLAYER_JOIN_HOOK,this);
	UnregisterEvent(EVENT_THINK_HOOK,this);

	Delete_Console_Function("addmitigationexception");
	Delete_Console_Function("removemitigationexception");
	Delete_Console_Function("printmitigationexceptions");
	Delete_Console_Function("flushmitigationexceptions");
}

void TriggerbotMitigationsPlugin::OnLoadMapINISettings(INIClass *SSGMIni) {
	StringClass mitigationLevel;
	SSGMIni->Get_String(mitigationLevel, "General", "MitigationLevel", "Off");
	if (!mitigationLevel.Compare_No_Case("Aggressive")) {
		Level = MITIGATION_AGGRESSIVE;
	}
	else if (!mitigationLevel.Compare_No_Case("High")) {
		Level = MITIGATION_HIGH;
	}
	else if (!mitigationLevel.Compare_No_Case("Medium")) {
		Level = MITIGATION_MEDIUM;
	}
	else if (!mitigationLevel.Compare_No_Case("Low")) {
		Level = MITIGATION_LOW;
	}
	else if (!mitigationLevel.Compare_No_Case("Passive")) {
		Level = MITIGATION_PASSIVE;
	}
	else if (!mitigationLevel.Compare_No_Case("Off")) {
		Level = MITIGATION_OFF;
	}
	else {
		Console_Output("[ERROR] Invalid mitigation level: %s\n", mitigationLevel);
	}

	StringClass colorThreshold;
	SSGMIni->Get_String(colorThreshold, "General", "ColorThreshold", "20-50");
	const char* first = strtok(colorThreshold.Peek_Buffer(), "-");
	const char* second = strtok(NULL, "-");
	MinColorThreshold = (float)atof(first);
	MaxColorThreshold = (float)atof(second);

	if (INIClass* exceptionsINI = Get_INI("MitigationExceptions.ini")) {
		if (INISection* section = exceptionsINI->Find_Section("Exceptions")) {
			for(INIEntry* entry = section->EntryList.First(); entry && entry->Is_Valid(); entry = entry->Next()) {
				MitigationExceptions.Insert(entry->Entry, (MitigationLevel)atoi(entry->Value));
			}
		}
		Release_INI(exceptionsINI);
	}
}

void TriggerbotMitigationsPlugin::OnFreeMapData() {
	Level = MITIGATION_OFF;
	MinColorThreshold = 20.f;
	MaxColorThreshold = 80.f;

	Flush_Mitigation_Exceptions();
}

void TriggerbotMitigationsPlugin::OnLoadLevel() {
	LevelLoaded = true;
	for (SLNode<cPlayer>* p = Get_Player_List()->Head(); p; p = p->Next()) {
		if (p->Data()->Is_Active()) { // Don't add inactive (not connected) players.
			WaitingPlayers.Add_Tail(p->Data());
		}
	}
}

void TriggerbotMitigationsPlugin::OnPlayerJoin(int PlayerID,const char *PlayerName) {
	if (cPlayer* Player = Find_Player(PlayerID))
		WaitingPlayers.Add_Tail(Player);
}

void TriggerbotMitigationsPlugin::OnThink() {
	SLNode<cPlayer>* p = WaitingPlayers.Head();
	while (p) {
		bool restart = false;

		if (!p->Data()->Is_Active()) { // Remove people who left the game before joining.
			WaitingPlayers.Remove(p->Data());
			restart = true;
		}
		else if (p->Data()->Get_Is_In_Game() && p->Data()->Get_GameObj()) {
			Apply_Mitigation(p->Data()->Get_Id());
			WaitingPlayers.Remove(p->Data());
			restart = true;
		}

		p = restart ? WaitingPlayers.Head() : p->Next();
	}
}

void TriggerbotMitigationsPlugin::Add_Mitigation_Exception(const char* PlayerName, MitigationLevel Level) {
	if (!PlayerName) {
		return;
	}
	if (MitigationLevel* level = MitigationExceptions.Get(PlayerName)) {
		*level = Level;
	}
	else {
		MitigationExceptions.Insert(PlayerName, Level);
	}
	if (int id = Get_Player_ID_By_Name(PlayerName)) {
		Reset_Mitigations(id);
		Apply_Mitigation(id);
	}
}

void TriggerbotMitigationsPlugin::Remove_Mitigation_Exception(const char* PlayerName) {
	if (!PlayerName) {
		return;
	}
	MitigationExceptions.Remove(PlayerName);
	if (int id = Get_Player_ID_By_Name(PlayerName)) {
		Reset_Mitigations(id);
		Apply_Mitigation(id);
	}
}

bool TriggerbotMitigationsPlugin::Flush_Mitigation_Exceptions() {
	RawFileClass file("MitigationExceptions.ini");
	INIClass exceptionsINI(file);
	if (file.Open(2)) {
		for (HashTemplateIterator<StringClass, MitigationLevel> it(MitigationExceptions); it; ++it) {
			exceptionsINI.Put_Int("Exceptions", it.getKey(), it.getValue(), 0);
		}
		exceptionsINI.Save(file);
		return true;
	}
	else {
		return false;
	}
}

void TriggerbotMitigationsPlugin::Reset_Mitigations(int PlayerID) {
	GameObject* obj = Get_GameObj(PlayerID);
	Enable_Global_Targeting_Player(obj, true);
	Change_Friendly_HUD_Color_Player(obj, Vector3(COLOR_FRIENDLY));
	Change_Enemy_HUD_Color_Player(obj, Vector3(COLOR_ENEMY));
	Change_Neutral_HUD_Color_Player(obj, Vector3(COLOR_NEUTRAL));
}

void TriggerbotMitigationsPlugin::Apply_Mitigation(int PlayerID) {
	MitigationLevel level = Level;

	const char* playerName = Get_Player_Name_By_ID(PlayerID);
	if (MitigationLevel* excLevel = MitigationExceptions.Get(playerName)) {
		level = *excLevel;
	}
	delete[] playerName;

	switch(level) {
		case MITIGATION_AGGRESSIVE: { // Disable global targeting
			Enable_Global_Targeting_Player(Get_GameObj(PlayerID), false);
			break;
		}
		case MITIGATION_HIGH: { // Replace Friendly and Neutral color with Enemy color
			Change_Friendly_HUD_Color_Player(Get_GameObj(PlayerID), Vector3(COLOR_ENEMY));
			Change_Neutral_HUD_Color_Player(Get_GameObj(PlayerID), Vector3(COLOR_ENEMY));
			break;
		}
		case MITIGATION_MEDIUM: { // Replace Enemy and Friendly color with Neutral color
			Change_Friendly_HUD_Color_Player(Get_GameObj(PlayerID), Vector3(COLOR_NEUTRAL));
			Change_Enemy_HUD_Color_Player(Get_GameObj(PlayerID), Vector3(COLOR_NEUTRAL));
			break;
		}
		case MITIGATION_LOW: { // Replace Enemy color with Friendly color
			Change_Enemy_HUD_Color_Player(Get_GameObj(PlayerID), Vector3(COLOR_FRIENDLY));
			break;
		}
		case MITIGATION_PASSIVE: { // Slightly modify Enemy color per-player
			Vector3 origColor(COLOR_ENEMY);
			Vector3 newColor(COLOR_ENEMY);
			float r = (Commands->Get_Random(MinColorThreshold, MaxColorThreshold) * (Commands->Get_Random_Int(0, 100) % 2 ? -1 : 1)) / 255.f;
			while (origColor.X + r < 0 || origColor.X + r > 255) {
				r = (Commands->Get_Random(MinColorThreshold, MaxColorThreshold) * (Commands->Get_Random_Int(0, 100) % 2 ? -1 : 1)) / 255.f;
			}
			float g = (Commands->Get_Random(MinColorThreshold, MaxColorThreshold) * (Commands->Get_Random_Int(0, 100) % 2 ? -1 : 1)) / 255.f;
			while (origColor.X + g < 0 || origColor.X + g > 255) {
				g = (Commands->Get_Random(MinColorThreshold, MaxColorThreshold) * (Commands->Get_Random_Int(0, 100) % 2 ? -1 : 1)) / 255.f;
			}
			float b = (Commands->Get_Random(MinColorThreshold, MaxColorThreshold) * (Commands->Get_Random_Int(0, 100) % 2 ? -1 : 1)) / 255.f;
			while (origColor.X + b < 0 || origColor.X + b > 255) {
				b = (Commands->Get_Random(MinColorThreshold, MaxColorThreshold) * (Commands->Get_Random_Int(0, 100) % 2 ? -1 : 1)) / 255.f;
			}
			newColor.X += WWMath::Fabs(r);
			newColor.Y += WWMath::Fabs(g);
			newColor.Z += WWMath::Fabs(b);
			Change_Enemy_HUD_Color_Player(Get_GameObj(PlayerID), newColor);
			break;
		}
		case MITIGATION_OFF: { // Nothing
			break;
		}
	}
}

TriggerbotMitigationsPlugin plugin;
extern "C" __declspec(dllexport) Plugin * Plugin_Init()
{
	return &plugin;
}

/***************************************************************************/

void AddMitigationExceptionConsoleFunctionClass::Activate(const char* pArgs) {
	if (!pArgs)
	{
		return;
	}

	const char* pNameBuf = pArgs;
	pArgs = strchr(pArgs, ' ');
	if (!pArgs)
	{
		return;
	}

	char* pName = new char[pArgs - pNameBuf + 1];
	memcpy(pName, pNameBuf, pArgs - pNameBuf);
	pName[pArgs - pNameBuf] = 0;

	if (!(++pArgs))
	{
		return;
	}

	long mLvl = atoi(pArgs);

	if (mLvl >= MITIGATION_MAX || mLvl < 0) {
		Console_Output("Specified level value is invalid. Enter between 0 - 5.\n");
		return;
	}

	plugin.Add_Mitigation_Exception(pName, (MitigationLevel)mLvl);
	Console_Output("%s has been added to the exception list.\n", pName);
	delete[] pName;
}

/***************************************************************************/

void RemoveMitigationExceptionConsoleFunctionClass::Activate(const char* pArgs) {
	if (!pArgs) {
		return;
	}

	plugin.Remove_Mitigation_Exception(pArgs);
	Console_Output("%s has been removed from the exception list.\n", pArgs);
}

/***************************************************************************/

void PrintMitigationExceptionsConsoleFunctionClass::Activate(const char* pArgs) {
	Console_Output("Begin Mitigation Exceptions:\n");
	for (HashTemplateIterator<StringClass, MitigationLevel> it(plugin.Get_Mitigation_Exceptions()); it; ++it) {
		Console_Output("%s: %d\n", it.getKey(), it.getValue());
	}
	Console_Output("End of Mitigation Exceptions\n");
}

/***************************************************************************/

void FlushMitigationExceptionsConsoleFunctionClass::Activate(const char* pArgs) {
	if (plugin.Flush_Mitigation_Exceptions())
		Console_Output("Mitigation exceptions have been written to the file.\n");
	else
		Console_Output("Mitigation exceptions could not be flushed.\n");
}

/***************************************************************************/
