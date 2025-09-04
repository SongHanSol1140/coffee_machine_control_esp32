/*
	MCP23017 I2C Expander 연결
	
  라이브러리 설치
  Adafruit Busio + Adafruit MCP23017

	A0 A1 A2 Switch 설정 (보드 하드웨어에서 스위치 내려서 수정)
	0  0  0    0x20 - Adafruit_MCP23X17 mcp1;
	1  0  0    0x21 - Adafruit_MCP23X17 mcp2;
	0  1  0    0x22 - Adafruit_MCP23X17 mcp3;
	1  1  0    0x23 - Adafruit_MCP23X17 mcp4;
	0  0  1    0x24 - Adafruit_MCP23X17 mcp5;
	1  0  1    0x25 - Adafruit_MCP23X17 mcp6;
	1  1  0    0x26 - Adafruit_MCP23X17 mcp7;
	1  1  1    0x27 - Adafruit_MCP23X17 mcp8;
	
	MCP23017 GPIO I2C Expander
	SDA / SCL 핀
		ESP32 GPIO 21 (SDA) → MCP23017의 SDA
		ESP32 GPIO 22 (SCL) → MCP23017의 SCL

	MCP23017 Address못찾을시 .ino에 업로드
	만약 찾을수 없다면 배선에 문제가 있을 확률이 높습니다.
	// #include <Wire.h>

	// void setup() {
	//   Wire.begin(21, 22); // MCP23017_SDA, MCP23017_SCL 핀 번호
	//   Serial.begin(115200);
	//   while (!Serial);
	//   Serial.println("\nI2C Scanner");
	// }

	// void loop() {
	//   byte error, address;
	//   int nDevices;
	//   Serial.println("Scanning...");
	//   nDevices = 0;
	//   for (address = 1; address < 127; address++) {
	//     Wire.beginTransmission(address);
	//     error = Wire.endTransmission();
	//     if (error == 0) {
	//       Serial.print("I2C device found at address 0x");
	//       if (address < 16)
	//         Serial.print("0");
	//       Serial.println(address, HEX);
	//       nDevices++;
	//     } else if (error == 4) {
	//       Serial.print("Unknown error at address 0x");
	//       if (address < 16)
	//         Serial.print("0");
	//       Serial.println(address, HEX);
	//     }
	//   }
	//   if (nDevices == 0)
	//     Serial.println("No I2C devices found\n");
	//   else
	//     Serial.println("done\n");
	//   delay(5000);
	// }
*/




/*
	NTC 온도 센서 연결
	34,35번핀에 NTC 온도 센서 연결예정
	1. 한쪽을 그라운드에 연결하고 (GND)
	2. 한쪽을 빵판한곳에 꽂으세요. (3.3v인데 저항+입력핀으로 멀티드롭 해야함)
	   해당 핀과 ESP32 3.3v pin과 연결하는데, 10k 저항 사용
	   각 입력핀(34,35)에도 연결해주세요. (이쪽엔 저항이 필요없음)
	
	온도센서 #1은 Heater #1 연동
	온도센서 #2는 Heater #2 연동

*/

/*

	ACS712 전류 센서 모듈에는 5A 20A 30A 모듈이 있음
	칩셋을 카메라로 확대하거나, 빛을비춰서 자세하게보면 몇 A인지알 수 있음
	그러나 커넥터 부분에 10A 제한이 있는걸 발견

	긴급 종료용 CT 센서 연결
	27번핀에 CT 센서 연결
	측정값을 암페어로 환산하여 emergencyAmpere값 초과시 emergencyStop

	전압 입력 방향은 초록색 커넥터
	기기 초록색 부분을 정면에 두고 마주봤을때(VCC/OUT/GND문자 핀이 반대에 있음) 좌측이 +(VCC) 우측이 -(GND)
	

	
*/