const long short_pulse_ms = 330;
const long long_pulse_ms = 1050;
const long sld_delay_ms = 400;
const long ssd_delay_ms = 1120;
const long lld_delay_ms = 390;
const long lsd_delay_ms = 1120;
char signal_pattern[] = {'s', 's', 'l', 'l', 's', 's', 's', 'l',
                         's', 'l', 'l', 's', 'l', 's', 's', 's',
                         'l', 's', 'X'
                        };
const long BURST_COUNT = 49;
const long BURST_DELAY_ms = 6500;
const long SQNCE_END_ms = 47640;
const int SWITCHPIN = 10;
const int DATAPIN = 8;
long output_data[((sizeof(signal_pattern) / sizeof(signal_pattern[0])) * 2) - 2];
int output_data_counter = 0;
int signal_pattern_size = (sizeof(signal_pattern) / sizeof(signal_pattern[0]));
void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  pinMode(SWITCHPIN, INPUT_PULLUP);
  pinMode(DATAPIN, OUTPUT);


  Serial.println("\nComputing data from signal pattern\n");
  Serial.println("Total pulses in pattern: " + String(signal_pattern_size));
  char prev_pulse = 'n';
  long sleep = 0;

  // Iterate through the signal_pattern structure.
  // Get each pulse in order and store it in prev_pulse
  for (int count = 0; count < signal_pattern_size; count++) {
    char pulse_type = signal_pattern[count];
    Serial.println(String(count) + ". Pulse type: " + String(pulse_type));
    if (prev_pulse == 'n') {
      prev_pulse = pulse_type;
      Serial.println("This is the initial pulse, it requires special handling in the loop:");
    }
    else if (prev_pulse == 's' and pulse_type == 'l') {
      Serial.println("Intra-pulse delay for Short->Long pulses: "  + String(sld_delay_ms));
      sleep = sld_delay_ms;
    }
    else if (prev_pulse == 's' and pulse_type == 's') {
      Serial.println("Intra-pulse delay for Short->Short pulses: "  + String(ssd_delay_ms));
      sleep = ssd_delay_ms;
    }
    else if (prev_pulse == 'l' and pulse_type == 'l') {
      Serial.println("Intra-pulse delay for Long->Long pulses: "  + String(lld_delay_ms));
      sleep = lld_delay_ms;
    }
    else if (prev_pulse == 'l' and pulse_type == 's') {
      Serial.println("Intra-pulse delay for Long->Short pulses: "  + String(lsd_delay_ms));
      sleep = lsd_delay_ms;
    }
    else if (pulse_type == 'X') { // Burst delay
      Serial.println("Intra-burst delay before next sequence: " + String(BURST_DELAY_ms));
      sleep = BURST_DELAY_ms;
    }

    if (sleep > 0) {  // Not the first pulse
      long currPulseLen;
      if (pulse_type == 's') {
        currPulseLen = short_pulse_ms;
      }
      else if (pulse_type == 'l') {
        currPulseLen = long_pulse_ms;
      }
      else {
        currPulseLen = 0;
      }

      if (prev_pulse == 's' and pulse_type != 'X') {
        Serial.println("Output sleep for " + String(sleep) + " microseconds");
        output_data[output_data_counter++] = sleep;

        Serial.println("Output " + String(pulse_type) + " pulse for " + String(currPulseLen) + " microseconds");
        output_data[output_data_counter++] = currPulseLen;
      }
      else if (prev_pulse == 'l' and pulse_type != 'X') {
        Serial.println("Output sleep for " + String(sleep) + " microseconds");
        output_data[output_data_counter++] = sleep;
        Serial.println("Output long pulse for " + String(currPulseLen) + " microseconds");
        output_data[output_data_counter++] = currPulseLen;
      }
      else if (pulse_type == 'X') {
        Serial.println("Output BURST_DELAY of " + String(BURST_DELAY_ms) + " microseconds");
        output_data[output_data_counter] = BURST_DELAY_ms;
        break;
      }
    }
    else if (prev_pulse == pulse_type) { // First pulse.
      if (prev_pulse == 's') {
        Serial.println("Output initial short pulse for " + String(short_pulse_ms) + " microseconds");
        output_data[output_data_counter++] = short_pulse_ms;
      }
      else if (prev_pulse == 'l') {
        Serial.println("Output initial long pulse for " + String(long_pulse_ms) + " microseconds");
        output_data[output_data_counter++] = long_pulse_ms;
      }
    }
    prev_pulse = pulse_type;
  }

  Serial.println("Dry run of output sequence follows.  output_data array length is " + String((sizeof(output_data) / sizeof(output_data[0]))));
  int outputDataSize = (sizeof(output_data) / sizeof(output_data[0]));
  bool isData = true;
  for (int count = 0; count < outputDataSize; count++) {
    if (isData) {
      long opdms = output_data[count];
      char opdtype;
      if (opdms == short_pulse_ms) {
        opdtype = 'S';
      }
      else if (opdms == long_pulse_ms) {
        opdtype = 'L';
      }
      else if (opdms == BURST_DELAY_ms) {
        opdtype = 'B';
      }

      Serial.println(String(count) + ". ON for " + String(opdms) + " microseconds (pulse type " + String(opdtype) + "), then OFF");
      isData = false;
    }
    else {
      Serial.println(String(count) + ". Intra-pulse sleep for " + String(output_data[count]) + " microseconds");
      isData = true;
    }
  }
}

bool sleeping = false;
void loop() {
  if (digitalRead(SWITCHPIN) == HIGH) {
    if (!sleeping) {
      Serial.println("Sleeping...");
      sleeping = true;
    }
  }
  else if (digitalRead(SWITCHPIN) == LOW) {
    sleeping = false;
    Serial.println("Transmitting...");
    int outputDataSize = (sizeof(output_data) / sizeof(output_data[0]));
    bool isData = true;
    for (int bursts = 0; bursts < BURST_COUNT; bursts++) {
      for (int count = 0; count <= outputDataSize; count++) {
        if (isData) {
          digitalWrite(DATAPIN, 1);
          delayMicroseconds(output_data[count]);
          digitalWrite(DATAPIN, 0);
          isData = false;
        }
        else {
          delayMicroseconds(output_data[count]);
          isData = true;
        }
      }
       delayMicroseconds(BURST_DELAY_ms);
    }
    Serial.println("Going back to sleep");
  }

}
