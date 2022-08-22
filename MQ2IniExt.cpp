#include <mq/Plugin.h>

#include <string>
#include <map>
using namespace std;

#define SI_CONVERT_WIN32
#include "SimpleIni.h"
#include "IniResult.h"

PreSetup("MQ2IniExt");




map<string, PIniRef> iniMap;
map<string, PIniRef>::iterator iter;
IniData::iterator iniIter;

char DataIniExtTemp[MAX_STRING] = { 0 };
char DataIniExtName[MAX_STRING] = { 0 };
char DataIniExtTemp2[MAX_STRING] = { 0 };
char DataIniRequest[MAX_STRING] = { 0 };
char DataIniTempComment[MAX_STRING] = { 0 };
PCHAR DataIniSection = 0;
PCHAR DataIniKey = 0;
char DataIniDefault[MAX_STRING] = { 0 };
IniData tempData;

void GetIniFilename(PCHAR source, PCHAR dest);

PIniRef LoadIni(const char* filename) {
	string fileString = string(filename);
	iter = iniMap.find(fileString);

	if (iter != iniMap.end()) {
		//WriteChatf("File '%s' found!", fileString);
		return iter->second;
	}
	else {
		SimpleIni* newIni = new SimpleIni(false, true, false);
		SIReturnCode rc = newIni->LoadFile(fileString.c_str());
		if (rc < 0) {
			if (rc == SI_Error::SI_FILE) {
				PIniRef newIniRef = new IniRef(newIni);
				iniMap[fileString] = newIniRef;
				//WriteChatf("File '%s' loaded!", fileString);
				return newIniRef;
			}
			return 0;
		}
		else {
			PIniRef newIniRef = new IniRef(newIni);
			iniMap[fileString] = newIniRef;
			//WriteChatf("File '%s' loaded!", fileString);
			return newIniRef;
		}
	}
	return 0;
}

