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

#include <WiFi.h>      // WiFiライブラリ
#include <WebServer.h>// WebServerライブラリ

// センサを定義する変数
MAX30105 particleSensor;

// センサから取得した値を代入する変数
const byte RATE_SIZE = 4; // これを増やすと、より平均化される。
byte rates[RATE_SIZE];    // 心拍数の配列、(RATE_SIZEの数だけ作成)
byte rateSpot = 0;        // ラップ変数
long lastBeat = 0;        // 最後の心拍が発生した時刻
float beatsPerMinute;     // 1分あたりの心拍
int beatAvg = 0;          // 心拍の平均
String error = "";        // エラー内容
String success = "";       // 成功時表示内容

// ◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇
// アクセスポイント情報
// 環境に合わせて変更ください
const char* ssid = "ENDO Takumi(iPhone)";   // SSID
const char* passwd = "08021323772"; // ネットワークパスワード
// ◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇

// サーバーを代入する変数
WebServer server(80);

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

  // WiFiに接続
  WiFi.begin(ssid, passwd);               // アクセスポイント接続のためのIDとパスワードの設定
  while (WiFi.status() != WL_CONNECTED) { // 接続状態の確認
    delay(300);                           // 接続していなければ0.3秒待つ
    Serial.print(".");                   // 接続しなかったらシリアルモニタに「.」と表示
  }

  // 通信が可能となったら各種情報を表示する
  Serial.println("");               //シリアルモニタ改行
  Serial.println("WiFi Connected"); //接続したらシリアルモニタに「WiFi Connected」と表示
  Serial.print("IP Address : ");    //シリアルモニタに表示
  Serial.println(WiFi.localIP());   //割り当てられたIPアドレスをシリアルモニタに表示

  // serverにアクセスしたときの処理関数
  server.on("/", handleSample);      //TOPページのアドレスにアクセスしたときの処理関数
  server.onNotFound(handleNotFound); //存在しないアドレスにアクセスしたときの処理関数

  // WebServerを起動、server(80)で作成したサーバー
  server.begin();                    //WebServer起動

  Serial.println("人差し指をセンサーに当て、一定の圧力を加えてください。");

  delay(5000);

  particleSensor.setup();                       // センサーを初期化する
  particleSensor.setPulseAmplitudeRed(0x0A);    // 赤色LEDをLowにし、センサーが動作していることを示す
  particleSensor.setPulseAmplitudeGreen(0xFF);  // 緑色LEDを消灯する
}

/*
  ------------
   メインループ
  ------------
*/
void loop() {
  server.handleClient();
  long irValue = particleSensor.getIR(); // IR値を読み取るとセンサーに指があるか知ることができるIRとは赤外線のこと

  if (checkForBeat(irValue) == true) {  //IR値がTrueだったら
    long delta = millis() - lastBeat;   // デルタ = millis() - lastBeat
    // millis() : Arduinoボードがプログラムの実行を開始した時から現在までの時間をミリ秒単位で返します。
    lastBeat = millis();                // lastBeat に心拍を測定した時間を代入

    beatsPerMinute = 60 / (delta / 1000.0); // 一分間の心拍の計算

    if (beatsPerMinute < 255 && beatsPerMinute > 20) { // 平均の計算
      rates[rateSpot++] = (byte)beatsPerMinute;        // 読み取りを配列に格納する
      rateSpot %= RATE_SIZE;                           // ラップ変数

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

  error = "";
  success = "測定中...";
  if (irValue < 50000) { // 指がセンサーに置かれていない場合
    Serial.print("  【エラー】センサーで指を感知できていません");
    error = "センサーで指を感知できていません";
    success = "";
  }

  Serial.println();
}

// TOPページにアクセスしたきの処理関数
void handleSample() {
  String html;

  //HTML記述


  html = "<code class=\"code-multiline\"><!doctype html>";
  html += "<html lang=\"ja\">";
  html += "<head>";
  html += "<meta charset=\"utf-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">";
  html += "<meta http-equiv=\"refresh\" content=\"2\">";
  html += "<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css\" integrity=\"sha384-JcKb8q3iqJ61gNV9KGb8thSsNjpSL0n8PARn9HuZOnIxN0hoP+VmmDGMN5t9UJ0Z\" crossorigin=\"anonymous\">";
  html += "<title>心拍数測定 | Tech Tok Tech</title>";
  html += "</head>";
  html += "<body>";
  html += "<nav class=\"navbar navbar-light bg-light p-3\">";
  html += "<div class=\"d-flex col-12 col-md-3 col-lg-2 mb-2 mb-lg-0 flex-wrap flex-md-nowrap justify-content-between\">";
  html += "<a class=\"navbar-brand\" href=\"#\">";
  html += "心拍数測定";
  html += "</a>";
  html += "</div>";
  html += "<div class=\"col-12 col-md-5 col-lg-8 d-flex align-items-center justify-content-md-end mt-3 mt-md-0\">";
  html += "<div class=\"mr-3 mt-1\">";
  html += "<a class=\"button\"  href=\"javascript:location.reload();\" >再度読み込み</a>";
  html += "</div>";
  html += "</div>";
  html += "</nav>";
  html += "<div class=\"card text-body\" style=\"width: 500px;margin: 0 auto;margin-top: 10px;\">";
  html += "<h5 class=\"card-header\">心拍数データ</h5>";
  html += "<div class=\"card-body\">";
  html += "<h5 class=\"card-title\">データ</h5>";
  html += "<p class=\"card-text\">" + String(beatAvg) + "BPM</p>";
  html += "<p class=\"card-text text-success\">" + success + "</p>";
  html += "<p class=\"card-text text-danger\">" + error + "</p>";
  html += "</div>";
  html += "</div>";
  html += "<script src=\"https://code.jquery.com/jquery-3.5.1.slim.min.js\" integrity=\"sha384-DfXdz2htPH0lsSSs5nCTpuj/zy4C+OGpamoFVy38MVBnE+IbbVYUew+OrCXaRkfj\" crossorigin=\"anonymous\"></script>";
  html += "<script src=\"https://cdn.jsdelivr.net/npm/popper.js@1.16.1/dist/umd/popper.min.js\" integrity=\"sha384-9/reFTGAW83EW2RDu2S0VKaIzap3H66lZH81PoYlFhbGU+6BZp6G7niu735Sk7lN\" crossorigin=\"anonymous\"></script>";
  html += "<script src=\"https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/js/bootstrap.min.js\" integrity=\"sha384-B4gt1jrGC7Jh4AgTPSdUtOBvfO8shuf57BaghqFfPlYxofvL8/KUEfYiJOMMV+rV\" crossorigin=\"anonymous\"></script>";
  html += "</body>";
  html += "</html>";
  html += "</code>";




  // HTMLを出力する
  server.send(200, "text/html", html);
}

// 存在しないアドレスへアクセスしたときの処理関数
void handleNotFound(void) {
  server.send(404, "text/plain", "Not Found");
}
