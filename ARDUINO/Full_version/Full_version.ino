#include <Wire.h>
#include "I2C_LCD.h"
#include <Preferences.h>

Preferences preferences;

I2C_LCD lcd(39);

#include <DallasTemperature.h>
#include <L3G.h>
#include <Keypad.h>
#include <HardwareSerial.h>
HardwareSerial SimComA7670G(2);

L3G gyro;

short sumX = 0;
short sumY = 0;
short sumZ = 0;
short sumXOld = 0;
short sumYOld = 0;
short sumZOld = 0;
short counter = 5;
bool gyroStableOK = true;
short gyroTreshold = 800;  //This setts up sensitivity for gyroscop
unsigned long current_time = 0;
unsigned long interupt_time = 0;
String s = "";
String j = "";

bool ARMED = false;

String returnStringSIMCOM = "";
String toSEND = "";




#define ONE_WIRE_BUS 18
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass the oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// Set the LCD address to 0x27 for a 16 chars and 2 line display

short value = 0;
float voltage;
bool voltage2 = true;  //This one blocks from calling and calling
float R1 = 100000.0;
float R2 = 10000.0;
short valueWire = 0;
float voltageWire;
float temperatureC;
bool temperatureC2 = true;  //This one blocks from calling and calling
bool wireLoopOK = true;
bool wireLoopOK2 = true;  //This one blocks from calling and calling

const byte ROWS = 4;  //four rows
const byte COLS = 4;  //four columns
char keys[ROWS][COLS] = {
  { 'D', 'C', 'B', 'A' },
  { '#', '9', '6', '3' },
  { '0', '8', '5', '2' },
  { '*', '7', '4', '1' }
};
byte rowPins[ROWS] = { 6, 7, 8, 12 };  //connect to the row pinouts of the keypad
byte colPins[COLS] = { 2, 3, 4, 5 };   //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

char key;
String phoneNumber;
float retrievedTemp = 0;
float retrievedVoltage = 0;


void setup() {
  Serial.begin(115200);
  SimComA7670G.begin(115200,SERIAL_8N1, 11, 10);  // SimComA7670G
  initializeModule();
  pinMode(A2, OUTPUT);
  digitalWrite(A2, LOW);  //Keeps LTE module off
  //digitalWrite(A2, HIGH); //Keeps LTE module on

  getPhoneNumberFromEEPROM();  //fill the char array with EEPROM number
  getTemperatureFromEEPROM(retrievedTemp);
  getVoltageFromEEPROM();



  // Start up the library
  sensors.begin();

  Wire.begin();
  lcd.begin(16, 4);
  if (!gyro.init()) {
    //Serial.println("Failed to autodetect gyro type!");
    while (1)
      ;
  }
  gyro.enableDefault();
  current_time = millis();
  keypad.addEventListener(keypadEvent);  // Add an event listener for key events
  //Serial.begin(9600);
}

void loop() {

  getSensorData();
  mainScreen();
  sendHELP();
}
void keypadEvent(KeypadEvent keyN) {
  switch (keypad.getState()) {
    case 0:
      key = keyN;
      break;
  }
}

