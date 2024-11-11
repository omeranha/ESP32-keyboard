#pragma once

#include <iostream>
#include <Windows.h>
#include <string>
#include <map>

#define PORT L"COM4"

struct keypress {
	uint8_t key;
	uint8_t modifier;
};

std::map<uint8_t, uint8_t> modifiers = {
	{1, VK_LCONTROL},
	{2, VK_LSHIFT},
	{4, VK_LMENU},
	{8, VK_LWIN},
	{16, VK_RCONTROL},
	{32, VK_RSHIFT},
	{64, VK_RMENU},
	{128, VK_RWIN}
};