BOOL dataIniExt(PCHAR szIndex, MQ2TYPEVAR& Ret) {
	if (szIndex) {
		PCHAR pIniFile = 0;
		PCHAR pKey = 0;
		PCHAR pDefault = "";
		char* Next_Token1 = 0;
		if (pIniFile = strtok_s(szIndex, ",", &Next_Token1)) {
			if (DataIniSection = strtok_s(NULL, ",", &Next_Token1)) {
				if (!strcmp(DataIniSection, "-1"))
					DataIniSection = 0;
				if (DataIniKey = strtok_s(NULL, ",", &Next_Token1)) {
					if (!strcmp(DataIniKey, "-1"))
						DataIniKey = 0;
					pDefault = strtok_s(NULL, "¦", &Next_Token1);
					if (!pDefault) {
						pDefault = "";
					}
				}
			}
		}
		else {
			return false;
		}

		CHAR FileName[MAX_STRING] = { 0 };

		GetIniFilename(pIniFile, &FileName[0]);
		ZeroMemory(&DataIniRequest[0], MAX_STRING);

		strcpy_s(DataIniExtName, FileName);
		strcpy_s(DataIniRequest, DataIniExtName);

		if (DataIniSection) strcat_s(DataIniRequest, DataIniSection);
		if (DataIniKey) strcat_s(DataIniRequest, DataIniKey);
		if (pDefault) strcpy_s(DataIniDefault, pDefault);

		if (PIniRef iniRef = LoadIni(DataIniExtName)) {
			PSimpleIni ini = iniRef->pIni;
			if (DataIniSection) {
				if (DataIniKey) {

					/* oututs the whole map
					map<string, PIniRefResult>::iterator iter;
					for (iter = iniRef->cache->begin(); iter != iniRef->cache->end(); iter++)
					{
						WriteChatf("iniresults: %d, %s, %s", iter->second->type, iter->second->str, iter->second->data->front().pItem);
					}*/

					//make it lower case for consistency
					_strlwr_s(DataIniRequest);

					//check if we've found this key before
					auto it = iniRef->cache->find(DataIniRequest);
					//if we have
					if (it != iniRef->cache->end()) {
						Ret.Ptr = it->second;
						Ret.Type = pMQ2IniResultType;
						return true;
					}
					PIniData newData = new IniData();

					ini->GetAllValues(DataIniSection, DataIniKey, *newData);
					if (newData->size() == 0) {
						PIniRefResult newResult = 0;
						if (strlen(pDefault)) {
							PCHAR newString = new char[MAX_STRING];
							ZeroMemory(newString, MAX_STRING);
							strncpy_s(newString, MAX_STRING, pDefault, MAX_STRING - 1);
							newResult = new IniRefResult(newString);
						}
						else {
							newResult = new IniRefResult();
						}
						newResult->isDefault = true;
						Ret.Ptr = newResult;
						Ret.Type = pMQ2IniResultType;
						return true;
					}

					PIniRefResult newResult = new IniRefResult(newData);

					(*iniRef->cache)[DataIniRequest] = newResult;
					Ret.Ptr = newResult;
					Ret.Type = pMQ2IniResultType;
					return true;

				}
				else {
					ini->GetAllKeys(DataIniSection, tempData);
					ZeroMemory(DataIniExtTemp, 2048);
					if (tempData.size() == 0) {
						return false;
					}
					else {
						for (auto x : tempData) {
							sprintf_s(DataIniExtTemp2, "%s|", x.pItem);
							strcat_s(DataIniExtTemp, DataIniExtTemp2);
						}
					}
					Ret.Ptr = &DataIniExtTemp[0];
					Ret.Type = mq::datatypes::pStringType;
					return true;
				}
			}
			else {
				ini->GetAllSections(tempData);
				ZeroMemory(DataIniExtTemp, 2048);
				if (tempData.size() == 0) {
					return false;
				}
				else {
					for (auto x : tempData) {
						sprintf_s(DataIniExtTemp2, "%s|", x.pItem);
						strcat_s(DataIniExtTemp, DataIniExtTemp2);
					}
				}
				Ret.Ptr = &DataIniExtTemp[0];
				Ret.Type = mq::datatypes::pStringType;
				return true;
			}
		}
	}
	return false;
}

char Arg1[MAX_STRING] = { 0 };
char Arg2[MAX_STRING] = { 0 };
char Arg3[MAX_STRING] = { 0 };
char Arg4[MAX_STRING] = { 0 };
char Arg5[MAX_STRING] = { 0 };
char Arg6[MAX_STRING] = { 0 };

