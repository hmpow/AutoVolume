#pragma once
#ifndef AUTOVOLUMESETTING_H
#define AUTOVOLUMESETTING_H


#define TARGET_ROLE eConsole
// eConsole ,eCommunications, eMultimedia

// サンプリング周期 ※1桁にしないこと 
#define AUTOVOL_PERIOD_MS 20

// ピークホールド デクリメント量 
#define DEFAULT_P_HOLD_DECR_DB_PER_SEC 40 // デフォルト値 
#define DEFAULT_TARGET_SLIDER_POS 50
#define MAX_P_HOLD_DECR_DB_PER_SEC 100 // スライダ-で選べる最大値 

// 音量設定 
#define STOP_TH_DIFF_DB 20.0f  // 無音と判定するピークメータdB値 ※TH = targetVal - STOP_TH_DB  
#define STOP_TH_DB -60.0f
#define TARGET_DB  -26.0f // 目標とする出力レベルデフォルト値 

// 閾値最小値 
// 非常にステップが細かいオーディオデバイスにおいてAPI呼び出し頻度を抑えるため設定 
#define MIN_VOLUME_INCREMENT_DB 0.5f


// 移動平均ステップ数 
#define AVR_STEP 10


// 多重起動防止用MUTEXネーム 
#define MUTEX_NAME "4f79630c6e885a99245f2efd8d159704"

#endif