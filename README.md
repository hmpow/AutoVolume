# AutoVolume
The automatic volume control app utilizes the Win32 Core Audio API.  
It is suitable for listening to web meetings, consumer-generated videos, or live streams.  
The app detects the PC's real-time sound output level and controls the master volume using the EndpointVolume API.  
  
Win32 コアオーディオAPIを用いて自動音量調整をするアプリです。  
Web会議、投稿オンライン動画視聴、ライブ配信試聴に適しています。  
EndpointVolume API を用いて、リアルタイムの音声出力レベルを検出し、マスターボリュームを制御します。

<img width="277" alt="image" src="https://github.com/hmpow/AutoVolume/assets/67511807/7ebed174-0d6d-4de0-928a-792404b67626">

# Japanese Only
I can not speak English; therefore, comments and UI texts are available in Japanese only.  
If I write comments in English, I can no longer read my own code...

# Assumed build environment 想定ビルド環境
## OS
Windows 11 Pro 64bit 22H2
## IDE
Microsoft Visual Studio Community 2022  
Version 17.5.0  
Microsoft Visual C++ 2022  
  
### 構成プロパティ
下記資料の手順でプロジェクトを作成後、構成プロパティを変更した  
　https://learn.microsoft.com/ja-jp/cpp/windows/walkthrough-creating-windows-desktop-applications-cpp?view=msvc-170
  
日本語使用のため、C/C++::コマンドライン に "/utf-8" を追加  
WinMain関数で始まるGUIアプリのため、リンカー::システム::サブシステム　を　Windows　に変更  


# Important note about licensing ライセンスに関する重要注意事項
This app is released under the MIT license; however, it includes some sample code from Microsoft API documentation.  
The parts using the sample code are indicated in comments.  
The MIT license does not apply to the quoted code, so please check Microsoft's terms of use before using it.  
  
本アプリはMITライセンスで公開しておりますが、一部Microsoft APIドキュメントのサンプルコードを引用しています。  
サンプルコード使用部分はコメントで記載しております。  
引用コードにMITライセンスは適用されませんので、使用にあたりましてはMicrosoftの利用規約を確認ください。  
  
https://learn.microsoft.com/ja-jp/windows/apps/


# 自動調整の対象

本ソフトウェアで自動操作している対称の音量をGUIの音量ミキサーで当てはめると、下図になります。

各アプリの音がミックスされたマスター音量(EndpointVolume)を制御対象としています。  
「耳で聴こえる音量をもとにマウスでタスクバーの音量を操作する」作業を自動で行うイメージです。

既定のデバイス(=Windowsの効果音が鳴るデバイス)を制御対象としますから、アプリで個別の出力先や排他モードを使用している場合は自動調整ができません。

また音量は減衰させるのみで、0dBを超える調整(増幅)はできません。  
入力音量 < 目標音量　となった際は音量が100% に張り付き自動調整ができません。  
※ミックスされる前の各アプリ毎の個別音量はMAXにておく必要があります。

![image](https://github.com/user-attachments/assets/1a6fd702-50d0-48f4-91ab-a3efb36711ce)


# 詳細 はQiitaをご覧ください

快適Web会議！ Windows コアオーディオAPIで自動音量調整
https://qiita.com/hmpow/items/f54483ead5da0e5e6893
