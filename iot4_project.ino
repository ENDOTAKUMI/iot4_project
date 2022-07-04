/*
  ------------ ------------
  IoT実習4
  Tech Tok Tech
  ------------ ------------
*/


/*
  ------------
  読み込み
  ------------
*/
#include <Wire.h>      // Wire.h I2C（Inter-Integrated Circuit）通信を行うための通信ライブラリ
#include "MAX30105.h"  // MAX30105.h センサのライブラリ
#include "heartRate.h" // 心拍数算出アルゴリズム

// センサを定義する変数
MAX30105 particleSensor;

const byte RATE_SIZE = 4; // これを増やすと、より平均化される。
byte rates[RATE_SIZE];    // 心拍数の配列、(RATE_SIZEの数だけ作成)
byte rateSpot = 0;        // ラップ変数
long lastBeat = 0;        // 最後の心拍が発生した時刻
float beatsPerMinute;     // 1分あたりの心拍
int beatAvg;              // 心拍の平均

/*
  ------------
  初期化処理
  ------------
*/
void setup() {
  Serial.begin(115200);
  Serial.println("初期化中...");

  // センサーが接続されているか
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) { // デフォルトのI2Cポート、400kHzを使用します。speed
    Serial.println("MAX30105が見つかりませんでした。配線/電源を確認してください。 ");
    while (1);
  }

  Serial.println("人差し指をセンサーに当て、一定の圧力を加えてください。");

  particleSensor.setup();                    // センサーを初期化する
  particleSensor.setPulseAmplitudeRed(0x0A); // 赤色LEDをLowにし、センサーが動作していることを示す
  particleSensor.setPulseAmplitudeGreen(0xFF);  // 緑色LEDを消灯する
}

/*
  ------------
   メインループ
  ------------
*/
void loop() {
  long irValue = particleSensor.getIR(); // IR値を読み取るとセンサーに指があるか知ることができるIRとは赤外線のこと

  if (checkForBeat(irValue) == true) {    //IR値がTrueだったら
    long delta = millis() - lastBeat;   // デルタ = millis() - lastBeat
    // millis() : Arduinoボードがプログラムの実行を開始した時から現在までの時間をミリ秒単位で返します。
    lastBeat = millis();                // lastBeat に心拍を測定した時間を代入

    beatsPerMinute = 60 / (delta / 1000.0); // 一分間の心拍の計算

    if (beatsPerMinute < 255 && beatsPerMinute > 20) { // 平均の計算
      rates[rateSpot++] = (byte)beatsPerMinute; // 読み取りを配列に格納する
      rateSpot %= RATE_SIZE; // ラップ変数

      // 読み取り値の平均を取る
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  // 出力関係
  Serial.print(" IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  if (irValue < 50000) { // 指がセンサーに置かれていない場合
    Serial.print("  【エラー】センサーで指を感知できていません");
  }

  Serial.println();
}
