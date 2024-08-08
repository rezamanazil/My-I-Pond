// Include necessary libraries and define constants
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <addons/RTDBHelper.h>
#include <addons/TokenHelper.h>

// WiFi
#define WIFI_SSID "IPOND Wifi" //username wifi
#define WIFI_PASSWORD "ipond12345" //password wifi

//Link Firebase
#define DATABASE_URL "my-ipond-test-default-rtdb.asiasoutheast1.firebasedatabase.app"
#define API_KEY "AIzaSyBjBiwUYskgTzdpPSLJGPPNpGiEtxIJt-4"
#define USER_EMAIL "rezamanazil@gmail.com"
#define USER_PASSWORD "ipond12345"

// Library Untuk Telegram Bot
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#define BOTtoken "6073919430:AAGLL59pOwsEpNii03vtVS_jm47LnND7y7A" // ini 
dapet dari bot telegramnya (bot father)
#define CHAT_ID "935287938" // ini dapet dari bot (idBot)
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Library Untuk Sensor Suhu
#include <OneWire.h>

// Library Untuk Waktu
#include <NTPClient.h>
#include <WiFiUdp.h>
const long utcOffsetInSeconds = 25200; //(60*60*7) WIB
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", utcOffsetInSeconds);
int DS18S20_Pin = 27; // Pin Untuk Sensor Suhu
OneWire ds(DS18S20_Pin);

// Inisialisasi
int tanggal, bulan, tahun;
String jam, menit, detik, waktu_realtime, tanggal_realtime, suhu, phv, tur;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData fbdo_realtime;
FirebaseData fbdo_data;
unsigned long previousNotificationTime = 0;
const unsigned long interval_notifikasi = 300000; // Interval notifikasi (5 
menit)

// Array untuk menyimpan 10 nilai terakhir
float phValues[10];
float ntuValues[10];
float temperatureValues[10];
int valueIndex = 0;

void setup() {
  Serial.begin(115200);
  // WiFi dan FIREBASE
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
  Serial.print(".");
  delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.database_url = DATABASE_URL;
  config.signer.test_mode = true;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // sertifikat untuk telegram
  timeClient.begin();
  bot.sendMessage(CHAT_ID, "<NOTFIKASI MY I-POND AKTIF>", "");
}

void loop() {
  unsigned long currentMillis = millis();

  // Update waktu + tanggal
  timeClient.update();

  // Tanggal
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  tanggal = ptm->tm_mday;
  bulan = ptm->tm_mon + 1;
  tahun = ptm->tm_year + 1900;
  tanggal_realtime = String(bulan) + "-" + String(tanggal) + "-" + 
  String(tahun);
  Serial.println("Tanggal Sekarang : " + tanggal_realtime);

  // Waktu
  waktu_realtime = timeClient.getFormattedTime();
  jam = String(timeClient.getHours());
  menit = String(timeClient.getMinutes());
  detik = String(timeClient.getSeconds());
  waktu_realtime = jam + ":" + menit + ":" + detik;
  Serial.println("Waktu Sekarang : " + waktu_realtime);

  // Nilai pH
  int sensorValueph = analogRead(33);
  float voltageph = sensorValueph / 4096.0 * 3.3;
  float ph = (123.33 - sensorValueph) / -175.06;
  Serial.print("PH: ");
  Serial.print(ph);
  Serial.print("\tADC PH: ");
  Serial.print(sensorValueph);
  Serial.print("\tVoltage PH: ");
  Serial.println(voltageph);

  // Nilai Kekeruhan
  int sensorValueturdibity = analogRead(32);
  float voltageturdibity = sensorValueturdibity / 4096.0 * 3.3;
  float ntu = (2464.3571 - sensorValueturdibity) / 0.8452;
  if (ntu < 0) {
  ntu = 0;
  }
  Serial.print("NTU: ");
  Serial.print(ntu);
  Serial.print("\tADC Turdibity: ");
  Serial.print(sensorValueturdibity);
  Serial.print("\tVoltage Turdibity: ");
  Serial.println(voltageturdibity);

  // Nilai Suhu
  float temperature = getTemp();
  Serial.print("Suhu: ");
  Serial.print(temperature);
  Serial.println(" C");
  Serial.println(" ");

  // Simpan nilai ke array
  phValues[valueIndex] = ph;
  ntuValues[valueIndex] = ntu;
  temperatureValues[valueIndex] = temperature;
  valueIndex = (valueIndex + 1) % 10; // Use 10 instead of 30
  // Realtime di firebasenya yang sudah dirata ratakan
  realtimefirebase(ph, ntu, temperature);

  // Hitung rata-rata jika array sudah terisi penuh
  if (valueIndex == 0) {
    float avgPh = calculateAverage(phValues, 10);
    float avgNtu = calculateAverage(ntuValues, 10);
    float avgTemperature = calculateAverage(temperatureValues, 10);

    // Pengkondisian push Data ke Firebase
    if(waktu_realtime >= "7:0:0" && waktu_realtime <= "7:0:30"){
      pushdatabase(avgPh, avgNtu, avgTemperature, waktu_realtime, 
      tanggal_realtime);
    }
    if(waktu_realtime >= "13:0:0" && waktu_realtime <= "13:0:30"){
      pushdatabase(avgPh, avgNtu, avgTemperature, waktu_realtime, 
      tanggal_realtime);
    }
    if(waktu_realtime >= "16:0:0" && waktu_realtime <= "16:0:30"){
      pushdatabase(avgPh, avgNtu, avgTemperature, waktu_realtime, 
      tanggal_realtime);
    }
    if(waktu_realtime >= "20:0:0" && waktu_realtime <= "20:0:30"){
      pushdatabase(avgPh, avgNtu, avgTemperature, waktu_realtime, 
      tanggal_realtime);
    }
    // Notifikasi ke Telegram
    if (currentMillis - previousNotificationTime >= interval_notifikasi) {
      previousNotificationTime = currentMillis;
      notifikasi(ph, temperature, ntu);
    }
  }
  delay(1000);
}