void sendHELP() {
  if (ARMED && (gyroStableOK == false)) {
    specificHELP("SHAKING HAPPEN", "Someone SHOOK the DEVICE!!!");
  }

  if ((ARMED && (wireLoopOK == false)) && wireLoopOK2) {
    wireLoopOK2 = false;
    specificHELP("WIRE GOT CUT", "Someone CUT the WIRE!!!");
  }


  if ((ARMED && (temperatureC >= retrievedTemp)) && temperatureC2) {
    temperatureC2 = false;
    toSEND = "Current Temperature is:";
    toSEND = toSEND + retrievedTemp;
    toSEND = toSEND + "K";
    specificHELP("Temp is OVER lim", toSEND);
    toSEND = "";
  }

  if ((ARMED && (voltage <= retrievedVoltage)) && voltage2) {
    delay(5000);
    if (voltage <= retrievedVoltage) {
      voltage2 = false;
      toSEND = "Current Voltage is under:";
      toSEND = toSEND + retrievedVoltage;
      toSEND = toSEND + "V";
      specificHELP("Volt is UNDR lim", toSEND);
      toSEND = "";
    }
  }
}
void specificHELP(String messege1, String messege2) {
  digitalWrite(A2, HIGH);
  current_time = millis();
  interupt_time = millis();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(messege1);
  lcd.setCursor(0, 1);
  lcd.print("Network Search..");
  lcd.setCursor(0, 3);
  lcd.print("D>ABORT");
  unsigned short counter2 = 60;
  lcd.setCursor(0, 2);
  lcd.print(counter2);
  //lcd.setCursor(2, 2);
  lcd.print("s");
  lcd.setCursor(4, 2);
  lcd.print("Till TimeOUT");
  lcd.setCursor(0, 3);
  lcd.print("D>ABORT & Disarm");
  unsigned short time2 = 0;
  initializeModule();
  while ((current_time - interupt_time) < 60000) {
    current_time = millis();
    if ((current_time - interupt_time) > time2) {
      time2 = time2 + 5000;
      lcd.setCursor(0, 2);
      lcd.print("  ");
      lcd.setCursor(0, 2);
      lcd.print(counter2);
      counter2 = counter2 - 5;

      //PASTE HERE
      initializeModule();
      if (returnStringSIMCOM[20] == '1') {
        interupt_time = -60000;
      }
    }
    keypad.getKey();
    if (key == 'D') {
      ARMED = false;
      break;
    }
  }
  if (returnStringSIMCOM[20] == '1') {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(messege1);
    lcd.setCursor(3, 2);
    lcd.print("SMS & Call");
  }
  if (key != 'D' && (returnStringSIMCOM[20] == '1' || returnStringSIMCOM[20] == '2')) {
    delay(5000);
    makeHTTPPush(messege2);
    delay(5000);
    SendTextMessage(messege2);
    delay(2000);
    makeCall();
    digitalWrite(A2, LOW);

    key = 'Q';
  } else if (key == 'D') {
    lcd.clear();
    digitalWrite(A2, LOW);
    key = 'Q';
  } else if (returnStringSIMCOM[20] != '1' && returnStringSIMCOM[20] != '2') {
    delay(5000);
    digitalWrite(A2, LOW);
    key = 'Q';
  }


  lcd.clear();
}






