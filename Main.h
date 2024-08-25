/*
	Triggerbot Mitigations
	Copyright (C) 2024 Unstoppable

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.
*/
#pragma once

#include "gmplugin.h"

enum MitigationLevel {
	MITIGATION_OFF,
	MITIGATION_PASSIVE,
	MITIGATION_LOW,
	MITIGATION_MEDIUM,
	MITIGATION_HIGH,
	MITIGATION_AGGRESSIVE,
	MITIGATION_MAX   // To specify the limit.
};

class TriggerbotMitigationsPlugin : public Plugin {
public:
	TriggerbotMitigationsPlugin();
	~TriggerbotMitigationsPlugin();
	
	void OnLoadMapINISettings(INIClass *SSGMIni) override;
	void OnFreeMapData() override;
	void OnLoadLevel() override;
	void OnPlayerJoin(int PlayerID,const char *PlayerName) override;
	void OnThink() override;

	void Add_Mitigation_Exception(const char* PlayerName, MitigationLevel Level);
	void Remove_Mitigation_Exception(const char* PlayerName);
	bool Flush_Mitigation_Exceptions();
	void Reset_Mitigations(int PlayerID);
	void Apply_Mitigation(int PlayerID);

	HashTemplateClass<StringClass, MitigationLevel>& Get_Mitigation_Exceptions() { return MitigationExceptions; }

private:
	// Configuration
	MitigationLevel Level;
	float MinColorThreshold;
	float MaxColorThreshold;
	HashTemplateClass<StringClass, MitigationLevel> MitigationExceptions;

	// Runtime
	SList<cPlayer> WaitingPlayers;
	bool LevelLoaded;
};

class AddMitigationExceptionConsoleFunctionClass : public ConsoleFunctionClass {
public:
	const char* Get_Name() override { return "addmitigationexception"; }
	const char* Get_Alias() override { return "ame"; }
	const char* Get_Help() override { return "ADDMITIGATIONEXCEPTION <name> <level> - Adds a custom mitigation exception for specified player."; }
	void Activate(const char* pArgs) override;
};

class RemoveMitigationExceptionConsoleFunctionClass : public ConsoleFunctionClass {
public:
	const char* Get_Name() override { return "removemitigationexception"; }
	const char* Get_Alias() override { return "rme"; }
	const char* Get_Help() override { return "REMOVEMITIGATIONEXCEPTION <name> - Removes the custom mitigation exception for specified player."; }
	void Activate(const char* pArgs) override;
};

class PrintMitigationExceptionsConsoleFunctionClass : public ConsoleFunctionClass {
public:
	const char* Get_Name() override { return "printmitigationexceptions"; }
	const char* Get_Alias() override { return "pme"; }
	const char* Get_Help() override { return "PRINTMITIGATIONEXCEPTIONS - Prints all the mitigation exceptions in the memory."; }
	void Activate(const char* pArgs) override;
};

class FlushMitigationExceptionsConsoleFunctionClass : public ConsoleFunctionClass {
public:
	const char* Get_Name() override { return "flushmitigationexceptions"; }
	const char* Get_Alias() override { return "fme"; }
	const char* Get_Help() override { return "FLUSHMITIGATIONEXCEPTIONS - Saves the changes in the mitigation exceptions to the file."; }
	void Activate(const char* pArgs) override;
};
