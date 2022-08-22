#ifndef INIRESULT_H
#define INIRESULT_H

#include <mq/Plugin.h>
#include "SimpleIni.h"
typedef CSimpleIniA SimpleIni;
typedef SimpleIni* PSimpleIni;
typedef CSimpleIniA::TNamesDepend IniData;
typedef SI_Error SIReturnCode;

typedef IniData* PIniData;

#define ISINDEX() (szIndex[0])
#define ISNUMBER() (IsNumber(szIndex))
#define GETNUMBER() (atoi(szIndex))

enum IniResultType {
	MultiValue = 0,
	SingleValue = 1,
	NoValue = 2,
};

class IniRefResult {
public:
	bool isDefault = false;
	IniResultType type;
	union {
		PIniData data;
		PCHAR str;
	};
	IniRefResult(PIniData data) {
		this->type = MultiValue;
		this->data = data;
	}
	IniRefResult(PCHAR str) {
		this->type = SingleValue;
		this->str = str;
	}
	IniRefResult() {
		this->type = NoValue;
		this->data = 0;
	}
	~IniRefResult() {
		if (type == MultiValue && data) delete data;
		if (type == SingleValue && str) delete str;
	}
};

typedef IniRefResult* PIniRefResult;

class IniRef {
public:
	PSimpleIni pIni = 0;
	map<string, PIniRefResult>* cache = new map<string, PIniRefResult>();
	IniRef(PSimpleIni pIni) {
		this->pIni = pIni;
	}
	~IniRef() {
		if (pIni) delete pIni;
		if (cache) {
			for (auto it = cache->begin(); it != cache->end(); ++it) {
				if (it->second) delete it->second;
			}
			delete cache;
		}
	}
};

void ReplaceAllInString(std::string & data, std::string toSearch, std::string replaceStr)
{
	// Get the first occurrence
	size_t pos = data.find(toSearch);

	// Repeat till end is reached
	while (pos != std::string::npos)
	{
		// Replace this occurrence of Sub String
		data.replace(pos, toSearch.size(), replaceStr);
		// Get the next occurrence from the current position
		pos = data.find(toSearch, pos + replaceStr.size());
	}
}

typedef IniRef*  PIniRef;
char DataIniStringTemp[MAX_STRING] = { 0 };
char DataIniStringTemp2[MAX_STRING] = { 0 };
class MQ2IniResultType : public MQ2Type {
private:

	enum IniResultMembers {
		Count, Index, Arg, Compare, CompareCS, Equal, EqualCS, Find,
		Length, Left, Lower, Mid, NotEqual, NotEqualCS, Remove, Replace, Right, Token, Upper
	};

public:

	MQ2IniResultType() : MQ2Type("IniResult") {
		TypeMember(Count);
		TypeMember(Index);
		TypeMember(Arg);
		TypeMember(Compare);
		TypeMember(CompareCS);
		TypeMember(Equal);
		TypeMember(EqualCS);
		TypeMember(Find);
		TypeMember(Length);
		TypeMember(Left);
		TypeMember(Lower);
		TypeMember(Mid);
		TypeMember(NotEqual);
		TypeMember(NotEqualCS);
		TypeMember(Remove);
		TypeMember(Replace);
		TypeMember(Right);
		TypeMember(Token);
		TypeMember(Upper);
	}
	
	void CopyVariable(MQTypeVar &Dest, MQTypeVar Source) {
		Dest.Argb = Source.Argb;
		Dest.DWord = Source.DWord;
		Dest.Float = Source.Float;
		Dest.Int = Source.Int;
		Dest.Ptr = Source.Ptr;
		Dest.Type = Source.Type;
		Dest.Int64 = Source.Int64;
		Dest.UInt64 = Source.UInt64;
	}

