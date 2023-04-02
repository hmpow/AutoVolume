#pragma once
#ifndef INITFILECTRL
#define INITFILECTRL

#include <map>
#include <fstream>

//初期設定ファイルの制御関係
namespace initFileCtrl {
	void writeInt(std::string, int);
	int readInt(std::string, int);
	void updateMapFromFile(std::map<std::string, int>*);
	void mapToFile(std::map<std::string, int>);
}

#endif