#pragma once
#include "c_uipanel.h"
class c_uievent
{
public:
	c_uipanel* pos1;
	char pad_0008[ 8 ];
	c_uipanel* pos2;
	c_uipanel* pos3;
	char pad_0020[ 24 ];
	c_uipanel* pos4;
	char pad_0040[ 8 ];
	c_uipanel* pos5;
	c_uipanel* pos6;
	char pad_0058[ 8 ];
	c_uipanel* pos7;
	c_uipanel* pos8;
	char pad_0070[ 16 ];
	c_uipanel* pos9;
	c_uipanel* pos10;
	c_uipanel* pos11;
	char pad_0098[ 24 ];
	c_uipanel* pos12;
	c_uipanel* pos13;
	char pad_00C0[ 8 ];
	c_uipanel* pos14;
	char pad_00D0[ 16 ];
	c_uipanel* pos15;
	char pad_00E8[ 8 ];
	char* eventName;
	char pad_00F8[ 16 ];
	c_uipanel* pos16;
	char pad_0110[ 8 ];
	c_uipanel* pos17;
	char pad_0120[ 40 ];
	c_uipanel* pos18;
	char pad_0150[ 8 ];
	c_uipanel* pos19;
	char pad_0160[ 8 ];
	c_uipanel* pos20;
	char pad_0170[ 32 ];
	c_uipanel* pos21;
	char pad_0198[ 360 ];
};