void mainScreen() {
  temperatureC = sensors.getTempCByIndex(0) + 273.15;
  lcd.setCursor(0, 0);
  lcd.print("T=");
  lcd.print(temperatureC, 1);
  lcd.print("K");
  lcd.setCursor(10, 0);
  lcd.print("V=");
  lcd.print(voltage, 1);
  lcd.setCursor(0, 1);
  lcd.print("W=");
  if (wireLoopOK) {
    lcd.print("Good");
    wireLoopOK2 = true;
  } else {
    lcd.print(" Bad");
  }
  lcd.setCursor(10, 1);
  lcd.print("S=");

  if (gyroStableOK == false) {
    lcd.print(" Bad");
  } else {
    lcd.print("Good");
  }




  if (ARMED != true) {
    lcd.setCursor(0, 2);
    lcd.print("*>Settings");
    lcd.setCursor(0, 3);
    lcd.print("A>ArmIn300[s]");
    temperatureC2 = true;
  } else {
    lcd.setCursor(1, 2);
    lcd.print("ALARM IS ARMED");
    lcd.setCursor(0, 3);
    lcd.print("D>ABORT");
  }
  keypad.getKey();
  if (key == 'D') {
    ARMED = false;
  }
}
void getSensorData() {
  // Request temperature conversion
  sensors.requestTemperatures();
  value = analogRead(A0);
  voltage = value * (3.5 / 4096) * ((R1 + R2) / R2);
  valueWire = analogRead(A1);
  voltageWire = valueWire * (3.5 / 4096) * ((R1 + R2) / R2);
  if (voltageWire < voltage - 3) {
    wireLoopOK = false;
  } else {
    wireLoopOK = true;
  }

  if (gyroStableOK == false) {
    current_time = millis();
    if ((current_time - interupt_time) > 10000) {
      gyroStableOK = true;
    }
  } else {
    gyro.read();
    sumX = 0;
    sumY = 0;
    sumZ = 0;

    for (int i = 0; i < counter; i++) {
      sumX = sumX + (int)gyro.g.x;
      sumY = sumY + (int)gyro.g.y;
      sumZ = sumZ + (int)gyro.g.z;
      delay(1);
    }
    sumX = sumX / counter;
    sumY = sumY / counter;
    sumZ = sumZ / counter;

    if ((sumX - sumXOld > gyroTreshold) || (sumY - sumYOld > gyroTreshold) || (sumZ - sumZOld > gyroTreshold)) {
      gyroStableOK = false;
      interupt_time = millis();
    } else {
      gyroStableOK = true;
    }
  }


  keypad.getKey();
  if (key == '*') {
    key = 'Q';
    settings1stLayer();
  }
  if (key == 'A') {
    key = 'Q';
    settingsARMLayer();
  }
}
void settingsARMLayer() {
  lcd.clear();
  while (key != 'D') {
    lcd.setCursor(0, 0);
    lcd.print(" ArmIn300[s]");
    lcd.setCursor(0, 2);
    lcd.print("A>Initiate");
    lcd.setCursor(0, 3);
    lcd.print("D>ABORT");
    keypad.getKey();

    if (key == 'A') {

      interupt_time = millis();
      current_time = millis();
      int counter = 30;
      int time = 1000;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("   Arm in :");
      lcd.setCursor(7, 1);
      lcd.print(counter);
      lcd.print("s");
      lcd.setCursor(0, 3);
      lcd.print("D>ABORT");
      while (!((current_time - interupt_time) > 30000)) {
        current_time = millis();
        if ((current_time - interupt_time) > time) {
          time = time + 1000;
          counter--;
          lcd.setCursor(7, 1);
          lcd.print("  ");
          lcd.setCursor(7, 1);
          lcd.print(counter);
        }
        keypad.getKey();
        if (key == 'D') {
          break;
        }
      }
      ARMED = true;
      gyroStableOK = true;
      if (key == 'D') {
        ARMED = false;
      }
      key = 'D';
    }
  }
  lcd.clear();
  key = 'Q';
}
void settings1stLayer() {
  lcd.clear();
  while (key != 'D') {
    lcd.setCursor(0, 0);
    lcd.print("A>New Number");
    lcd.setCursor(0, 1);
    lcd.print("B>New Temperatur");
    lcd.setCursor(0, 2);
    lcd.print("C>New VoltageLim");
    lcd.setCursor(0, 3);
    lcd.print("D>ABORT");
    keypad.getKey();

    if (key == 'A') {
      key = 'Q';
      NewNumberLayerA();
    }
    if (key == 'B') {
      key = 'Q';
      NewNumberLayerB();
    }
    if (key == 'C') {
      key = 'Q';
      NewNumberLayerC();
    }
  }
  lcd.clear();
  key = 'Q';
}


