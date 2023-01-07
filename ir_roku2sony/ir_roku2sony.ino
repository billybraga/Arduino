/*
Convert roku volume change signal to sony stereo system remote
*/

#include <IRremote.h>

#define NO_IR 0
#define IR_SEND_PIN 1
#define IR_RECEIVE_PIN 0

#define REPEAT_UNKNOWN 1

void setup() {
  Serial.begin(9600);

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  Serial.print(F("Ready to receive IR signals of protocols: "));
  printActiveIRProtocols(&Serial);

  Serial.print("at pin ");
  Serial.println(IR_RECEIVE_PIN);

  IrSender.begin(IR_SEND_PIN, 0, USE_DEFAULT_FEEDBACK_LED_PIN);  // Specify send pin and enable feedback LED at default feedback LED pin
  Serial.print("Ready to send IR signals at pin ");
  Serial.print(IR_SEND_PIN);
  Serial.println(" on press of button at pin ");
}

void printIrState(IRData* irDecode) {
  Serial.print("{\"ir_command\": 0x");
  Serial.print(irDecode->command, HEX);
  Serial.print(", \"ir_protocol\": ");

  switch (irDecode->protocol) {
    case SONY:
      Serial.print("SONY");
      break;
    default:
      Serial.print(irDecode->protocol);
      break;
  }

  Serial.print(", \"numberOfBits\":");
  Serial.print(irDecode->numberOfBits);
  Serial.println("}");
}

void debugIrReceiver() {
  Serial.println();  // 2 blank lines between entries
  Serial.println();
  IrReceiver.printIRResultShort(&Serial);
  Serial.println();
  IrReceiver.printIRSendUsage(&Serial);
  Serial.println();
  Serial.println(F("Raw result in internal ticks (50 us) - with leading gap"));
  IrReceiver.printIRResultRawFormatted(&Serial, false);  // Output the results in RAW format
  Serial.println(F("Raw result in microseconds - with leading gap"));
  IrReceiver.printIRResultRawFormatted(&Serial, true);  // Output the results in RAW format
  Serial.println();                                     // blank line between entries
  Serial.print(F("Result as internal ticks (50 us) array - compensated with MARK_EXCESS_MICROS="));
  Serial.println(MARK_EXCESS_MICROS);
  IrReceiver.compensateAndPrintIRResultAsCArray(&Serial, false);  // Output the results as uint8_t source code array of ticks
  Serial.print(F("Result as microseconds array - compensated with MARK_EXCESS_MICROS="));
  Serial.println(MARK_EXCESS_MICROS);
  IrReceiver.compensateAndPrintIRResultAsCArray(&Serial, true);  // Output the results as uint16_t source code array of micros
  IrReceiver.printIRResultAsCVariables(&Serial);                 // Output address and data as source code variables

  IrReceiver.compensateAndPrintIRResultAsPronto(&Serial);
}

IRData* getIr() {
  IRData* value = NULL;

  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) {
      Serial.println(F("Overflow detected"));
      Serial.print("Try to increase the \"RAW_BUFFER_LENGTH\" value of ");
      Serial.print(RAW_BUFFER_LENGTH);
      Serial.print(" in ");
      Serial.println(__FILE__);
      // see also https://github.com/Arduino-IRremote/Arduino-IRremote#compile-options--macros-for-this-library
    } else {

      if (!IrReceiver.decodedIRData.command)
        Serial.println("Empty command");
      else if (IrReceiver.decodedIRData.command == REPEAT)
        Serial.println("repeat command");
      else {
        if (false) {
          debugIrReceiver();
        }

        printIrState(&IrReceiver.decodedIRData);
        value = &IrReceiver.decodedIRData;
      }
    }

    IrReceiver.resume();
  }

  return value;
}

void sendIrCommands(int_fast8_t count, uint32_t code, decode_type_t protocol) {
  if (code == NO_IR)
    return;

  IrReceiver.stop();

  Serial.print("{\"action\":\"send_ir_data\", \"code\": 0x");
  Serial.print(code, HEX);
  Serial.print(", \"count\": ");
  Serial.print(count);
  Serial.print(", \"protocol\": ");

  switch (protocol) {
    case SONY:
      Serial.print("SONY");
      IrSender.sendSony(0x10, code, count);
      break;
    case NEC:
      Serial.print("NEC");
      IrSender.sendNEC(0xC7EA, code, count);
      break;
    default:
      Serial.print(protocol);
      break;
  }

  Serial.println("}");

  delay(100);

  IrReceiver.start();
}

void sendIrRaw(IRData* data) {
  IrReceiver.stop();
  
  printIrState(data);
  IrSender.write(data);

  delay(100);

  IrReceiver.start();
}

void transfer() {
  IRData* ir = getIr();
  delay(1000);
  int count = 1;

  if (ir != NULL && ir->command != NO_IR) {
    switch (ir->command) {
      // roku vol up
      case 0x8F:
        count = 2;
      case 0xF:
        // sony stereo vol up
        sendIrCommands(count, 0x12, SONY);
        break;
      // roku vol down
      case 0x90:
        count = 2;
      case 0x10:
        // sony stereo vol down
        sendIrCommands(count, 0x13, SONY);
        break;
      default:
        Serial.println("Unknown code");
        if (REPEAT_UNKNOWN == 1) {
          Serial.print("Forwarding code: ");
          sendIrRaw(ir);
        }
        return;
    }
  }
}

void sendTest() {
  //sendIrCommands(1, 0x13, SONY);

  sendIrCommands(1, 0x10, NEC);

  delay(1000);
}

void selfReadTest() {
  IrSender.sendNEC(0xC7EA, 0x10, 1);

  if (IrReceiver.decode()) {
      debugIrReceiver();
      IrReceiver.resume();
  }

  delay(5000);
}

void loop() {
  transfer();
}