// Kodingan untuk Sensor Suhu by website resmi
float getTemp() {
  // returns the temperature from one DS18S20 in DEG Celsius
  byte data[12];
  byte addr[8];

  if (!ds.search(addr)) {
    // no more sensors on chain, reset search
    ds.reset_search();
    return -1000;
    }

  if (OneWire::crc8(addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return -1000;
    }

  if (addr[0] != 0x10 && addr[0] != 0x28) {
    Serial.print("Device is not recognized");
    return -1000;
    }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); // start conversion, with parasite power on at the end
  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad
  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
    }

  ds.reset_search();
  byte MSB = data[1];
  byte LSB = data[0];
  float tempRead = ((MSB << 8) | LSB); // using two's compliment
  float TemperatureSum = tempRead / 16;
  return TemperatureSum;
}

// Fungsi untuk menghitung rata-rata
float calculateAverage(float values[], int length) {
  float sum = 0;
  for (int i = 0; i < length; i++) {
    sum += values[i];
    }
  return sum / length;
}

// Fungsi Push ke Firebase berupa json
void pushdatabase(float avgPh, float avgNtu, float avgTemperature, String 
waktu_realtime, String tanggal_realtime) {

  FirebaseJson json;
  suhu = String(avgTemperature);
  phv = String(avgPh);
  tur = String(avgNtu);
  String waktu_dan_tanggal = tanggal_realtime + " " + waktu_realtime;

  json.add("Temperature", suhu);
  json.add("pH", phv);
  json.add("Turbidity", tur);
  json.add("Tanggal", waktu_dan_tanggal);

  if (Firebase.RTDB.pushJSON(&fbdo_data, "/Data_MyIpond/Data_Json", &json)){
    Serial.println(fbdo_data.dataPath() + "/" + fbdo_data.pushName());
    Serial.print("Data Berhasil Masuk");
    Serial.println();
  } 
  else {
    Serial.println("Data Gagal Masuk");
    Serial.println(fbdo_data.errorReason());
  }
}
// Fungsi Realtime di databasenya
void realtimefirebase(float ph,float ntu, float temperature) {
  suhu = String(temperature);
  phv = String(ph);
  tur = String(ntu);
  Firebase.RTDB.setString(&fbdo_realtime, 
  "/Data_MyIpond/Data_RealTime/PH_AIR", phv);
  Firebase.RTDB.setString(&fbdo_realtime, 
  "/Data_MyIpond/Data_RealTime/KEKERUHAN_AIR", tur);
  Firebase.RTDB.setString(&fbdo_realtime, 
  "/Data_MyIpond/Data_RealTime/SUHU_AIR", suhu);
}

// Fungsi Notifikasi Telegram
void notifikasi(float ph, float temperature, float ntu) {
  if(ph < 6) {
    bot.sendMessage(CHAT_ID, "Kondisi Air Tambak ASAM");
    bot.sendMessage(CHAT_ID, "Segera Periksa Kondisi Air Tambak");
  }
  if(ph > 9) {
    bot.sendMessage(CHAT_ID, "Kondisi Air Tambak BASA");
    bot.sendMessage(CHAT_ID, "Segera Periksa Kondisi Air Tambak");
  }
  if(temperature < 24) {
    bot.sendMessage(CHAT_ID, "Kondisi Air Tambak DINGIN");
    bot.sendMessage(CHAT_ID, "Segera Periksa Kondisi Air Tambak");
  }
  if(temperature > 30) {
    bot.sendMessage(CHAT_ID, "Kondisi Air Tambak PANAS");
    bot.sendMessage(CHAT_ID, "Segera Periksa Kondisi Air Tambak");
  }
  if(ntu > 100) {
    bot.sendMessage(CHAT_ID, "Kondisi Air Tambak KERUH");
    bot.sendMessage(CHAT_ID, "Segera Periksa Kondisi Air Tambak");
  }
}
