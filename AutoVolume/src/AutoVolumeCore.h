#pragma once
#ifndef AUTOVOLUMECORE_H
#define AUTOVOLUMECORE_H


#include "AutoVolumeSetting.h"
#include "MmNotificationClient.h"

#include <map>
#include <iostream>
#include <tchar.h>
#include <windows.h>
#include <winuser.h>
#include <combaseapi.h>
#include <audiopolicy.h>
#include <shobjidl_core.h>
#include <endpointvolume.h>
#include <Functiondiscoverykeys_devpkey.h>


//自動音量調整コア部分
namespace autoVolCore {
    //プロトタイプ宣言
    void init_autoVol(void);
    void drive_autoVol(void);
    void release_autoVol(void);
    void error_chk(HRESULT);

    //リソースをヌルチェックしてリリース 型不問
    template <class Resource> void release_resource(Resource** ppResource) {
        if (*ppResource)
        {
            (*ppResource)->Release();
            *ppResource = NULL;
        }
    }

    //制御系
    namespace ctrl {
        void         set_targetLevel(float);        //目標値を設定 set_targetLevel_by_0_100から呼ばれる
        void         set_targetLevel_by_0_100(int); //目標値を設定 UIから呼ばれる
        float        get_taget_lev_db(void);        //dB表示の目標値を取得 UIから呼ばれる
        void         get_DeviceName(static TCHAR*); //表示用の名前取得
        unsigned int get_ReleaseRate(void);         //リリース速度 dB/s の取得
        void         set_ReleaseRate(unsigned int); //リリース速度 dB/s の設定
        bool         isRestarted(void);             //リスタートされたか
    }

    //事前処理系
    namespace preTrt {
        float        pk_Hold(float, float, float); //ピークホールド
        inline float to_dB(float);                 //0.0～1.0をdbに変換
    }
}

#endif