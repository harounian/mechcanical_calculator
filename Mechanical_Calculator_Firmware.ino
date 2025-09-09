#include <Keypad.h>

#define DATA_IN 13
#define LOAD 12
#define CLK 11

const byte ROWS = 5;
const byte COLS = 4;
byte rowPins[ROWS] = {6, 7, 8, 9, 10};
byte colPins[COLS] = {2, 3, 4, 5};
char keys[ROWS][COLS] = {
  {'C', '/', '*', '-'},
  {'7', '8', '9', '+'},
  {'4', '5', '6', 'X'},
  {'1', '2', '3', 'X'},
  {'0', '.', '=', 'X'}
};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String input = "";
double operand1 = 0, operand2 = 0;
char operation = 0;
bool isEnteringSecondOperand = false;
bool hasResult = false;
bool isNegative = false;
bool hasDecimal = false;

void max7219_send(byte reg, byte data) {
  digitalWrite(LOAD, LOW);
  shiftOut(DATA_IN, CLK, MSBFIRST, reg);
  shiftOut(DATA_IN, CLK, MSBFIRST, data);
  digitalWrite(LOAD, HIGH);
}

void max7219_init() {
  pinMode(DATA_IN, OUTPUT);
  pinMode(LOAD, OUTPUT);
  pinMode(CLK, OUTPUT);
  digitalWrite(LOAD, HIGH);
  max7219_send(0x09, 0xFF);
  max7219_send(0x0A, 0x0C);
  max7219_send(0x0B, 0x07);
  max7219_send(0x0C, 0x01);
  max7219_send(0x0F, 0x00);
}

void clear_display() {
  for (int i = 1; i <= 8; i++) {
    max7219_send(i, 0x0F);
  }
  delay(100);
}

String trimTrailingZeros(String numStr) {
  int dotPos = numStr.indexOf(".");
  if (dotPos == -1) return numStr; // No decimal point, no need to trim

  while (numStr.endsWith("0")) {
    numStr.remove(numStr.length() - 1);
  }
  if (numStr.endsWith(".")) {
    numStr.remove(numStr.length() - 1); // Remove decimal point if it’s the last character
  }
  return numStr;
}

void display_number(String numStr) {
  clear_display();
  bool isNeg = numStr.startsWith("-");
  int len = numStr.length();
  int dotPos = numStr.indexOf(".");
  
  // Limit display to 8 digits
  if (len > 8) {
    numStr = numStr.substring(0, 8);
  }

  int digit = 1;
  for (int i = len - 1; i >= 0; i--) {
    char c = numStr[i];
    byte value = (c == '-') ? 0x0A : (c - '0');

    if (dotPos != -1 && i == dotPos) {
      max7219_send(digit, (numStr[i - 1] - '0') | 0x80); // Add decimal point to previous digit
      i--; // Skip over the decimal itself
    } else {
      max7219_send(digit, value);
    }
    digit++;
  }
}

void setup() {
  Serial.begin(9600);
  max7219_init();
  display_number("0");
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    Serial.print("Key Pressed: ");
    Serial.println(key);
    clear_display();

    if ((key >= '0' && key <= '9') || key == '.') {
      if (hasResult) {
        input = "";
        hasResult = false;
        hasDecimal = false;
      }
      if (key == '.' && !hasDecimal) {
        if (input.length() == 0) {
          input = "0";
        }
        input += ".";
        hasDecimal = true;
      } else if (key != '.') {
        if (input.length() < 8) {
          input += key;
        }
      }
      display_number((isNegative ? "-" : "") + input);
    } 
    else if (key == 'C') {
      input = "";
      operand1 = 0;
      operand2 = 0;
      operation = 0;
      isEnteringSecondOperand = false;
      hasResult = false;
      isNegative = false;
      hasDecimal = false;
      display_number("0");
    } 
    else if (key == '-' && input.length() == 0) {
      isNegative = !isNegative;
      display_number(isNegative ? "-" : "0");
    }
    else if (key == '+' || key == '-' || key == '*' || key == '/') {  
      if (input.length() > 0) {
        operand1 = input.toDouble() * (isNegative ? -1 : 1);
        operation = key;
        input = "";
        isEnteringSecondOperand = true;
        isNegative = false;
        hasDecimal = false;
      }
    } 
    else if (key == '=' && isEnteringSecondOperand) {  
      operand2 = input.toDouble() * (isNegative ? -1 : 1);
      double result = 0;
      
      // Handle division by zero
      if (operation == '/' && operand2 == 0) {
        input = "Error";  // Display error message for division by zero
        display_number(input);
      } else {
        switch (operation) {
          case '+': result = operand1 + operand2; break;
          case '-': result = operand1 - operand2; break;
          case '*': result = operand1 * operand2; break;
          case '/': result = operand1 / operand2; break;
        }
        input = String(result, 6); // Limit result to 6 decimal places
        input = trimTrailingZeros(input); // Remove trailing zeros
        display_number(input);
        operand1 = result;
      }
      
      hasResult = true;
      isEnteringSecondOperand = false;
      isNegative = false;
      hasDecimal = input.indexOf('.') != -1;
    }
  }
}
