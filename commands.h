#ifndef __COMMANDS_H__
#define __COMMANDS_H__
/*
親機子機間の通信内容

親機から
01 ゲーム開始
02 ポーズ
09 ゲーム終了
  理由 勝利チーム タイムアップ
00 状況ポーリング
  残り時間
  各ノード色
08 設定配信内容
  ゲーム時間
  制圧に必要な時間
  明るさ
  音
  rssi閾値
  ボタン有効無効

子機から
10 ポーリングへの応答 
  色
  制圧継続時間
19 ゲーム終了
  理由 制圧時間差が残り時間を超えた
*/
#define CMD_STATUS  0x00
#define CMD_START   0x01
#define CMD_PAUSE   0x02
#define CMD_STOP    0x03
#define CMD_CFG     0x08
#define CMD_RESET   0x09
#define RES_STATUS  0x10
#define RES_START   0x11
#define RES_PAUSE   0x12
#define RES_STOP    0x13
#define RES_CFG     0x18
#define RES_RESET   0x19

#endif