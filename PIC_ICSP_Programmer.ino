
#define MCLR 11
#define ICSPDAT 10
#define ICSPCLK 9

#define TENTS 10
#define TENTH 300

#define TCKH 1
#define TCKL 1

#define TDLY 2
#define TERAB_MS 15
#define TERAR_MS 3
#define PINT_MS 6

#define NUM_LATCHES 32

void sendByte(byte b) {
  for (int i = 7; i >= 0; i--) {
    byte bit = bitRead(b, i);
    digitalWrite(ICSPCLK, HIGH);
    if (bit == 1) digitalWrite(ICSPDAT, HIGH);
    else digitalWrite(ICSPDAT, LOW);
    delayMicroseconds(TCKH);
    digitalWrite(ICSPCLK, LOW);
    delayMicroseconds(TCKL);
  }
}

void sendCommand(byte command, uint16_t payload, int time) {
  sendByte(command);
  delayMicroseconds(time);

  // start and padding bits
  for (int i = 6; i >= 0; i--) {
    byte bit = bitRead(payload, i);
    digitalWrite(ICSPCLK, HIGH);
    digitalWrite(ICSPDAT, LOW);
    delayMicroseconds(TCKH);
    digitalWrite(ICSPCLK, LOW);
    delayMicroseconds(TCKL);
  }

  // payload  bits
  for (int i = 15; i >= 0; i--) {
    byte bit = bitRead(payload, i);
    digitalWrite(ICSPCLK, HIGH);
    if (bit == 1) digitalWrite(ICSPDAT, HIGH);
    else digitalWrite(ICSPDAT, LOW);
    delayMicroseconds(TCKH);
    digitalWrite(ICSPCLK, LOW);
    delayMicroseconds(TCKL);
  }

  // stop bit
  {
    digitalWrite(ICSPCLK, HIGH);
    digitalWrite(ICSPDAT, LOW);
    delayMicroseconds(TCKH);
    digitalWrite(ICSPCLK, LOW);
    delayMicroseconds(TCKL);
  }
  
  delayMicroseconds(time);
}

void loadPCAddress(uint16_t address) {
  sendCommand(0x80, address, TDLY);
}

void incPCAddress(){
  sendByte(0xF8);
  delayMicroseconds(TDLY);
}

void bulkErase() {
  sendByte(0x18);
  delay(TERAB_MS);
}

void rowErase(){
  sendByte(0xF0);
  delay(TERAR_MS);
}

void writeDataAndAdvance(uint16_t data) {
  sendCommand(0x02, data, TDLY);
}

void writeData(uint16_t data) {
  sendCommand(0x00, data, TDLY);
}

uint16_t readDataAndAdvance() {
  sendByte(0xFE);
  pinMode(ICSPDAT, INPUT);
  delayMicroseconds(TDLY);

  uint16_t result = 0x00;
  
  // Start and padding bits
  for (int i = 6; i >= 0; i--) {
    digitalWrite(ICSPCLK, HIGH);
    delayMicroseconds(TCKH);
    digitalWrite(ICSPCLK, LOW);
    delayMicroseconds(TCKL);
  }

  // Actual good bits
  for (int i = 15; i >= 0; i--) {
    digitalWrite(ICSPCLK, HIGH);
    delayMicroseconds(TCKH);
    
    result <<= 1;
    int val = digitalRead(ICSPDAT);
    if(val == HIGH) result |= 1;

    digitalWrite(ICSPCLK, LOW);
    delayMicroseconds(TCKL);
  }

  // Stop bit
  {
    digitalWrite(ICSPCLK, HIGH);
    delayMicroseconds(TCKH);
    digitalWrite(ICSPCLK, LOW);
    delayMicroseconds(TCKL);
  }

  delayMicroseconds(TDLY);
  pinMode(ICSPDAT, OUTPUT);

  return result;
}

uint16_t readData() {
  sendByte(0xFC);
  pinMode(ICSPDAT, INPUT);
  delayMicroseconds(TDLY);

  uint16_t result = 0x00;
  
  // Start and padding bits
  for (int i = 6; i >= 0; i--) {
    digitalWrite(ICSPCLK, HIGH);
    delayMicroseconds(TCKH);
    digitalWrite(ICSPCLK, LOW);
    delayMicroseconds(TCKL);
  }

  // Actual good bits
  for (int i = 15; i >= 0; i--) {
    digitalWrite(ICSPCLK, HIGH);
    delayMicroseconds(TCKH);
    
    result <<= 1;
    int val = digitalRead(ICSPDAT);
    if(val == HIGH) result |= 1;

    digitalWrite(ICSPCLK, LOW);
    delayMicroseconds(TCKL);
  }

  // Stop bit
  {
    digitalWrite(ICSPCLK, HIGH);
    delayMicroseconds(TCKH);
    digitalWrite(ICSPCLK, LOW);
    delayMicroseconds(TCKL);
  }

  delayMicroseconds(TDLY);
  pinMode(ICSPDAT, OUTPUT);

  return result;
}