void NewNumberLayerC() {
  lcd.clear();
  bool flagNumberShowed = true;
  while (key != 'D') {
    lcd.setCursor(0, 0);
    lcd.print("Memorized Volt :");
    lcd.setCursor(0, 2);
    lcd.print("Enter new Volt:");
    lcd.setCursor(0, 3);
    lcd.print("A>YES    D>ABORT");

    if (flagNumberShowed) {
      getVoltageFromEEPROM();
      flagNumberShowed = false;
    }
    lcd.setCursor(0, 1);
    lcd.print(retrievedVoltage, 1);

    keypad.getKey();
    if (key == 'A') {
      key = 'Q';
      flagNumberShowed = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter Volt:");
      lcd.setCursor(9, 3);
      lcd.print("D>ABORT");
      toSEND = "";
      while (key != 'D') {
        delay(50);

        keypad.getKey();

        for (int i = 0; i < 12; i++) {
          lcd.setCursor(i, 1);
          lcd.print(" ");
        }
        lcd.setCursor(0, 1);
        lcd.print(toSEND);

        if (toSEND.length() < 5) {
          keyPadInstructions();
        }

        if (toSEND.length() > 0 && toSEND.length() < 6) {
          lcd.setCursor(0, 3);
          lcd.print("A>Save");
          if (key == 'A') {
            key = 'D';
            setVoltageToEEPROM(round(toSEND.toFloat() * 10) / 10.0);
            retrievedVoltage = toSEND.toFloat();
            retrievedVoltage = round(retrievedVoltage * 10) / 10.0;
          }
        }
      }
      toSEND = "";
      lcd.clear();
      key = 'Q';
    }
  }
  lcd.clear();
  key = 'Q';
}
void NewNumberLayerB() {
  lcd.clear();
  bool flagNumberShowed = true;
  while (key != 'D') {
    lcd.setCursor(0, 0);
    lcd.print("Memorized temp :");
    lcd.setCursor(0, 2);
    lcd.print("Enter new temp:");
    lcd.setCursor(0, 3);
    lcd.print("A>YES    D>ABORT");

    if (flagNumberShowed) {
      getTemperatureFromEEPROM(retrievedTemp);
      flagNumberShowed = false;
    }
    lcd.setCursor(0, 1);
    lcd.print(retrievedTemp, 1);

    keypad.getKey();
    if (key == 'A') {
      key = 'Q';
      flagNumberShowed = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter temp:");
      lcd.setCursor(9, 3);
      lcd.print("D>ABORT");
      toSEND = "";
      while (key != 'D') {
        delay(50);

        keypad.getKey();

        for (int i = 0; i < 12; i++) {
          lcd.setCursor(i, 1);
          lcd.print(" ");
        }
        lcd.setCursor(0, 1);
        lcd.print(toSEND);

        if (toSEND.length() < 5) {
          keyPadInstructions();
        }

        if (toSEND.length() > 0 && toSEND.length() < 6) {
          lcd.setCursor(0, 3);
          lcd.print("A>Save");
          if (key == 'A') {
            key = 'D';
            setTemperatureToEEPROM(round(toSEND.toFloat() * 10) / 10.0);
            retrievedTemp = toSEND.toFloat();
            retrievedTemp = round(retrievedTemp * 10) / 10.0;
          }
        }
      }
      toSEND = "";
      lcd.clear();
      key = 'Q';
    }
  }
  lcd.clear();
  key = 'Q';
}
void NewNumberLayerA() {
  lcd.clear();
  bool flagNumberShowed = true;
  while (key != 'D') {
    lcd.setCursor(0, 0);
    lcd.print("Memorized numbr:");
    lcd.setCursor(0, 1);
    lcd.print("+");
    lcd.setCursor(0, 2);
    lcd.print("Enter new numbr:");
    lcd.setCursor(0, 3);
    lcd.print("A>YES    D>ABORT");

    if (flagNumberShowed) {
      getPhoneNumberFromEEPROM();
      flagNumberShowed = false;
    }
    lcd.setCursor(1, 1);
    lcd.print(phoneNumber);

    keypad.getKey();
    if (key == 'A') {
      key = 'Q';
      flagNumberShowed = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter numbr:");
      lcd.setCursor(0, 1);
      lcd.print("+");
      lcd.setCursor(9, 3);
      lcd.print("D>ABORT");
      toSEND = "";
      while (key != 'D') {
        delay(50);

        keypad.getKey();

        for (int i = 1; i < 12; i++) {
          lcd.setCursor(i, 1);
          lcd.print(" ");
        }
        lcd.setCursor(1, 1);
        lcd.print(toSEND);

        if (toSEND.length() < 14) {
          keyPadInstructions();
        }

        if (toSEND.length() > 9 && toSEND.length() < 14) {
          lcd.setCursor(0, 3);
          lcd.print("A>Save");
          if (key == 'A') {
            key = 'D';
            setPhoneNumberFromEEPROM(toSEND);
          }
        }
      }
      toSEND = "";
      lcd.clear();
      key = 'Q';
    }
  }
  lcd.clear();
  key = 'Q';
}
void keyPadInstructions() {
  if (key == '#') {
    toSEND = toSEND + '.';
    key = 'Q';
  }
  if (key == '0') {
    toSEND = toSEND + '0';
    key = 'Q';
  }
  if (key == '1') {
    toSEND = toSEND + '1';
    key = 'Q';
  }
  if (key == '2') {
    toSEND = toSEND + '2';
    key = 'Q';
  }
  if (key == '3') {
    toSEND = toSEND + '3';
    key = 'Q';
  }
  if (key == '4') {
    toSEND = toSEND + '4';
    key = 'Q';
  }
  if (key == '5') {
    toSEND = toSEND + '5';
    key = 'Q';
  }
  if (key == '6') {
    toSEND = toSEND + '6';
    key = 'Q';
  }
  if (key == '7') {
    toSEND = toSEND + '7';
    key = 'Q';
  }
  if (key == '8') {
    toSEND = toSEND + '8';
    key = 'Q';
  }
  if (key == '9') {
    toSEND = toSEND + '9';
    key = 'Q';
  }
}






