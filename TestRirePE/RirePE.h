#ifndef __PACKETEDITOR_H__
#define __PACKETEDITOR_H__

#include<Windows.h>

#pragma pack(push, 1)
enum MessageHeader {
	SENDPACKET, // stop encoding
	RECVPACKET, // start decoding
	ENCODEHEADER, // start encoding
	ENCODE1,
	ENCODE2,
	ENCODE4,
	ENCODE8,
	ENCODESTR,
	ENCODEBUFFER,
	DECODEHEADER,
	DECODE1,
	DECODE2,
	DECODE4,
	DECODE8,
	DECODESTR,
	DECODEWSTR_1,
	DECODEWSTR_2,
	DECODEBUFFER,
	DECODEEND,
	UNKNOWNDATA, // not decoded by function
	NOTUSED, // recv not used
	WHEREFROM // not encoded by function
};
typedef struct {
	MessageHeader header;
	ULONG_PTR id;
	ULONG_PTR addr;
	union {
		// SEND or RECV
		struct {
			ULONG_PTR length; // �p�P�b�g�̃T�C�Y
			BYTE packet[1]; // �p�P�b�g
		} Binary;
		// Encode or Decode
		struct {
			ULONG_PTR pos; // Encode or Decode���ꂽ�ʒu
			ULONG_PTR size; // �T�C�Y
		} Extra;
		// Encode or Decode �����ʒm
		ULONG_PTR status; // ���
	};
} PacketEditorMessage;
#pragma pack(pop)

#endif