﻿
#include "../res/resource.h "
#include "AutoVolumeCore.h"
#include "InitFileCtrl.h"
#include <string>
#include <tchar.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <commctrl.h>


//プロトタイプ宣言
ATOM InitApp(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK MyDlgProc(HWND, UINT, WPARAM, LPARAM);
void saveParamToFile(void);
void initParamMap(void);
void showStartupErrMsg(void);

// グローバル変数
HANDLE    ghMutex;    //ミューテックスオブジェクト
HINSTANCE ghCurrInst; //現在のインスタンスハンドルは良く使うのでグローバル変数にしておくと良い
static    std::map<std::string, int> gParamMap; //動作パラメータマップ 他所から書き換えられないようにstatic

// メイン ウィンドウ クラス名
static TCHAR szWindowClass[] = _T("AutoVolume");

// アプリケーションのタイトル バーに表示される文字列
static TCHAR szTitle[] = _T("AutoVolume");

void debugmsg(LPCWSTR word) {
    /*
    MessageBox(NULL,
        word,
        _T("AutoVolume エラー"),
        MB_ICONERROR);
    */
}

//最適化無効化しないとリリースビルド時InitInstanceでエラーになる
#pragma optimize("", off)
int WINAPI wWinMain(
    _In_ HINSTANCE     hCurrInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR        lpCmdLine,
    _In_ int           nCmdShow
)
{

    //多重起動防止
    ghMutex = CreateMutexA(
        NULL,
        TRUE,
        MUTEX_NAME
    );
    if (ghMutex == NULL)
    {
        debugmsg(_T("mutexがNULL"));
        showStartupErrMsg();
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        debugmsg(_T("mutexでエラーあり"));
        showStartupErrMsg();
        return 1;
    }

    //ここまでくれば多重起動ではない

    MSG msg;
    BOOL bRet;

    ghCurrInst = hCurrInstance; //現在のインスタンスをグローバル変数に格納

    //ウインドウクラスの登録
    if (!InitApp(hCurrInstance))
    {
        showStartupErrMsg();
        debugmsg(_T("ウインドウクラスの登録"));
        return 1;
    }

    //ウィンドウの生成
    if (!InitInstance(hCurrInstance, nCmdShow))
    {
        showStartupErrMsg();
        debugmsg(_T("ウィンドウの生成"));
        return 1;
    }

    //音量ボタンをホットキー登録
    if (!RegisterHotKey(
        NULL, //現在のスレッド
        1,
        MOD_ALT,
        VK_VOLUME_DOWN | VK_VOLUME_UP
    )) {
        showStartupErrMsg();
        debugmsg(_T("音量ボタンをホットキー登録"));
        return 1;
    }

    // メイン メッセージ ループ(メッセージを取得)
    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) //後半2つは普通フィルタ掛けないので0
    {
        if (bRet == -1) {
            break;
        }
        else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}
#pragma optimize("", on)

//ウインドウクラスの登録
ATOM InitApp(HINSTANCE hInstance) {
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1));   //アイコン
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW); 
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);                         //背景
    wcex.lpszMenuName = TEXT("MAINUIMENU");                              //メニュー名
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1)); //小さいアイコン

    //ウィンドウクラスをシステムに登録
    ATOM ret = RegisterClassEx(&wcex);
    return ret;
}



//ウィンドウの生成
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    HWND hWnd;

    hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        szWindowClass,          //アプリケーションの名前(クラス名)
        szTitle,                //タイトル バーに表示されるテキスト(ウィンドウ名)
        (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX), //作成するウィンドウのタイプ(ウィンドウスタイル)
        CW_USEDEFAULT,          //初期位置 (x)
        CW_USEDEFAULT,          //初期位置 (y)
        360,                    //初期ウィンドウ幅
        260,                    //初期ウィンドウ高さ
        NULL,
        NULL,
        hInstance,
        NULL
    );

    //ウィンドウ作成が成功していればウィンドウハンドルが返される
    if (!hWnd) {
        return false; //失敗
    }

    // ShowWindow で表示
    ShowWindow(
        hWnd,
        nCmdShow
    );

    //ウィンドウを更新
    UpdateWindow(hWnd);

    return true; //失敗
}


//メインのウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //メッセージ受信"ループ" だから、あらゆる初期化付きの独自変数はstaticにしておかないと
    //毎回白紙撤回されてしまう
    PAINTSTRUCT ps;
    HDC hdc;
    wchar_t debugmsg[32] = { 0 };
    static TCHAR stmsg[32] = { 0 };
    static TCHAR volmsg[8] = { 0 };
    static TCHAR volLabel[8] = { 0 };
    static TCHAR firiendryName[2048] = { 0 };
    static HWND hBtn;
    static HWND hTrcbar;
    static bool isRun = false;
    static bool isFirstStart = true;
    const RECT  stmsg_area = { 5,5,200,50 };//左上右下
    const RECT  trgVol_area = { 10,40,350,70 };
    RECT  devName_area = { 5,85,330,240 }; //constにしたいがconstにするとtextviewでconstなしポインタアクセスできない

    switch (message)
    {
    case WM_QUERYENDSESSION: // シャットダウンやログオフしようとした
        //動作していたら自動調整停止
        if (isRun) {
            KillTimer(hWnd, AUTOVOL_TIMER);
            autoVolCore::release_autoVol();
        }
        //設定保存
        saveParamToFile();
        //UI更新
        wsprintf(stmsg, _T("停止中"));
        //終了OKをOSに返す
        return true;
        break;
    case WM_CREATE: //開始
        //設定値をファイルから読み込み
        initParamMap();

        //起動停止ボタンを作成(定義済みシステムクラスBUTTON使用)
        hBtn = CreateWindow(TEXT("BUTTON"), TEXT("自動／停止"),
            WS_CHILD | WS_VISIBLE, 230, 150, 100, 35, //左上x座標、左上y座標、幅、高さ
            hWnd, (HMENU)ID_BUTTON, ghCurrInst, NULL);

        //ボリュームつまみを作成(定義済みシステムクラスmsctls_trackbar32使用)
        hTrcbar = CreateWindow(TEXT("msctls_trackbar32"), TEXT(""),
            WS_CHILD | WS_VISIBLE, 50, 40, 225, 40, //左上x座標、左上y座標、幅、高さ
            hWnd, (HMENU)ID_TRCBAR, ghCurrInst, NULL);

        SendMessage(hTrcbar, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));            // レンジを指定
        SendMessage(hTrcbar, TBM_SETTICFREQ, 10, 0);                             // 目盛りの増分
        SendMessage(hTrcbar, TBM_SETPOS, TRUE, gParamMap.at("targetSliderVal")); // 位置の設定
        SendMessage(hTrcbar, TBM_SETPAGESIZE, 0, 10);                            // クリック時の移動量

        wsprintf(volmsg, _T("---dB"));
        wsprintf(stmsg, TEXT("初回起動"));
        wsprintf(volLabel, TEXT("目標:"));

        //リリースレートをメモリから設定
        autoVolCore::ctrl::set_ReleaseRate(gParamMap.at("releaseSliderVal"));
        //停止閾値をメモリから設定
        autoVolCore::ctrl::set_StopTh((float)gParamMap.at("stopThSliderVal"));
        

        break;
    case WM_HSCROLL: //トラックバーがスクロールされたら
        //メモリ更新
        //停止中でもメモリ更新させておけば各地で再取得しなくて良い
        gParamMap["targetSliderVal"] = (int)SendMessage(hTrcbar, TBM_GETPOS, 0, 0);
        if (isRun) {
            //最小値が接続デバイスに依存するためデバイス取得するまで使えない
            autoVolCore::ctrl::set_targetLevel_by_0_100(gParamMap.at("targetSliderVal"));
            wsprintf(volmsg, _T("%3ddB"), (int)autoVolCore::ctrl::get_taget_lev_db());
            InvalidateRect(hWnd, &trgVol_area, TRUE); //UI 更新
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BUTTON://ボタン_自動／停止
            isRun = !isRun;
            if (isRun) { //自動調整開始
                isFirstStart = false;

                autoVolCore::init_autoVol(); //初期化

                //有効なオーディオデバイス未接続で開始した
                if (autoVolCore::ctrl::isErrorStop()) {
                    isRun = false;
                    autoVolCore::release_autoVol();
                    return 0;
                }
                autoVolCore::set_hWnd(hWnd); //ウィンドウ情報設定
                //目標値をメモリから設定
                autoVolCore::ctrl::set_targetLevel_by_0_100(gParamMap.at("targetSliderVal"));
                wsprintf(volmsg, _T("%3ddB"), (int)autoVolCore::ctrl::get_taget_lev_db());
                //デバイス名表示
                autoVolCore::ctrl::get_DeviceName(firiendryName);
                //タイマー開始
                SetTimer(hWnd, AUTOVOL_TIMER, AUTOVOL_PERIOD_MS, NULL);
                //UI更新
                wsprintf(stmsg, _T("自動調整中"));
            }
            else {
                //停止する

                //タイマー停止
                KillTimer(hWnd, AUTOVOL_TIMER);
                //リリース
                autoVolCore::release_autoVol();
                //UI更新
                wsprintf(volmsg, _T("---dB"));
                wsprintf(stmsg, _T("停止中"));
            }
            InvalidateRect(hWnd, NULL, TRUE); //UI Update 全体
            break;
        case IDM_END: //メニュー_終了
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;
        case IDM_DLG: //メニュー_ダイアログボックス
            DialogBox(ghCurrInst, TEXT("RELEASE_SET_DLG"), hWnd, (DLGPROC)MyDlgProc);
            break;
        case IDM_HELP: //メニュー_ヘルプ
            MessageBox(NULL,
                _T("詳細はgitを参照ください\nhttps://github.com/hmpow/AutoVolume"),
                _T("HELP"),
                MB_OK);
            break;
        default:
            break;
        }//コマンド処理switch～case終

    case WM_TIMER:
        //関係ないタイマーイベントはDefWindowProcへ
        if (wParam != AUTOVOL_TIMER) {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }

        autoVolCore::drive_autoVol();

        //実行中かつエラーなら停止
        if (isRun && autoVolCore::ctrl::isErrorStop()) {
            //通常終了とコピペなのでスコープ考える気になったら関数にする
            //停止する
            //タイマー停止
            KillTimer(hWnd, AUTOVOL_TIMER);
            //リリース
            autoVolCore::release_autoVol();
            //UI更新
            wsprintf(volmsg, _T("---dB"));
            wsprintf(stmsg, _T("停止中"));
            isRun = false;
            InvalidateRect(hWnd, NULL, TRUE); //UI Update 全体
        }

        //実行中かつリスタートしていたらUI更新
        //isRestarted()は一度呼ぶと自動でフラグクリアする
        if (isRun && autoVolCore::ctrl::isRestarted()) {
            //目標値をメモリから設定
            autoVolCore::ctrl::set_targetLevel_by_0_100(gParamMap.at("targetSliderVal"));
            wsprintf(volmsg, _T("%3ddB"), (int)autoVolCore::ctrl::get_taget_lev_db());
            //フレンドリーネーム取得
            autoVolCore::ctrl::get_DeviceName(firiendryName);
            InvalidateRect(hWnd, NULL, TRUE); //UI Update 全体
        }
        break;
    case WM_PAINT:

        //デバイスコンテキストハンドルを取得
        hdc = BeginPaint(hWnd, &ps);

        //これから各部分の背景色を残す設定にする(白くしない）
        SetBkMode(hdc, TRANSPARENT);

        SetTextAlign(hdc, TA_LEFT);
        TextOut(
            hdc,
            5,                //x座標
            5,                //y座標
            stmsg,            //文字列
            lstrlen(stmsg)    //文字数
        );

        TextOut(
            hdc,
            5,
            45,
            volLabel,
            lstrlen(volLabel)
        );

        SetTextAlign(hdc, TA_RIGHT);
        TextOut(
            hdc,
            335,
            45,
            volmsg,
            lstrlen(volmsg)
        );

        SetTextAlign(hdc, TA_LEFT);

        DrawText(
            hdc,
            firiendryName,          //文字列
            lstrlen(firiendryName), //文字数
            &devName_area,          //xエリア
            DT_WORDBREAK            //フォーマット
        );

        EndPaint(hWnd, &ps);//BegenPaintしたら必ず呼ぶ

        break;

    case WM_CLOSE:
        //音量ボタンホットキー解除
        UnregisterHotKey(NULL, 1);
        //自動調整停止
        if (isRun) {
            KillTimer(hWnd, AUTOVOL_TIMER);
            autoVolCore::release_autoVol();
        }
        //設定保存
        saveParamToFile();
        //ミューテックス解放
        ReleaseMutex(ghMutex);
        CloseHandle(ghMutex);
        //ウインドウ消す
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY: //✕ボタンが押された
        PostQuitMessage(0); //必ずこれしないとメッセージループが抜けられず終了できない
        break;

    default:
        //システムに処理を任せたら DefWindowProc を返す
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }//メッセージ処理switch～case終

    return 0; //自分で処理したら0を返す
}