PLUGIN_API VOID IniX(PSPAWNINFO pChar, PCHAR szLine) {
	GetArg(Arg1, szLine, 1);
	GetArg(Arg2, szLine, 2);
	if (!Arg1) return;
	CHAR FileName[MAX_STRING] = { 0 };
	*DataIniTempComment = 0;
	if (!_strnicmp(Arg1, "clear", 5)) {
		if (Arg2[0]) {
			GetIniFilename(Arg2, &FileName[0]);

			string fileString = string(FileName);
			iter = iniMap.find(fileString);

			if (iter != iniMap.end()) {
				//WriteChatf("File '%s' found and cleared!", fileString);
				if (iter->second) delete iter->second;
				iniMap.erase(iter);
			}
		}
		else {
			for (auto it : iniMap) {
				if (it.second) delete it.second;
			}
			iniMap.clear();
		}
		return;
	}
	if (!_strnicmp(Arg1, "save", 4) && Arg2[0]) {
		GetIniFilename(Arg2, &FileName[0]);
		if (PIniRef iniRef = LoadIni(FileName)) {
			PSimpleIni ini = iniRef->pIni;
			ini->SaveFile(FileName);
		}
		return;
	}
	if (!_strnicmp(Arg1, "set", 3) || !_strnicmp(Arg1, "write", 5)) {
		// /iniext set "Filename" "Section" "Key" "Value" "Comment"
		//           1          2         3     4       5         6
		GetArg(Arg3, szLine, 3);
		GetArg(Arg4, szLine, 4);
		GetArg(Arg5, szLine, 5);
		GetArg(Arg6, szLine, 6);
		GetIniFilename(Arg2, &FileName[0]);
		if (Arg3[0]) {
			if (PIniRef iniRef = LoadIni(FileName)) {
				PSimpleIni ini = iniRef->pIni;
				ini->SetSpaces(false);
				if (strlen(Arg6)) {
					if (Arg6[0] != '#' && Arg6[0] != ';') {
						sprintf_s(&DataIniTempComment[0], MAX_STRING, "#%s", Arg6);
					}
					else {
						strcpy_s(&DataIniTempComment[0], MAX_STRING, Arg6);
					}
				}
				ini->SetValue(Arg3, strlen(Arg4) ? Arg4 : NULL, strlen(Arg5) ? Arg5 : NULL, strlen(Arg6) ? DataIniTempComment : NULL);
				//WriteChatf("writing [%s] %s=%s with comment %s", Arg3, Arg4 ? Arg4 : "NULL", Arg5 ? Arg5 : "NULL", Arg6 ? Arg6 : "NULL");
			}
		}
		return;
	}
	if (!_strnicmp(Arg1, "delete", 6)) {
		// /iniext delete "Filename" "Section" "Key" remove
		GetArg(Arg3, szLine, 3);
		GetArg(Arg4, szLine, 4);
		GetArg(Arg5, szLine, 5);
		GetIniFilename(Arg2, &FileName[0]);
		if (Arg3[0]) {
			if (PIniRef iniRef = LoadIni(FileName)) {
				PSimpleIni ini = iniRef->pIni;
				bool removeEmpty = false;
				if (!_strnicmp(Arg5, "remove", 6)) removeEmpty = true;
				ini->Delete(Arg3, strlen(Arg4) ? Arg4 : NULL, removeEmpty);

				string fileString = string(FileName);
				iter = iniMap.find(fileString);

				if (iter != iniMap.end()) {
					//WriteChatf("File '%s' found and cleared!", fileString);
					if (iter->second) delete iter->second;
					iniMap.erase(iter);
				}
			}
		}
		return;
	}
	if (!_strnicmp(Arg1, "test", 4) && Arg2[0]) {
		GetIniFilename(Arg2, &FileName[0]);
		WriteChatf("File '%s' referenced", FileName);
		return;
	}
}

void GetIniFilename(PCHAR source, PCHAR dest) {
	PCHAR pTemp = source;

	while (pTemp[0]) {
		if (pTemp[0] == '/')
			pTemp[0] = '\\';
		pTemp++;
	}

	if (source[0] != '\\' && !strchr(source, ':'))
		sprintf_s(dest, MAX_STRING, "%s\\%s", gszMacroPath, source);
	else
		strcpy_s(dest, MAX_STRING, source);

	if (!strchr(source, '.'))
		strcat_s(dest, MAX_STRING, ".ini");
	_strlwr_s(dest, MAX_STRING);
	return;
}

PLUGIN_API VOID InitializePlugin(VOID)
{
	DebugSpewAlways("Initializing MQ2IniExt");
	pMQ2IniResultType = new MQ2IniResultType();
	AddMQ2Data("IniExt", dataIniExt);
	AddCommand("/iniext", IniX, 0, 1, 1);
}

void CleanupIniData() {
	for (auto x : iniMap) {
		if (x.second) delete x.second;
	}
	iniMap.clear();
}

PLUGIN_API VOID ShutdownPlugin(VOID)
{
	DebugSpewAlways("Shutting down MQ2IniExt");
	RemoveMQ2Data("IniExt");
	RemoveCommand("/iniext");
	CleanupIniData();
	delete pMQ2IniResultType;
}
