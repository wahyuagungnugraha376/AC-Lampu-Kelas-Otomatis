#define BLYNK_TEMPLATE_ID "TMPL6I47YkzI5"
#define BLYNK_TEMPLATE_NAME "AC dan Lampu Kelas Otomatis"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LED_AC 13
#define LED_LAMP 12

char auth[] = "yGt57G5tDe296u5pVW_S3YKIeIOxIJ3n";
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

BlynkTimer timer;
LiquidCrystal_I2C lcd(0x27, 16, 2); 

float suhuSimulasi = 23;
int suhuTarget = 23;

int jamSimulasi = 16;     // Awal waktu simulasi
int menitSimulasi = 55;

int lastResetHourSuhu = -1; 
int lastResetHourLampu = -1; 

bool lampuManual = false;
bool overrideLampu = false;

BLYNK_WRITE(V0) {
  int val = param.asInt();
  if (val >= 10 && val <= 35) {
    suhuSimulasi = val;
    Serial.println("Suhu simulasi dari slider: " + String(suhuSimulasi));
  }
}

BLYNK_WRITE(V5) {
  lampuManual = param.asInt();
  overrideLampu = true;
  Serial.println("Lampu manual: " + String(lampuManual ? "ON" : "OFF"));
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_AC, OUTPUT);
  pinMode(LED_LAMP, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Menghubungkan");

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Terhubung");

  Blynk.begin(auth, ssid, pass);
  timer.setInterval(10000L, loopLogic);  // Setiap 10 detik
}

void loopLogic() {
  // Simulasi waktu bertambah setiap 10 detik
  menitSimulasi += 1;
  if (menitSimulasi >= 60) {
    menitSimulasi = 0;
    jamSimulasi++;
    if (jamSimulasi >= 24) jamSimulasi = 0;
  }

  // Reset suhu ke 23 pada jam tertentu
  if ((jamSimulasi == 17 && menitSimulasi == 0 && lastResetHourSuhu != 17) ||
      (jamSimulasi == 23 && menitSimulasi == 59 && lastResetHourSuhu != 24)) {
    suhuSimulasi = 23;
    lastResetHourSuhu = (jamSimulasi == 17) ? 17 : 24;
    Serial.println("Reset suhu otomatis ke 23");
  }

  // Reset override lampu jam 17:00 atau 23:59
  if ((jamSimulasi == 17 && menitSimulasi == 0 && lastResetHourLampu != 17) ||
      (jamSimulasi == 23 && menitSimulasi == 59 && lastResetHourLampu != 24)) {
    overrideLampu = false;
    lampuManual = false;
    lastResetHourLampu = (jamSimulasi == 17) ? 17 : 24;
    Serial.println("Reset override lampu");
  }

  // Reset flags di jam 00:00
  if (jamSimulasi == 0 && menitSimulasi == 0) {
    lastResetHourSuhu = -1;
    lastResetHourLampu = -1;
  }

  // Logika AC
  bool acOn = suhuSimulasi > suhuTarget;
  digitalWrite(LED_AC, acOn);
  Blynk.virtualWrite(V1, acOn);

  // Logika Lampu (nyala jam 18-22 jika tidak override)
  bool lampuOn = overrideLampu ? lampuManual : (jamSimulasi >= 18 && jamSimulasi <= 22);
  digitalWrite(LED_LAMP, lampuOn);
  Blynk.virtualWrite(V2, lampuOn);

  // Kirim suhu 
  Blynk.virtualWrite(V3, suhuSimulasi);

  // Tampilkan ke LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("Jam %02d.%02d %.1fC", jamSimulasi, menitSimulasi, suhuSimulasi);
  lcd.setCursor(0, 1);
  lcd.print("AC:" + String(acOn ? "ON " : "OFF"));
  lcd.print(" Lampu:" + String(lampuOn ? "ON" : "OFF"));

  // Debug Serial
  Serial.printf("Jam: %02d:%02d | %.1f | Target: %d | AC: %s | Lampu: %s\n",
    jamSimulasi, menitSimulasi, suhuSimulasi, suhuTarget,
    acOn ? "ON" : "OFF", lampuOn ? "ON" : "OFF");
}

void loop() {
  Blynk.run();
  timer.run();
}
