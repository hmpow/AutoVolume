#include "initFileCtrl.h"

namespace initFileCtrl {

	const char initFileName[] = "AutoVolumeInitFile.txt";

	//mapをファイルに保存
	void mapToFile(std::map<std::string, int> inputMap) {
		std::ofstream fout(initFileName);
		if (!fout) {
			//ファイルオープン失敗
			return;
		}
		else {
			//ファイルが開けたらmapの中身をスペース区切りで書き込む
			for (auto iter = inputMap.begin(); iter != inputMap.end(); iter++) {
				fout << iter->first << " " << iter->second << "\n";
			}
			fout << std::endl;
		}
		fout.close();
		return;
	}

	//ファイルからmapに取り込み
	std::map<std::string, int> fileToMap() {
		std::ifstream fin(initFileName);
		std::string keyValue;
		int settingValue;
		bool errFlag = false;

		std::map<std::string, int> readMap;

		if (!fin) {
			//ファイルがなければ白紙を返す
			return readMap;
		}
		else {
			while (!fin.eof()) {
				if (fin >> keyValue >> settingValue) {
					readMap.insert(std::make_pair(keyValue, settingValue));
					errFlag = false;
				}
				else {
					//誤記の行を読むと無限ループに入るので2連続で入ったらエラー処理する
					if (errFlag) {
						readMap.insert(std::make_pair("error", -2));
						//エラーのあるファイルだったら消す
						fin.close();
						remove(initFileName);
						return readMap;
					}
					errFlag = true;
				}
			}
		}
		fin.close();
		return readMap;
	}

	//引数で渡されたmapをファイルから取り込んだデータで更新
	void updateMapFromFile(std::map<std::string, int>* pInputMap) {
		std::map<std::string, int> localMap;

		//ファイルをマップで取ってくる
		//ファイルが無ければ白紙であるので全部catchに入るはず
		localMap = fileToMap();

		//取ってきたマップに引数と同じキーがあったら上書きする
		//ファイル側マップでfor回すとfileを複数マップで共有していた際に実行回数が増える
		std::string key;
		for (auto iter = (*pInputMap).begin(); iter != (*pInputMap).end(); iter++) {
			
			key = iter->first; //検索対象キー

			//キーがあれば取得、無ければデフォルト値返却
			//キーが作られてしまうのでoperator[]出の参照は使用不可
			try {
				(*pInputMap)[key] = localMap.at(key);
			}
			catch (std::out_of_range&) {
				//キーが無かったら更新しない
			}
		}
		return;
	}

	//ファイルにintを1つ追加する
	void writeInt(std::string key, int val) {

		std::map<std::string, int> localMap;

		//ファイルをマップで取ってくる
		localMap = fileToMap();

		//キーがあれば更新、無ければ上書き
		localMap[key] = val;

		//マップをファイルに保存する
		mapToFile(localMap);
		return;
	}


	//ファイルからintを1つ読み込む
	int readInt(std::string key, int defaultVal) {
		std::map<std::string, int> localMap;
		int retVal = defaultVal;

		//ファイルをマップで取ってくる
		localMap = fileToMap();
		
		//キーがあれば取得、無ければデフォルト値返却
		try {
			retVal = localMap.at(key);
			//キャッチ飛ばされれるのでここでリターンないこと
		}
		catch (std::out_of_range&) {
			return defaultVal;
		}
		return retVal;
	}
}
