const byte numChars = 32;
char receivedChars[numChars];

char pubkey[] = "approved";

boolean newData = false;

void setup() {
    Serial.begin(9600);
}

void loop() {
    recvWithEndMarker();
    if(newData) {
      newData = false;
      if (strcmp(receivedChars, "get_data") == 0) {
        sendPubKey();
      } else {
      sendErr();
      }
    }
}

void recvWithEndMarker() {
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;
    
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0';
            ndx = 0;
            newData = true;
            break;
        }
    }
}

void sendErr() {
  Serial.println("command is not recognized");
}

void sendPubKey() {
  Serial.println(pubkey);
}

void showNewData() {
    if (newData == true) {
        Serial.print("This just in ... ");
        Serial.println(receivedChars);
        newData = false;
    }
}
