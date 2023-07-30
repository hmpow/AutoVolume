#include "AutoVolumeCore.h"

//クラス代用namespace
//Todo 素直にクラスにする
namespace autoVolCore {

    // メンバ変数
    bool isCanRun = false;
    static std::map<std::string, int> paramMap; //設定ファイルのマップ

    // 動作パラメータ設定 ※名前空間内グローバル変数的動作なので関数から操作可能
    float        targetLeveldB   = (float)TARGET_DB;  // 目標とする出力レベル
    float        stopTh_dB       = STOP_TH_DB;        // 無音と判定するピークメータdB値 
    unsigned int peakH_decPerSec = DEFAULT_P_HOLD_DECR_DB_PER_SEC;
    float        p_hold_dec      = 0.0f;

    // API置き場 
    //ToDo namespaceではなくクラスにしてpublic,private適切にする
    //ポインタ変数追加したら忘れないうちにkill_autoVolに書いておくこと

    HRESULT                 hr;
    IMMDeviceEnumerator*    pEnumerator  = NULL;
    IMMDevice*              pDevice      = NULL;
    IAudioMeterInformation* pMeterInfo   = NULL;
    IAudioEndpointVolume*   pEndpointVol = NULL;
    IAudioClient*           pAudioClient = NULL;
    mmNotificationClient    mNotificationClient;


    // IAudioEndpointVolume::GetVolumeRange 用 
    // https://learn.microsoft.com/ja-jp/windows/win32/api/endpointvolume/nf-endpointvolume-iaudioendpointvolume-getvolumerange 
    float pflVolumeMindB = 0.0f;
    float pflVolumeMaxdB = 0.0f;
    float pflVolumeIncrementdB = 0.0f;

    // 処理用データ置き場 
    float currentATT          = 0.0f; // 現在のアッテネータ量 
    float predictOutputDB     = 0.0f; // 予想出力レベル 
    float beforeATTLevelRealT = 0.0f; // リアルタイムの計測値格納 
    float beforeATTLevelPeakH = 0.0f; // ピークホールド値格納 
    float diff_val_db         = 0.0f; // 目標値との差分 
    float nextATT             = 0.0f; // 次にセットするATT量 
    float avr_diff_val_db     = 0.0f; // 差分の移動平均 
    bool  isRestartedFlag     = false;
    BOOL  endVol_isMute       = FALSE; //ミュート状態
    HWND hWnd                 = NULL;  //親ウィンドウ

    //初期化 リソースを取得
    void init_autoVol() {

        //メンバ変数初期化
        ctrl::set_ReleaseRate(peakH_decPerSec);

        //現在のスレッドで COM ライブラリを初期化
        //S_FALSE(既に初期化されている)でも当然使えるのでエラー処理しなくて良い
        (void)CoInitialize(NULL); 

        //デバイスエミュレータインスタンス作成
        const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
        const IID   IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
        hr = CoCreateInstance(
            CLSID_MMDeviceEnumerator,
            NULL,
            CLSCTX_ALL,
            IID_IMMDeviceEnumerator,
            (void**)&pEnumerator);
        error_chk(hr);

        //デフォルトデバイスを取得
        //ToDo：デフォルト以外も選択できるようにする(Zoomで既定以外のデバイス選択に対応)
        //ToDo：TARGET_ROLE(eConsole,eCommunication,eMultimedia)選択に対応
        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, TARGET_ROLE, &pDevice);
        if (hr == E_NOTFOUND) {
            MessageBox(NULL,
                TEXT("有効な出力デバイスが見つかりません"),
                TEXT("AutoVolume エラー"),
                MB_ICONERROR);
        }
        error_chk(hr);

        // ピークメーターをアクティブ化
        hr = pDevice->Activate(
            __uuidof(IAudioMeterInformation),
            CLSCTX_ALL,
            NULL,
            (void**)&pMeterInfo);
        error_chk(hr);

        //エンドポイントボリュームをアクティブ化
        hr = pDevice->Activate(
            __uuidof(IAudioEndpointVolume),
            CLSCTX_ALL,
            NULL,
            (void**)&pEndpointVol);
        error_chk(hr);

        //ボリュームレンジを取得
        pEndpointVol->GetVolumeRange(&pflVolumeMindB, &pflVolumeMaxdB, &pflVolumeIncrementdB);
        error_chk(hr);
        //ボリュームステップが細かいデバイスの場合はsetVolume呼び出し頻度下げるためステップを粗くする
        //連続高速呼び出しするとタスクバーのボリュームつまみが激しく振動して再起動するまで直らなくなる
        if (pflVolumeIncrementdB < MIN_VOLUME_INCREMENT_DB) {
            pflVolumeIncrementdB = MIN_VOLUME_INCREMENT_DB;
        }

