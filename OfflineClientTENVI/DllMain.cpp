#include"../Share/Simple/Simple.h"
#include"../Share/Hook/SimpleHook.h"
#include"../TestRirePE/RirePE.h"

#pragma pack(push, 1)
// TENVI v127
typedef struct {
	DWORD unk1; // 0x00
	BYTE *packet;
	DWORD unk3; // 0x100
	DWORD unk4; // 0x100
	DWORD unk5; // 0x0
	DWORD encoded;
} OutPacket;

typedef struct {
	DWORD unk1; // 0
	DWORD unk2; // 4
	BYTE *packet; // +0x08
	DWORD unk4; // C
	DWORD unk5; // 10
	DWORD unk6; // 14
	WORD length; // + 0x18 ???
	WORD unk8; // 1A
	DWORD unk9; // 1C
	DWORD decoded; // +0x20
	DWORD unk10;
} InPacket;

typedef struct {
	BYTE padding[0x2C];
	void (__thiscall *_CurrentProcessPacket)(void *, InPacket *);
} FunctionTable;

typedef struct {
	FunctionTable *FT;
} OnPacketClass;

typedef struct {
	BYTE padding[0x160];
	OnPacketClass *OPC;
} OnClass;
#pragma pack(pop)

PipeClient *pc = NULL;
bool ConnectToServer() {
	pc = new PipeClient(L"TenviServer");
	pc->Run();
}

void(__thiscall *_EnterSendPacket)(OutPacket *p) = NULL;
void __fastcall EnterSendPacket_Hook(OutPacket *p) {
	union {
		PacketEditorMessage *pem;
		BYTE *b;
	};

	b = new BYTE[sizeof(PacketEditorMessage) + p->encoded];

	if (!b) {
		return;
	}

	pem->header = SENDPACKET;
	pem->id = 0;
	pem->addr = 0;
	pem->Binary.length = p->encoded;
	memcpy_s(pem->Binary.packet, p->encoded, p->packet, p->encoded);
	pc->Send(b, sizeof(PacketEditorMessage) + p->encoded);

	delete[] b;

	return _EnterSendPacket(p);
}

void MyProcessPacket(InPacket *p) {
	//OnClass **oc = (decltype(oc))0x006DB164;
	//*oc->OPC->FT->_CurrentProcessPacket(oc->OPC, p);

	void(__thiscall *_CurrentProcessPacket)(void *CurrentClass, InPacket *p) = (decltype(_CurrentProcessPacket)(*(DWORD *)(*(DWORD *)(*(DWORD *)(*(DWORD *)0x006DB164 + 0x160) + 0x00) + 0x2C)));
	_CurrentProcessPacket((void *)(*(DWORD *)(*(DWORD *)0x006DB164 + 0x160)), p);
}

// Sender
bool bToBeInject = false;
std::vector<BYTE> global_data;
VOID CALLBACK PacketInjector(HWND, UINT, UINT_PTR, DWORD) {
	if (!bToBeInject) {
		return;
	}

	std::vector<BYTE> data = global_data;
	bToBeInject = false;

	PacketEditorMessage *pcm = (PacketEditorMessage *)&data[0];
	if (pcm->header == RECVPACKET) {
		std::vector<BYTE> packet;
		packet.resize(pcm->Binary.length + 0x04);
		packet[0] = 0xF7;
		packet[1] = 0x39;
		packet[2] = 0xEF;
		packet[3] = 0x39;
		memcpy_s(&packet[4], pcm->Binary.length, &pcm->Binary.packet[0], pcm->Binary.length);
		InPacket p = { 0x00, 0x02, &packet[0], 0x01, 0, 0, (DWORD)packet.size(), 0, 0, 0x04 };
		MyProcessPacket(&p);
	}
}

bool bInjectorCallback = false;

BOOL CALLBACK SearchMaple(HWND hwnd, LPARAM lParam) {
	DWORD pid = 0;
	WCHAR wcClassName[256] = { 0 };
	if (GetWindowThreadProcessId(hwnd, &pid)) {
		if (pid == GetCurrentProcessId()) {
			if (GetClassNameW(hwnd, wcClassName, _countof(wcClassName) - 1)) {
				if (wcscmp(wcClassName, L"EngineClass") == 0) {
					if (!bInjectorCallback) {
						bInjectorCallback = true;
						SetTimer(hwnd, 8787, 50, PacketInjector);
						DEBUG(L"main thread is found by EnumWindows");
					}
				}
				return FALSE;
			}
		}
	}
	return TRUE;
}

bool SetCallBack() {
	if (bInjectorCallback) {
		return true;
	}

	EnumWindows(SearchMaple, NULL);
	return bInjectorCallback;
}

bool SetCallBackThread() {
	while (bInjectorCallback == false) {
		SetCallBack();
		Sleep(1000);
	}

	return true;
}

bool CommunicateThread(PipeServerThread& psh) {
	std::vector<BYTE> data;
	if (psh.Recv(data) && !bToBeInject) {
		global_data.clear();
		global_data = data;
		bToBeInject = true;
	}
	return true;
}

bool TenviClient() {
	PipeServer ps(L"TenviClient");
	ps.SetCommunicate(CommunicateThread);
	return ps.Run();
}

bool SetPacketRecver() {
	HANDLE hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SetCallBackThread, NULL, NULL, NULL);
	if (hThread) {
		CloseHandle(hThread);
	}
	hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)TenviClient, NULL, NULL, NULL);
	if (hThread) {
		CloseHandle(hThread);
	}
	ConnectToServer();
	return true;
}
//


void Init() {
	SHookFunction(EnterSendPacket, 0x0055F2A8);
	SetPacketRecver();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hinstDLL);
		Init();
	}
	return TRUE;
}