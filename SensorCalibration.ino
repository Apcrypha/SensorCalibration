#define TIA_address 0x90


void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}

void changeGain(){
/*
- TIA_address
- wait for ACK
- 0x01 = Targets the LOCK register
- wait for ACK
- 0x00 = unlock register
- wait for ACK
- Stop condition

- TIA_address
- wait for ACK
- 0x10 = Targets the TIACN register
- wait for ACK
- 0x-- = Specify the gain. Refer to the datasheet
- wait for ACK
- Stop condition

*/

}

