#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>

// Inisialisasi LCD
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// Inisialisasi TCS3200 (Sensor Warna)
#define S0 8
#define S1 9
#define S2 10
#define S3 11
#define sensorOut 12

// Inisialisasi Buka Tabungan
#define MASTERKEY_LENGTH 4
#define MAX_USERS 2
#define SERVO_PIN A0
#define SERVO_PIN_360 A1

// LED
const int ledPin = 13;

Servo myServo;
Servo myServo360;

// Informasi Pengguna
struct User{
    char rek[5];
    char pin[5];
    long saldo;
};

User users[MAX_USERS] = {
    {"0123", "5678", 0},  // Pengguna 1
    {"9876", "4321", 0}   // Pengguna 2
};

int currentUser = 0; // Indeks pengguna yang sedang login

// Inisialisasi Keypad
const byte ROW_NUM = 4;    // Jumlah baris keypad
const byte COLUMN_NUM = 4; // Jumlah kolom keypad

char keys[ROW_NUM][COLUMN_NUM] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

byte pin_rows[ROW_NUM] = {7, 6, 5, 4};    // Koneksi baris keypad ke pin Arduino
byte pin_column[COLUMN_NUM] = {3, 2, 1, 0}; // Koneksi kolom keypad ke pin Arduino

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

// Informasi Login
char enteredRek[5];
char enteredPin[5];
int rekIndex = 0;
int pinIndex = 0;
int loginState = 0; // 0 untuk memasukkan rek, 1 untuk memasukkan pin

// Informasi Masterkey
char masterkey[MASTERKEY_LENGTH + 1] = "1234"; // Masterkey default
bool doorUnlocked = false;                      // Menyimpan status kunci pintu

// Informasi Pemindaian Warna/Uang
int redFrequency = 0;
int greenFrequency = 0;
int blueFrequency = 0;

long redColor = 0;
long greenColor = 0;
long blueColor = 0;

// Variabel Saldo
long Saldo = 0;

// Definisi State
enum State{
    LOGIN_REK,
    LOGIN_PIN,
    MAIN_MENU,
    PROCESS_MONEY_INPUT
};

// State awal
State currentState = LOGIN_REK;

void setup(){
    // Set output led
    pinMode(ledPin, OUTPUT);
    // Celengan Terkunci
    myServo.attach(SERVO_PIN);
    myServo.write(180);

    // Tampilan Awal Login
    digitalWrite(ledPin, HIGH);
    lcd.begin(16, 2); // Inisialisasi LCD
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Rekening(4) : ");
    lcd.setCursor(0, 1); // Pindahkan kursor ke baris kedua

    // Aktifkan Sensor Warna PIN
    pinMode(S0, OUTPUT);
    pinMode(S1, OUTPUT);
    pinMode(S2, OUTPUT);
    pinMode(S3, OUTPUT);
    pinMode(sensorOut, INPUT);
    digitalWrite(S0, HIGH);
    digitalWrite(S1, LOW);

    // Jalankan sekali logika
    while (currentState != MAIN_MENU){
        char key = keypad.getKey();
        if (key){
            switch (currentState){
            case LOGIN_REK:
                // Memasukkan rek
                lcd.print(key);
                if (rekIndex < 4){
                    enteredRek[rekIndex] = key;
                    rekIndex++;
                }
                if (rekIndex == 4){
                    enteredRek[rekIndex] = '\0'; // Akhiri array karakter
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Masukkan PIN (5) :");
                    lcd.setCursor(0, 1); // Pindahkan kursor ke baris kedua
                    digitalWrite(ledPin, LOW);
                    currentState = LOGIN_PIN; // Beralih ke memasukkan PIN
                    rekIndex = 0;
                }
                break;

            case LOGIN_PIN:
                // Memasukkan pin
                digitalWrite(ledPin, LOW);
                lcd.print('*'); // Tampilkan '*' sebagai ganti tombol sebenarnya
                if (pinIndex < 4){
                    enteredPin[pinIndex] = key;
                    pinIndex++;
                }
                if (pinIndex == 4){
                    enteredPin[pinIndex] = '\0'; // Akhiri array karakter
                    currentState = LOGIN_REK;     // Beralih kembali ke memasukkan rek
                    pinIndex = 0;
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    if (strcmp(enteredRek, users[currentUser].rek) == 0 && strcmp(enteredPin, users[currentUser].pin) == 0){
                        digitalWrite(ledPin, HIGH);
                        lcd.setCursor(0, 0);
                        lcd.print("AKSES DITERIMA");
                        lcd.setCursor(0, 1);
                        lcd.print("Hai ");
                        lcd.setCursor(4, 1);
                        if (currentUser == 0){
                            lcd.print("Kakak");
                        }
                        else if (currentUser == 1){
                            lcd.print("Adik");
                        }
                        delay(2000);
                        currentState = MAIN_MENU;
                    }
                    else{
                        lcd.print("AKSES DITOLAK");
                        delay(2000); // Tampilkan hasil selama 2 detik
                        digitalWrite(ledPin, HIGH);
                        lcd.clear();
                        lcd.setCursor(0, 0);
                        lcd.print("Rekening(4) : ");
                        lcd.setCursor(0, 1); // Pindahkan kursor ke baris kedua
                    }
                }
                break;

            default:
                break;
            }
        }
    }
    // Setelah berada di MAIN_MENU, panggil fungsi mainMenu
    mainMenu();
}