//設定のダイアログプロシージャ
BOOL CALLBACK MyDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static int releaserate = 0;
    static HWND hRateTrcbar;
    static HWND hRateText;
    static TCHAR rateLabel[8] = { 0 };

    static int stopTh = 0;
    static HWND hStopThTrcbar;
    static HWND hStopThText;
    static TCHAR StopThLabel[8] = { 0 };

    switch (msg) {
    case WM_INITDIALOG: //ダイアログボックス表示時

        //ToDo トラックバーをクラスにしてコピペをやめる

        //リリースレート
        hRateTrcbar = GetDlgItem(hDlg, ID_RELEASE_TRACKBAR);  //トラックバー取得
        hRateText = GetDlgItem(hDlg, ID_RELASE_VALUE);        //dB/sテキスト取得
        releaserate = autoVolCore::ctrl::get_ReleaseRate();


        SendMessage(hRateTrcbar, TBM_SETRANGE, TRUE, MAKELPARAM(1, MAX_P_HOLD_DECR_DB_PER_SEC)); // レンジを指定
        SendMessage(hRateTrcbar, TBM_SETTICFREQ, 10, 0);          // 目盛りの増分
        SendMessage(hRateTrcbar, TBM_SETPOS, TRUE, releaserate);  // 位置の設定
        SendMessage(hRateTrcbar, TBM_SETPAGESIZE, 0, 5);          // クリック時の移動量
        wsprintf(rateLabel, _T("%3ddB/s"), releaserate);
        SetWindowText(hRateText, rateLabel);

        //停止閾値
        hStopThTrcbar = GetDlgItem(hDlg, ID_DEFAULT_STOP_TH_DB_TRACKBAR);  //トラックバー取得
        hStopThText = GetDlgItem(hDlg, ID_DEFAULT_STOP_TH_DB_VALUE);        //dB/sテキスト取得
        stopTh = (int)autoVolCore::ctrl::get_StopTh();


        SendMessage(hStopThTrcbar, TBM_SETRANGE, TRUE, MAKELPARAM(-100, -20)); // レンジを指定
        SendMessage(hStopThTrcbar, TBM_SETTICFREQ, 10, 0);          // 目盛りの増分
        SendMessage(hStopThTrcbar, TBM_SETPOS, TRUE, stopTh);  // 位置の設定
        SendMessage(hStopThTrcbar, TBM_SETPAGESIZE, 0, 5);          // クリック時の移動量
        wsprintf(StopThLabel, _T("%3ddB"), stopTh);
        SetWindowText(hStopThText, StopThLabel);

        break;

    case WM_HSCROLL:
    {

        HWND hTrackbar = (HWND)lp;  // ここで lParam からトラックバーのハンドルを取得

        switch (GetDlgCtrlID(hTrackbar)) {
        case ID_RELEASE_TRACKBAR:
            releaserate = (int)SendMessage(hRateTrcbar, TBM_GETPOS, 0, 0);
            wsprintf(rateLabel, _T("%3ddB/s"), releaserate);
            SetWindowText(hRateText, rateLabel);
            break;
        case ID_DEFAULT_STOP_TH_DB_TRACKBAR:
            stopTh = (int)SendMessage(hStopThTrcbar, TBM_GETPOS, 0, 0);
            wsprintf(StopThLabel, _T("%3ddB"), stopTh);
            SetWindowText(hStopThText, StopThLabel);
            break;
        default:
            //何もしない
            break;
        }
        break; //バグ修正　忘れていたためトラックバー 下げ クリック時に case WM_COMMAND が引き続いて実行され、wp = 2(IDCANCEL)扱いで閉じてしまった
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDOK:
            gParamMap["releaseSliderVal"] = releaserate;     //メモリに書き込み
            gParamMap["stopThSliderVal"] = stopTh;           //メモリに書き込み
            autoVolCore::ctrl::set_ReleaseRate(releaserate); //反映
            autoVolCore::ctrl::set_StopTh((float)stopTh);    //反映
            EndDialog(hDlg, IDOK);                           //ダイアログを破棄
            return TRUE;
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL); //ダイアログを破棄
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

//設定値をローカル設定情報MAPから外部ファイルに保存
void saveParamToFile(void) {
    initFileCtrl::mapToFile(gParamMap);
    return;
}

//ローカル設定情報MAP初期化
void initParamMap(void) {
    //ローカル設定情報MAP初期化
    gParamMap.insert(std::make_pair("targetSliderVal" , DEFAULT_TARGET_SLIDER_POS));
    gParamMap.insert(std::make_pair("releaseSliderVal", DEFAULT_P_HOLD_DECR_DB_PER_SEC));
    gParamMap.insert(std::make_pair("stopThSliderVal" , DEFAULT_STOP_TH_DB));
    //外部ファイルから読み込んでローカル設定情報MAP更新(外部ファイルが無ければデフォルトのまま)
    initFileCtrl::updateMapFromFile(&gParamMap);
    return;
}

//起動時エラーメッセージ表示
void showStartupErrMsg(void) {
    MessageBox(NULL,
        _T("起動時エラーが発生しました"),
        _T("AutoVolume エラー"),
        MB_ICONERROR);
}