void getPhoneNumberFromEEPROM() {
  preferences.begin("storage", true); // Start the preferences in read-only mode
  String value = preferences.getString("n_key", ""); // Retrieve the string
  phoneNumber = value;
  preferences.end();                 // Close the preferences
}

void setPhoneNumberFromEEPROM(String newNumber) {
  String v = newNumber;
  preferences.begin("storage", false); // Start the preferences
  preferences.putString("n_key", v);   // Save the string
  preferences.end();                   // Close the preferences
}

void getTemperatureFromEEPROM(float &temperature) {
  preferences.begin("storage", true); // Start the preferences in read-only mode
  String value = preferences.getString("t_key", ""); // Retrieve the string
  temperature = value.toFloat();
  preferences.end();                 // Close the preferences
}

void setTemperatureToEEPROM(float temperature) {
  String v = String(temperature);
  preferences.begin("storage", false); // Start the preferences
  preferences.putString("t_key", v);   // Save the string
  preferences.end();                   // Close the preferences
}

void getVoltageFromEEPROM() {
  preferences.begin("storage", true); // Start the preferences in read-only mode
  String value = preferences.getString("v_key", ""); // Retrieve the string
  retrievedVoltage = value.toFloat();
  preferences.end();                 // Close the preferences
}

void setVoltageToEEPROM(float voltage) {
  String v = String(voltage);
  preferences.begin("storage", false); // Start the preferences
  preferences.putString("v_key", v);   // Save the string
  preferences.end();                   // Close the preferences
}
void initializeModule() {
  SimComA7670G.println("AT+CREG?");
  delay(50);  // Wait for a response
  returnStringSIMCOM = SimComA7670G.readString();
  while (SimComA7670G.available()) {
      String response = SimComA7670G.readString();
      Serial.print(F("Modem response: "));
      Serial.println(response);
  }
}
void SendTextMessage(String textMessege) {
  sendATCommand("AT+CMGF=1\r");

  toSEND = "AT+CMGS=\"";
  toSEND = toSEND + phoneNumber;
  toSEND = toSEND + "\"\r";
  sendATCommand(toSEND);
  sendATCommand(textMessege);
  sendATCommand("\r");
  sendATCommand(String((char)26));
  sendATCommand("");
  toSEND = "";
}

void makeCall() {
  sendATCommand("AT+CFUN=1");
  toSEND = "ATD";
  toSEND = toSEND + phoneNumber;
  toSEND = toSEND + ";\n\r";
  sendATCommand(toSEND);
  delay(10000);
  toSEND = "";
}
void makeHTTPPush(String messege2) {
  j = "{\"Alert\": \"";
  j = j + messege2;
  j = j + "\"}";
  s = "AT+HTTPDATA=";
  s = s + String(j.length());
  s = s + ",10000";
  sendATCommand("AT+HTTPINIT");
  sendATCommand("AT+HTTPPARA=\"URL\",\"https://yourproject-default-rtdb.firebaseio.com/data.json?auth=YourSecretKey\"");
  sendATCommand("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
  sendATCommand(s);
  Serial.println(s);
  sendATCommand(j);
  Serial.println(j);
  sendATCommand("AT+HTTPACTION=4");
  sendATCommand("AT+HTTPTERM");
}
void sendATCommand(String command) {
  SimComA7670G.println(command);
  while (SimComA7670G.available()) {
      String response = SimComA7670G.readString();
      Serial.print(F("Modem response: "));
      Serial.println(response);
  }
  delay(1000);
}