        //デバイス変更通知の受け取り登録
        hr = pEnumerator->RegisterEndpointNotificationCallback(&mNotificationClient);
        error_chk(hr);

        //運転準備「入」
        isCanRun = true;
        return;
    }

    //親ウィンドウ指定
    void set_hWnd(HWND _hWnd) {
        hWnd = _hWnd;
        return;
    }

    //終了 リソースを開放
    void release_autoVol() {
        isCanRun = false;
        pEnumerator->UnregisterEndpointNotificationCallback(&mNotificationClient);
        release_resource(&pEnumerator);
        release_resource(&pDevice);
        release_resource(&pMeterInfo);
        release_resource(&pEndpointVol);
        release_resource(&pAudioClient);
        (void)CoUninitialize();
        return;
    }

    //実行中にハード構成が変わり初期化からやり直す
    //ユースケース：bluetoothから有線に切り替えた
    void restart_autoVol() {
        isCanRun = false; //運転準備「切」　※UI側の定期タイマーは動き続けるため
        release_autoVol();
        init_autoVol();
        isCanRun = true;  //運転準備「入」
        isRestartedFlag = true; //UI更新用フラグ
    }


    void error_chk(HRESULT hr) {
        if (hr != S_OK) {
            MessageBox(NULL,
                TEXT("コアオーディオAPIエラー!\nAutoVolumeを終了します"),
                TEXT("AutoVolume エラー"),
                MB_ICONERROR);
            release_autoVol(); //リソース解放
            exit(EXIT_FAILURE); //プログラム終了 abortを使うとエラーが出てうるさい
        }
        return;
    }

    //ウィンドウ消去コールバック




    // 音量調整処理　ループまたは繰り返しタイマーで呼ばれる
    void drive_autoVol() {
        
        if (!isCanRun) {
            //運転準備「切」で呼ばれたら何もしない
            return;
        }

        //デフォルトオーディオデバイスが変わっていたら再起動
        if(mNotificationClient.get_isDefaultDeviceChanged()){
            mNotificationClient.clear_isDefaultDeviceChanged();
            /* MessageBox(hWnd,
                TEXT("デフォルト出力デバイスが変更されました\nAutoVolumeを再スタートします"), 
                TEXT("AutoVolume"), MB_ICONINFORMATION | MB_TASKMODAL); */
            restart_autoVol();
            return; //実行されないはず
        }

        //ミュート時動作停止
        pEndpointVol->GetMute(&endVol_isMute);
        //純粋boolじゃないのであえて冗長に==実装
        if (endVol_isMute == TRUE) {
            return;
        }

        // マスターボリューム前の音量をリニア値で取得
        pMeterInfo->GetPeakValue(&beforeATTLevelRealT);

        // マスターボリューム前の音量をdB値に
        beforeATTLevelRealT = preTrt::to_dB(beforeATTLevelRealT);

        // 無音時動作停止
        if (beforeATTLevelRealT < stopTh_dB) {
            return;
        }

        // ATT前の音量をピークホールド処理
        beforeATTLevelPeakH = preTrt::pk_Hold(beforeATTLevelRealT, pflVolumeMindB, p_hold_dec);

        // 現在のATT設定を取得 
        pEndpointVol->GetMasterVolumeLevel(&currentATT);

        // 出力レベルを計算 
        // predictOutputDB = beforeATTLevelPeakH + currentATT;

        // 単純に今のATT前音量に対してどれだけATTすせば目標値になるかをATT手前だけを見て反映する 
        // SetMasterVolumeLevel = 目標値 - GetPeakValue すればよい 
        // アタック特性は即時、リリース特性はピークホールドのdecrement依存 

        // ATT量を計算 
        nextATT = targetLeveldB - beforeATTLevelPeakH;

        if (nextATT < pflVolumeMindB + pflVolumeIncrementdB) {
            // －∞はできない → MUTEしない最小ボリュームに設定 
            nextATT = pflVolumeMindB + pflVolumeIncrementdB;
        }
        if (pflVolumeMaxdB < nextATT) {
            // 増幅はできない → 最大に設定 
            nextATT = pflVolumeMaxdB;
        }
        if (pflVolumeIncrementdB < abs(currentATT - nextATT)) {
            // 現在の設定値と新しい設定値が調整可能幅を超えていたら反映 
            pEndpointVol->SetMasterVolumeLevel(nextATT, NULL);
        }
        else {
            //音量維持
        }
        return;
    }

    namespace ctrl {

        //UIからの設定用 目標値を0～100で設定
        void set_targetLevel_by_0_100(int level_0_100) {
            
            if (level_0_100 < 1) {
                //無音にならないギリギリに設定
                set_targetLevel(pflVolumeMindB + pflVolumeIncrementdB);
            }
            else if (99 < level_0_100) {
                //最大に設定
                set_targetLevel(pflVolumeMaxdB);
            }
            else {
                float new_Level = pflVolumeMindB + (pflVolumeMaxdB - pflVolumeMindB) * ((float)level_0_100 / 100.0f);
                set_targetLevel(new_Level);
            }
            return;
        }


        //目標値をdB値で設定
        void set_targetLevel(float level_db) {
            if (level_db < pflVolumeMindB + 0.01f) {
                //デバイス最小値未満ならデバイス最小値に設定
                targetLeveldB = pflVolumeMindB;
            }
            else if(pflVolumeMaxdB - 0.01 < level_db){
                //デバイス最大値超過ならデバイス最大値に設定
                targetLeveldB = pflVolumeMaxdB;
            }
            else {
                //問題なければそのまま設定
                targetLeveldB = level_db;
            }
            return;
        }

        //表示用目標値を取得
        float get_taget_lev_db() {
            if(isCanRun){
                return targetLeveldB;
            }
            else {
                return -1;
            }
        }

        //表示用デバイス名取得
        //https://learn.microsoft.com/ja-jp/windows/win32/coreaudio/device-properties を引用
        void get_DeviceName(static TCHAR* buff) {
            LPWSTR pDeviceId = NULL;
            hr = pDevice->GetId(&pDeviceId);
            error_chk(hr);

            IPropertyStore* pIPropertyStore = NULL;
            hr = pDevice->OpenPropertyStore(STGM_READ, &pIPropertyStore);
            error_chk(hr);
            
            PROPVARIANT friendlyName;
            PropVariantInit(&friendlyName);

            hr = pIPropertyStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
            error_chk(hr);

            const wchar_t* targetRole[3] = { L"eConsole" ,  L"eMultimedia" ,  L"eCommunications"};

            wsprintf(buff,_T("対象デバイス (%s):\n %ls \n"), targetRole[TARGET_ROLE], friendlyName.pwszVal);

            PropVariantClear(&friendlyName);
            release_resource(&pIPropertyStore);
            CoTaskMemFree(pDeviceId);
        }

        //リリース速度 dB/s の取得
        unsigned int get_ReleaseRate(void) {
            return peakH_decPerSec;
        }

        //リリース速度 dB/s の設定
        void set_ReleaseRate(unsigned int rate_per_sec) {
            if (0 < rate_per_sec && rate_per_sec <= MAX_P_HOLD_DECR_DB_PER_SEC) {
                peakH_decPerSec = rate_per_sec;
                //ピークホールドの1サイクル復帰量
                p_hold_dec = ((float)AUTOVOL_PERIOD_MS) * ((float)peakH_decPerSec / 1000.0f);
            }
        }

        //リスタートしたか取得・リセット
        bool isRestarted(void) {
            bool isRFlag = isRestartedFlag;
            isRestartedFlag = false;
            return isRFlag;
        }
    }


    namespace preTrt {
        // 復帰付きピークホールド 
        // 線形に減衰させるためdBに変換後呼び出すこと(耳が対数特性のためdB値で線形に減衰させると自然に聴こえる) 
        // 定電流放電付きのピークホールド回路をソフトウェアで実現しています 
        // 一定周期で呼び出す必要があります 
        // @param input     入力値 
        // @param min       最小値 初期値及び減衰限界 ※電子回路のピークホールドにおけるGNDや-Vccに相当
        // @param decrenebt 1コール当たりの減衰量     ※0以下(丸め誤差あるので実際は0.0001より小)を入力すると無効 
        // @return          ピークホールド値 
        float pk_Hold(float input, float min, float decrement) {

            // 初回はminで初期化 
            static float peak = min;

            // decrement値チェックと減衰 
            if (decrement > 0.0001f && peak > (min + decrement)) {
                peak = peak - decrement;
            }

            // data値チェックとピーク値更新 
            if (input > min && input > peak) {
                peak = input;
            }
            return peak;
        }

        // 0.0～1.0に指定された値をdBに変換 
        // 高速周回呼出しする短文関数のためinline化して高速化を図る 
        // @param input 入力値(0.0～0.1) 
        // @return      dB値
        inline float to_dB(float input) {
            float output = 0.0f;
            output = 20.0f * (float)std::log10(input);
            return output;
        }
    }

}