void internallyTimedProgramming() {
  sendByte(0xE0);
  delay(PINT_MS);
}


void setup() {
  // Setup stuff up
  pinMode(MCLR, OUTPUT);
  pinMode(ICSPDAT, OUTPUT);
  pinMode(ICSPCLK, OUTPUT);

  digitalWrite(MCLR, HIGH);

  pinMode(13, OUTPUT);

  Serial.begin(115200);
  Serial.println("READY");
}

void loop() {
  char c;
  size_t num = Serial.readBytes(&c, 1);
  if (num == 0) return;
  switch(c){
    case 's':
    case 'S': {
      // Actually doing stuff
      digitalWrite(MCLR, HIGH);
      delayMicroseconds(TENTS);
      digitalWrite(MCLR, LOW);
      delayMicroseconds(TENTH);

      sendByte(0x4D);
      sendByte(0x43);
      sendByte(0x48);
      sendByte(0x50);

      delayMicroseconds(TENTH);

      digitalWrite(13, HIGH);

      Serial.println("ok");
    } break;
    case 'e':
    case 'E': {
      bulkErase();
      Serial.println("ok");
    } break;
    case 'r':
    case 'R': {
      char t;
      Serial.readBytes(&t, 1);
      uint16_t data = 1 << 15;
      if((t == 'A') || (t == 'a')){
        data = readDataAndAdvance();
      }else{
        data = readData();
      }
      Serial.println(data, HEX);
    } break;
    case 'b':
    case 'B': {
      String s = Serial.readStringUntil('.');
      uint16_t address = (uint16_t)strtoul(s.c_str(), 0, 0);
      uint16_t row[NUM_LATCHES] = {};
      
      for (int i=0; i<NUM_LATCHES; i++){
        String s = Serial.readStringUntil(',');
        uint16_t data = (uint16_t)strtoul(s.c_str(), 0, 0);
        row[i] = data;
      }

      // only the last 5 bits of the address are actually used when writing to latches
      loadPCAddress(0x0000);
      for (int i=0; i<NUM_LATCHES; i++){
        writeDataAndAdvance(row[i]);
        delay(1);
      }

      loadPCAddress(address & 0xffe0);
      internallyTimedProgramming();
      
      loadPCAddress(address & 0xffe0);
      for (int i=0; i<NUM_LATCHES; i++){
        uint16_t data = readDataAndAdvance();
        if (data != row[i]) {
          Serial.println("failure");
        }
      }

      Serial.println("ok");
    } break;
    case 'c':
    case 'C': {
      uint16_t configs[5] = {};
      
      for (int i=0; i<5; i++){
        String s = Serial.readStringUntil(',');
        uint16_t data = (uint16_t)strtoul(s.c_str(), 0, 0);
        configs[i] = data;
      }

      loadPCAddress(0x8007);
      for (int i=0; i<5; i++){
        writeData(configs[i]);
        internallyTimedProgramming();
        incPCAddress();
      }
      loadPCAddress(0x8007);
      for (int i=0; i<5; i++){
        uint16_t data = readDataAndAdvance();
        if (data != configs[i]) {
          Serial.println("failure");
        }
      }

      Serial.println("ok");
    } break;
    case 'w':
    case 'W': {
      String s = Serial.readStringUntil('\n');
      if((s.charAt(0) == 'A') || (s.charAt(0) == 'a')){
        s.remove(0, 1);
        uint16_t data = (uint16_t)strtoul(s.c_str(), 0, 0);
        writeDataAndAdvance(data);
      }else{
        uint16_t data = (uint16_t)strtoul(s.c_str(), 0, 0);
        writeData(data);
      }
      Serial.println("ok");
    } break;
    case 'p':
    case 'P': {
      internallyTimedProgramming();
      Serial.println("ok");
    }break;
    case 'g':
    case 'G': {
      String s = Serial.readStringUntil('\n');
      uint16_t address = (uint16_t)strtoul(s.c_str(), 0, 0);
      loadPCAddress(address);
      Serial.println("ok");
    }break;
    case 'f':
    case 'F': {
      digitalWrite(MCLR, HIGH);
      digitalWrite(13, LOW);
      Serial.println("ok");
    }break;
    case 'l':
    case 'L': {
      rowErase();
      Serial.println("ok");
    }break;
    case 'i':
    case 'I': {
      incPCAddress();
      Serial.println("ok");
    }break;
  }
}