// Fungsi untuk menampilkan saldo dalam format yang rapi
void printFormattedSaldo(long saldo){
    String formattedSaldo = "";
    String saldoStr = String(saldo);

    int len = saldoStr.length();
    int commaCount = 0;

    for (int i = len - 1; i >= 0; i--){
        formattedSaldo = saldoStr[i] + formattedSaldo;
        commaCount++;

        if (commaCount == 3 && i > 0){
            formattedSaldo = "." + formattedSaldo;
            commaCount = 0;
        }
    }

    lcd.print(formattedSaldo);
}

void mainMenu()
{
    digitalWrite(ledPin, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MENU UTAMA");

    char menuOptions[4][16] = {"A. Menabung ", "B. Cek Saldo   ", "C. Buka Tabungan", "D. Logout"};

    int currentOption = 0;

    char key;
    unsigned long lastKeyPressTime = millis();

    while (true){
        lcd.setCursor(0, 1);
        lcd.print(menuOptions[currentOption]);

        if (key = keypad.getKey()){
            lastKeyPressTime = millis(); // Reset timer pada saat menekan tombol

            switch (key){
            case 'A':
                lcd.clear();
                lcd.print("SELAMAT MENABUNG");
                delay(2000);
                processMoneyInput();
                break;

            case 'B':
                lcd.clear();
                lcd.print("CEK SALDO       ");
                delay(2000);
                cekSaldo();
                break;

            case 'C':
                lcd.clear();
                lcd.print("BUKA TABUNGAN   ");
                delay(2000);
                bukaTabungan();
                break;

            case 'D':
                lcd.clear();
                lcd.print("LOGOUT          ");
                delay(2000);
                currentUser = (currentUser + 1) % MAX_USERS; // Beralih ke pengguna berikutnya
                currentState = LOGIN_REK;                    // Perbarui variabel global currentState
                setup();
                break;

            default:
                lcd.clear();
                lcd.print("SALAH TEKAN");
                delay(2000);
                break;
            }

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("MENU UTAMA");
        }

        if (millis() - lastKeyPressTime >= 2000){
            currentOption = (currentOption + 1) % 4; // Beralih antara 0 dan 1 setiap 2 detik
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("MENU UTAMA");
            lastKeyPressTime = millis(); // Reset timer
        }
    }
}

void processMoneyInput(){
    digitalWrite(ledPin, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MASUKAN UANG");
    lcd.setCursor(0, 1);
    lcd.print("LALU TEKAN *");

    while (true){
        char key = keypad.getKey();
        if (key == '*'){
            lcd.setCursor(0, 0);
            lcd.print("UANG MASUK :    ");

            digitalWrite(S2, LOW);
            digitalWrite(S3, LOW);
            redFrequency = pulseIn(sensorOut, LOW);
            redColor = map(redFrequency, 16, 18, 255, 0);

            digitalWrite(S2, HIGH);
            digitalWrite(S3, HIGH);
            greenFrequency = pulseIn(sensorOut, LOW);
            greenColor = map(greenFrequency, 29, 35, 255, 0);

            digitalWrite(S2, LOW);
            digitalWrite(S3, HIGH);
            blueFrequency = pulseIn(sensorOut, LOW);
            blueColor = map(blueFrequency, 17, 21, 255, 0);

            long uang = 0;
            // Tentukan pengguna berdasarkan indeks saat ini
            User &currentUserObj = users[currentUser];

            if (redColor > greenColor && redColor > blueColor){
                lcd.setCursor(0, 1);
                lcd.print("Rp. 100.000  ");
                uang = 100000;
                delay(1000);
            }
            if (greenColor > redColor && greenColor > blueColor){
                lcd.setCursor(0, 1);
                lcd.print("Rp. 20.000   ");
                uang = 20000;
                delay(1000);
            }
            if (blueColor > redColor && blueColor > greenColor){
                lcd.setCursor(0, 1);
                lcd.print("Rp. 50.000   ");
                uang = 50000;
                delay(1000);
            }

            currentUserObj.saldo += uang;

            myServo360.attach(SERVO_PIN_360);
            myServo360.write(180); // Asumsikan 180 adalah posisi untuk rotasi penuh
            delay(5000);
            myServo360.detach(); // Lepas servo untuk menghentikannya

            lcd.setCursor(0, 0);
            lcd.print("Saldo ");
            lcd.setCursor(6, 0);
            if (currentUser == 0){
                lcd.print("Kakak :   ");
            }
            else if (currentUser == 1){
                lcd.print("Adik :    ");
            }
            lcd.setCursor(0, 1);
            lcd.print("Rp. ");
            printFormattedSaldo(currentUserObj.saldo);

            delay(2000);

            lcd.setCursor(0, 0);
            lcd.print("MASUKAN UANG");
            lcd.setCursor(0, 1);
            lcd.print("LALU TEKAN *");
        }
        else if (key == '#'){
            while (true){
                mainMenu();
            }
        }
    }
}

void cekSaldo(){
    digitalWrite(ledPin, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Saldo ");
    lcd.setCursor(6, 0);
    if (currentUser == 0){
        lcd.print("Kakak :   ");
    }
    else if (currentUser == 1){
        lcd.print("Adik :    ");
    }
    lcd.setCursor(0, 1);
    lcd.print("Rp. ");
    printFormattedSaldo(users[currentUser].saldo);
    while (true){
        char key = keypad.getKey();
        if (key == '#'){
            while (true){
                mainMenu();
            }
        }
    }
}

void bukaTabungan(){
    digitalWrite(ledPin, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Masterkey ");
    while (true){
        char key = keypad.getKey();

        if (key != NO_KEY){
            if (!doorUnlocked){
                checkMasterkey(key);
            }
        }
    }
}

void checkMasterkey(char key){
    static char enteredMasterkey[MASTERKEY_LENGTH + 1];
    static int masterkeyIndex = 0;

    enteredMasterkey[masterkeyIndex++] = key;
    lcd.setCursor(masterkeyIndex - 1, 1);
    lcd.print('*');

    if (masterkeyIndex == MASTERKEY_LENGTH){
        enteredMasterkey[masterkeyIndex] = '\0'; // Akhiri masterkey yang dimasukkan

        if (strcmp(enteredMasterkey, masterkey) == 0){
            lcd.clear();
            lcd.print("Akses diberikan!");
            delay(2000);
            lcd.clear();
            unlockDoor();
        }
        else{
            lcd.clear();
            lcd.print("Akses ditolak!");
            delay(2000);
            lcd.clear();
            lcd.print("Masterkey :     ");
        }

        // Reset variabel
        masterkeyIndex = 0;
        memset(enteredMasterkey, 0, MASTERKEY_LENGTH + 1);
    }
}

void unlockDoor(){
    digitalWrite(ledPin, HIGH);
    lcd.clear();
    lcd.print("Pintu terbuka");

    // Reset saldo untuk semua pengguna menjadi 0
    for (int i = 0; i < MAX_USERS; i++){
        users[i].saldo = 0;
    }

    while (true){
        char key = keypad.getKey();
        myServo.attach(SERVO_PIN);
        myServo.write(90); // Posisi terbuka
        delay(1000);        // Tunggu servo mencapai posisi
        doorUnlocked = true;            // Perbarui status pintu
        lcd.setCursor(0, 1);
        lcd.print("D Untuk Tutup   ");
        if (key == 'D'){
            lockDoor();
        }
    }
}

void lockDoor(){
    digitalWrite(ledPin, HIGH);
    lcd.clear();
    lcd.print("Pintu terkunci");
    while (true){
        myServo.attach(SERVO_PIN);
        myServo.write(180); // Posisi terkunci
        delay(1000);       // Tunggu servo mencapai posisi
        doorUnlocked = false; // Perbarui status pintu
        mainMenu();
    }
}

void loop(){
  
}