	/*bool FromData(MQVarPtr &VarPtr, MQTypeVar &Source) {
		return false;
	}

	bool FromString(MQVarPtr &VarPtr, PCHAR Source) {
		return false;
	}*/
	bool ToString(MQVarPtr VarPtr, PCHAR Destination) {
		if (!VarPtr.Ptr) {
			return false;
		}

		PIniRefResult result = (PIniRefResult)VarPtr.Ptr;
		if (result->type == NoValue) {
			if (result->isDefault) delete result;
			return false;
		} else if (result->type == SingleValue) {
			strcpy_s(Destination, MAX_STRING, result->str);
			if (result->isDefault) delete result;
			return true;
		} else if (result->type == MultiValue) {
			strcpy_s(Destination, MAX_STRING, result->data->front().pItem);
			if (result->isDefault) delete result;
			return true;
		}

		return false;
	}

	bool GetMember(MQVarPtr VarPtr, const char* Member, PCHAR szIndex, MQTypeVar &Dest) {
		if (!VarPtr.Ptr) {
			return false;
		}
		PIniRefResult result = (PIniRefResult)VarPtr.Ptr;
		if (MQTypeMember* pMember = MQ2IniResultType::FindMember(Member)) {
			switch ((IniResultMembers)pMember->ID) {
			case Count:
				if (ISINDEX()) break;
				if (result->type == NoValue) {
					Dest.Int = 0;
				} else if (result->type == MultiValue) {
					Dest.Int = result->data->size();
				} else {
					Dest.Int = 1;
				}
				Dest.Type = mq::datatypes::pIntType;
				if (result->isDefault) delete result;
				return true;
			case Length:
				if (result->type == NoValue) {
					Dest.Int = 0;
				} else if (result->type == MultiValue) {
					Dest.Int = strlen(result->data->front().pItem);
				} else {
					Dest.Int = strlen(result->str);
				}
				Dest.Type = mq::datatypes::pIntType;
				if (result->isDefault) delete result;
				return true;
			case Index:
				if (ISINDEX() && ISNUMBER()) {
					size_t index = GETNUMBER();
					int counter = 0;
					if (result->type == NoValue) {
						return false;
					} else if (result->type == MultiValue) {
						if (index <= result->data->size()) {
							for (auto it = result->data->begin(); it != result->data->end(); ++it) {
								counter += 1;
								if (counter == index) {
									Dest.Ptr = (void*)it->pItem;
									Dest.Type = mq::datatypes::pStringType;
									if (result->isDefault) delete result;
									return true;
								}
							}
						}
					} else {
						if (index == 1) {
							Dest.Ptr = result->str;
							Dest.Type = mq::datatypes::pStringType;
							if (result->isDefault) delete result;
							return true;
						} else {
							if (result->isDefault) delete result;
							return false;
						}
					}
				}
				return false;
			}
			if (result->type == NoValue) {
				return false;
			} else if (result->type == SingleValue) {
				strcpy_s(DataIniStringTemp, result->str);
				if (result->isDefault) delete result;
			} else if (result->type == MultiValue) {
				strcpy_s(DataIniStringTemp, result->data->front().pItem);
				if (result->isDefault) delete result;
			}
			switch ((IniResultMembers)pMember->ID) {
				case Left:
					if (!ISINDEX())
						return false;
					{
						int Len = GETNUMBER();
						if (Len == 0)
							return false;
						if (Len>0) {
							unsigned long StrLen = strlen(DataIniStringTemp);
							if ((unsigned long)Len>StrLen)
								Len = StrLen;
							memmove(DataTypeTemp, DataIniStringTemp, Len);
							DataTypeTemp[Len] = 0;
							Dest.Ptr = &DataTypeTemp[0];
							Dest.Type = mq::datatypes::pStringType;
						} else {
							Len = -Len;
							unsigned long StrLen = strlen(DataIniStringTemp);
							if ((unsigned long)Len >= StrLen) {
								Dest.Ptr = "";
								Dest.Type = mq::datatypes::pStringType;
								return true;
							}
							memmove(DataTypeTemp, DataIniStringTemp, StrLen - Len);
							DataTypeTemp[StrLen - Len] = 0;
							Dest.Ptr = &DataTypeTemp[0];
							Dest.Type = mq::datatypes::pStringType;
						}
					}
					return true;
				case Right:
					if (!ISINDEX())
						return false;
					{
						int Len = GETNUMBER();
						if (Len == 0)
							return false;
						if (Len<0) {
							Len = -Len;
							unsigned long StrLen = strlen(DataIniStringTemp);
							if ((unsigned long)Len >= StrLen) {
								Dest.Ptr = "";
								Dest.Type = mq::datatypes::pStringType;
								return true;
							}
							char *pStart = DataIniStringTemp;
							pStart = &pStart[Len];
							Len = StrLen - Len;
							memmove(DataTypeTemp, pStart, Len + 1);
							Dest.Ptr = &DataTypeTemp[0];
							Dest.Type = mq::datatypes::pStringType;
						} else {
							char *pStart = DataIniStringTemp;
							pStart = &pStart[strlen(pStart) - Len];
							if (pStart<DataIniStringTemp)
								pStart = DataIniStringTemp;
							memmove(DataTypeTemp, pStart, Len + 1);
							Dest.Ptr = &DataTypeTemp[0];
							Dest.Type = mq::datatypes::pStringType;
						}
					}
					return true;
				case Find:
					if (!ISINDEX())
						return false;
					{
						char A[MAX_STRING] = { 0 };
						char B[MAX_STRING] = { 0 };
						strcpy_s(A, DataIniStringTemp);
						strcpy_s(B, (char*)szIndex);
						_strlwr_s(A);
						_strlwr_s(B);
						if (char *pFound = strstr(A, B)) {
							Dest.DWord = (pFound - &A[0]) + 1;
							Dest.Type = mq::datatypes::pIntType;
							return true;
						}
					}
					return false;
				case Remove:
					if (!ISINDEX())
						return false;
					{
						char A[MAX_STRING] = { 0 };
						char B[MAX_STRING] = { 0 };
						char C[MAX_STRING] = { 0 };


						string strA = string(DataIniStringTemp);
						string strB = string((char*)szIndex);
						ReplaceAllInString(strA, strB, "");
						ZeroMemory(&DataTypeTemp[0], 2048);
						strcpy_s(DataTypeTemp, strA.c_str());
						if (Dest.Ptr = DataTypeTemp) {
							Dest.Type = mq::datatypes::pStringType;
							return true;
						} else {
							return false;
						}
					}
				case Replace:
					if (!ISINDEX())
						return false;
					if (PCHAR pComma = strchr(szIndex, ',')) {
						char A[MAX_STRING] = { 0 };
						char B[MAX_STRING] = { 0 };
						char C[MAX_STRING] = { 0 };

						strcpy_s(A, DataIniStringTemp);
						string strA = string(A);
						*pComma = 0;
						strcpy_s(B, (char*)szIndex);
						*pComma = ',';
						strcpy_s(C, (char*)&pComma[1]);

						ReplaceAllInString(strA, B, C);
						ZeroMemory(&DataTypeTemp[0], 2048);
						strcpy_s(DataTypeTemp, strA.c_str());
						if (Dest.Ptr = DataTypeTemp) {
							Dest.Type = mq::datatypes::pStringType;
							return true;
						} else {
							return false;
						}
					}
				case Upper:
					strcpy_s(DataTypeTemp, DataIniStringTemp);
					_strupr_s(DataTypeTemp);
					Dest.Ptr = &DataTypeTemp[0];
					Dest.Type = mq::datatypes::pStringType;
					return true;
				case Lower:
					strcpy_s(DataTypeTemp, DataIniStringTemp);
					_strlwr_s(DataTypeTemp);
					Dest.Ptr = &DataTypeTemp[0];
					Dest.Type = mq::datatypes::pStringType;
					return true;
				case Compare:
					if (ISINDEX()) {
						Dest.Int = _stricmp(DataIniStringTemp, szIndex);
						Dest.Type = mq::datatypes::pIntType;
						return true;
					}
					return false;
				case CompareCS:
					if (ISINDEX()) {
						Dest.Int = strcmp(DataIniStringTemp, szIndex);
						Dest.Type = mq::datatypes::pIntType;
						return true;
					}
					return false;
				case Mid:
				{
					if (PCHAR pComma = strchr(szIndex, ',')) {
						*pComma = 0;
						pComma++;
						PCHAR pStr = DataIniStringTemp;
						unsigned long nStart = GETNUMBER() - 1;
						unsigned long Len = atoi(pComma);
						if (nStart >= strlen(pStr)) {
							Dest.Ptr = "";
							Dest.Type = mq::datatypes::pStringType;
							return true;
						}
						pStr += nStart;
						unsigned long StrLen = strlen(pStr);
						if (Len>StrLen)
							Len = StrLen;
						memmove(DataTypeTemp, pStr, Len);
						DataTypeTemp[Len] = 0;
						Dest.Ptr = &DataTypeTemp[0];
						Dest.Type = mq::datatypes::pStringType;
						return true;
					}
				}
				return false;
				case Equal:
					if (ISINDEX()) {
						Dest.DWord = (_stricmp(DataIniStringTemp, szIndex) == 0);
						Dest.Type = mq::datatypes::pBoolType;
						return true;
					}
					return false;
				case NotEqual:
					if (ISINDEX()) {
						Dest.DWord = (_stricmp(DataIniStringTemp, szIndex) != 0);
						Dest.Type = mq::datatypes::pBoolType;
						return true;
					}
					return false;
				case EqualCS:
					if (ISINDEX()) {
						Dest.DWord = (strcmp(DataIniStringTemp, szIndex) == 0);
						Dest.Type = mq::datatypes::pBoolType;
						return true;
					}
					return false;
				case NotEqualCS:
					if (ISINDEX()) {
						Dest.DWord = (strcmp(DataIniStringTemp, szIndex) != 0);
						Dest.Type = mq::datatypes::pBoolType;
						return true;
					}
					return false;
				case Count:
					if (ISINDEX()) {
						Dest.DWord = 0;
						PCHAR pLast = (PCHAR)DataIniStringTemp - 1;
						while (pLast = strchr(&pLast[1], szIndex[0]))
							Dest.DWord++;
						Dest.Type = mq::datatypes::pIntType;
						return true;
					}
					return false;
				case Arg:
					if (IsNumberToComma(szIndex)) {
						CHAR Temp[MAX_STRING] = { 0 };
						strcpy_s(Temp, DataIniStringTemp);
						if (PCHAR pComma = strchr(szIndex, ',')) {
							*pComma = 0;
							GetArg(DataTypeTemp, Temp, GETNUMBER(), FALSE, FALSE, FALSE, pComma[1]);
							*pComma = ',';
							if (DataTypeTemp[0]) {
								Dest.Ptr = &DataTypeTemp[0];
								Dest.Type = mq::datatypes::pStringType;
								return true;
							}
						} else {
							GetArg(DataTypeTemp, Temp, GETNUMBER());
							if (DataTypeTemp[0]) {
								Dest.Ptr = &DataTypeTemp[0];
								Dest.Type = mq::datatypes::pStringType;
								return true;
							}
						}
					}
					return false;
				case Token:
					if (IsNumberToComma(szIndex)) {
						DWORD N = GETNUMBER();
						if (!N)
							return false;
						//CHAR Temp[MAX_STRING]={0};
						//strcpy_s(Temp,(char *)VarPtr.Ptr);
						if (PCHAR pComma = strchr(szIndex, ',')) {
							*pComma = 0;
							PCHAR pPos = (PCHAR)DataIniStringTemp;//strchr((char *)VarPtr.Ptr,pComma[1]);
							N--;
							while (N && pPos) {
								pPos = strchr(&pPos[1], pComma[1]);
								N--;
							}
							*pComma = ',';
							if (pPos) {
								if (pPos != (PCHAR)DataIniStringTemp)
									pPos++;
								PCHAR pEnd = strchr(&pPos[0], pComma[1]);
								if (pEnd) {
									if (pEnd != pPos) {
										strncpy_s(DataTypeTemp, pPos, pEnd - pPos);
										DataTypeTemp[pEnd - pPos] = 0;
									} else
										DataTypeTemp[0] = 0;
								} else
									strcpy_s(DataTypeTemp, pPos);
								// allows empty returned strings
								Dest.Ptr = &DataTypeTemp[0];
								Dest.Type = mq::datatypes::pStringType;
								return true;
							}
						}
					}
					return false;
				}
			}
		return false;
	}
	~MQ2IniResultType() {}
};

MQ2IniResultType* pMQ2IniResultType = 0;

#endif
