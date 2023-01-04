#include"../Share/Simple/Simple.h"
#include"../TestRirePE/RirePE.h"

// Server to Client
bool SendPacket(std::vector<BYTE> &data) {
	Sleep(100);
	PipeClient pc(L"TenviClient");
	if (!pc.Run()) {
		return false;
	}
	std::vector<BYTE> packet(data.size());
	memcpy_s(&packet[0], data.size(), &data[0], data.size());

	union {
		BYTE *b;
		PacketEditorMessage *pcm;
	};

	ULONG_PTR data_length = sizeof(PacketEditorMessage) - 1 + packet.size();
	b = new BYTE[data_length];

	if (!b) {
		return false;
	}

	memset(pcm, 0, sizeof(data_length));
	pcm->header = RECVPACKET;
	pcm->Binary.length = packet.size();
	memcpy_s(&pcm->Binary.packet[0], packet.size(), &packet[0], packet.size());
	pc.Send((BYTE *)pcm, data_length);


	std::wstring wpacket = DatatoString(pcm->Binary.packet, (pcm->Binary.length > 1024) ? 1024 : pcm->Binary.length, true);
	wpacket = L"@" + wpacket;
	wprintf(L"<- %s\n", wpacket.c_str());

	delete[] b;

	return true;
}

class Encoder {
private:
	std::vector<BYTE> packet;
public:
	Encoder(BYTE header) {
		packet.push_back(header);
	}

	std::vector<BYTE>& Get() {
		return packet;
	}

	void Encode1(BYTE val) {
		packet.push_back(val);
	}

	void Encode2(WORD val) {
		packet.push_back(val & 0xFF);
		packet.push_back((val >> 8) & 0xFF);
	}

	void Encode4(DWORD val) {
		packet.push_back(val & 0xFF);
		packet.push_back((val >> 8) & 0xFF);
		packet.push_back((val >> 16) & 0xFF);
		packet.push_back((val >> 24) & 0xFF);
	}

	void EncodeWStr1(std::wstring wstr) {
		Encode1(wstr.length());
		for (size_t i = 0; i < wstr.length(); i++) {
			Encode2((WORD)wstr.at(i));
		}
	}
};

enum CLIENT_PACKET {
	CP_BACK_TO_LOGIN = 0x0A,
};

enum SERVER_PACKET {
	SP_CHARACTER_SELECT = 0x04,
	SP_CHARACTER_SLOT_INFO = 0x05,
	SP_LOGIN = 0x08,
};

// test
void Login() {
	Sleep(3000);

	// Login
	{
		Encoder p(SP_LOGIN);
		p.EncodeWStr1(L"Test");
		p.Encode2(0);
		SendPacket(p.Get());
	}
	// Character Select
	{
		Encoder p(SP_CHARACTER_SELECT);
		p.Encode1(0);
		p.Encode4(-1);
		p.Encode1(0);
		SendPacket(p.Get());
	}
	// Character Slot Info
	{
		Encoder p(SP_CHARACTER_SLOT_INFO);
		p.Encode1(0); // not used = 0, used = 1
		p.Encode1(3); // free slot
		SendPacket(p.Get());
	}
}

bool OnPacket(BYTE *packet, DWORD length) {
	BYTE header = packet[0];
	switch (header) {
	case CP_BACK_TO_LOGIN: {
		Encoder p(SP_LOGIN);
		SendPacket(p.Get());
		return true;
	}
	default:
	{
		return false;
	}
	}
}

// Client to Server
bool Communicate(PipeServerThread& psh) {
	puts("[connected]");
	std::vector<BYTE> data;

	Login();

	while (psh.Recv(data)) {
		PacketEditorMessage &pem = (PacketEditorMessage&)data[0];

		if (pem.header != SENDPACKET) {
			continue;
		}

		if (pem.header == SENDPACKET) {
			std::wstring wpacket = DatatoString(pem.Binary.packet, (pem.Binary.length > 1024) ? 1024 : pem.Binary.length, true);
			wpacket = L"@" + wpacket;
			wprintf(L"-> %s\n", wpacket.c_str());
			OnPacket(pem.Binary.packet, pem.Binary.length);
			continue;
		}
	}
	puts("[disconnected]");
	return true;
}

bool TenviServer() {
	PipeServer ps(L"TenviServer");
	ps.SetCommunicate(Communicate);
	return ps.Run();
}

int main() {
	TenviServer();
	return 0;
}