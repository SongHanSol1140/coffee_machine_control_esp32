/*
	컨트롤러 페이지 접속방법
	wifi.cpp에 고정 IP 설정 후 코드 업로드
	페이지 접속 방법 : http://192.168.0.231/
	// 이미 사용중일 경우 동작 에러날 수 있음
	wifiSetup.cpp 상단
	// 와이파이 정보
	const char* wifi_ssid = "NNX2-2.4G";
	const char* wifi_password = "$@43skshslrtm";
	=>
	const char* wifi_ssid = "nnx-factory 3 2.4G";
	const char* wifi_password = "$@43skshslrtm";
	와 같이 수정(해당 위치에서 잡히는 와이파이명 & 비밀번호로 설정후 코드 업로드)

	연결 안될 시 ESP32 재부팅 후 시리얼 모니터에서
	16:58:17.589 -> Wifi IP: 192.168.0.231
	16:58:17.589 -> WIFI connected!
	와 같은 메세지가 나오는지 확인
*/

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

	MCP23017 Address못찾을시 .ino에 업로드할 코드
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
	34,35번핀에 NTC 온도 센서 연결

	배선 연결 가이드
	1. 한쪽을 그라운드에 연결하고 (GND)
	2. 한쪽을 빵판 한곳에 꽂으세요. (3.3v인데 저항+입력핀으로 멀티드롭 해야함)
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
/*
	유량계 2종류 연결
	1. 출구 유량계 GPIO #18
	2. 혼합탱크 입구 유량계 GPIO #19


*/
/*

	전제조건
		1. isRunning(System ON) true일때만 제조 시작 버튼 동작가능(html과 esp32에서 이중 점검)
		2. isWorking(제조 중) false일때만 제조 시작버튼 동작가능(html과 esp32에서 이중 점검)
		3. 현재 CT 측정 Amepere가 설정한 위험값보다 높아지면 모든 동작 중지(모든 GPIO off)
		4. 긴급 정지 버튼 눌릴시 모든 GPIO off. PWM off
		5. cold 상태라면 제조시 히터를 켜지 않음(Heater Relay, Heater SSR 모두 OFF)
		6. 작업중 코드블로킹이 되어 emergencyStop 또는 currentAmpere > emergencyA 상태가 되지않도록 유의할것
		
	에스프레소 추출(createEspresso)
		1. 웹페이지에서 에스프레소 제조버튼 클릭
		2. Start조건 확인 (isRunning, isWorking)
		3. GPIO Expander #6 ON (순환 3Way Valve)
		4. GPIO 25 ON (Heater Relay)
		5. GPIO 33 PWM ON ON (Heater SSR)
		6. GPIO 32 PWM ON (기어 펌프)
		7. 드레인 시간동안 대기
		8. GPIO Expander #5 ON (출구 3Way Valve)
		9. GPIO#18 유량계 총유량 > 에스프레소 - 에스프레소 설정ml * (히터 정지 유량비율(%) / 100)까지 대기
		10. GPIO 25 OFF (히터 #1)
		11. GPIO 33 PWM OFF(히터 #2)
		12. GPIO#18 유량계 총 유량 > 에스프레소 설정값까지 대기
		13. GPIO 32 PMW OFF(기어펌프)
		14. GPIO Expander #6 OFF (순환 3Way Valve)
		15. GPIO Expander #5 OFF (출구 3Way Valve)
		16. 제조 완료 (isWorking false)

	아메리카노 추출(createAmericano)
		1. 웹페이지에서 아메리카노 제조버튼 클릭
		2. Start조건 확인 (isRunning, isWorking), 제조값 초기화
		3. GPIO Expander #1 ON (에스프레소 출구 바이패스)
		4. GPIO Expander #8 ON (혼합 입구 펌프)
		5. GPIO#19 유량계 총 유량 > 아메리카노 에스프레소 설정값까지 대기
		6. GPIO Expander #8 OFF (혼합 입구 펌프)
		7. GPIO#19 유량계 누적값 초기화
		8. GPIO Expander #10 ON(정수 물 전자변)
		9. GPIO#19 유량계 총 유량 > 아메리카노 물 설정값 비교값까지 대기
		10. GPIO Expander #10 OFF 정지(정수 물 전자변)
		11. GPIO32 PWM ON(기어펌프)
		12. GPIO33 PWM ON된 시간이 설정된 혼합시간과 일치할때까지 대기
		13. GPIO#18 유량계 총 유량(에스프레소 설정값 + 물 설정값)의 10%가 이동할때까지 대기
		13. GPIO25 ON(Heater Relay)
		14. GPIO33 PWM ON(Heater SSR) - PID 제어
		15. 공기 흡입 시작
			15-1. 공기 흡입 시작 대기시간 대기
			15-2. GPIO Expander #7 ON
			15-3. 공기 흡입 ON 시간 대기
			15-4. GPIO Expander #7 OFF
			15-5. 공기 흡입 OFF 시간 대기
			15-6. 공기 흡입시간까지 ON/OFF반복 후 공기 흡입 종료
		16. GPIO Expander #6 ON
		17. 드레인 시간동안 대기
		18. GPIO Expander #5 ON(출구 3Way Valve)
		19. GPIO#18 유량계 총유량 > (아메리카노 에스프레소 설정값 + 아메리카노 물 설정값) * (히터 정지 유량비율(%) / 100)까지 대기
		21. GPIO25 출력 정지 (Heater Relay)
		22. GPIO33 PWM OFF (Heater SSR)
		21. GPIO#18 유량계 총 유량 > 아메리카노 에스프레소 + 물 설정값 까지 대기
		22. GPIO32 PWM출력 OFF (기어펌프)
		23. GPIO Expander #6 OFF (순환 3Way Valve)
		24. GPIO Expander #5 OFF (출구 3Way Valve)
		25. GPIO Expander #1 OFF (에스프레소 출구 바이패스)
		26. 제조 완료 (isWorking false)

	카페라떼 추출
		1. 웹페이지에서 카페라떼 제조버튼 클릭
		2. Start조건 확인 (isRunning, isWorking), 제조값 초기화
		3. GPIO Expander #1 ON (에스프레소 출구 바이패스)
		4. GPIO Expander #8 ON(혼합 입구 펌프)
		5. GPIO#19 유량계 총 유량 > 카페라떼 에스프레소 설정값까지 대기
		6. GPIO Expander #8 OFF(혼합 입구 펌프)
		7. GPIO#19 유량계 누적값 초기화
		8. GPIO Expander #2 ON(우유 전자변)
		9. GPIO Expander #8 On(혼합 입구 펌프)
		10. GPIO#19 유량계 총 유량 > 카페라떼 우유 설정값 비교값까지 대기
		11. GPIO Expander #2 OFF(우유 전자변)
		12. GPIO Expander #8 OFF(혼합 입구 펌프)
		13. GPIO33 PWM ON(기어펌프)
		14. GPIO33 PWM ON된 시간이 설정된 혼합시간과 일치할때까지 대기(순환)
		18. GPIO Expander #6 ON (순환 3Way Valve)
		=> 이게 열려잇어야 내보내는거고 닫혀있어야 순환이 된다.
		15-16.
		    필요한 량의 10%가 이동할때까지 대기 후 히터 ON
		    GPIO25 ON(Heater Relay)
		    GPIO33 PWM ON(Heater SSR) - PID 제어
		17. 공기 흡입 시작
			14-1. 공기 흡입 시작 대기시간 대기
			14-2. GPIO Expander #7 ON
			14-3. 공기 흡입 ON 시간 대기
			14-4. GPIO Expander #7 OFF
			14-5. 공기 흡입 OFF 시간 대기
			14-6. 공기 흡입시간까지 ON/OFF반복 후 공기 흡입 종료
		19. 드레인 시간동안 대기
		20. GPIO Expander #5 ON(출구 3Way Valve)
		21. GPIO#18 유량계 총유량 > (카페라떼 에스프레소 설정값 + 카페라떼 우유 설정값) * (히터 정지 유량비율(%) / 100)까지 대기
		22. GPIO#25 OFF
		23. GPIO#33 PWM OFF
		24. GPIO#18 유량계 총 유량 > 카페라떼 에스프레소 + 우유 설정값 까지 대기
		25. GPIO32 PWM출력 OFF (기어펌프)
		26. GPIO Expander #6 OFF (순환 3Way Valve)
		27. GPIO Expander #5 OFF (출구 3Way Valve)
		28. GPIO Expander #1 OFF (에스프레소 출구 바이패스)
		29. 제조 완료 (isWorking false)


	
	청소
		1. 웹페이지에서 청소버튼 클릭
		2. Start조건 확인 (isRunning, isWorking), 제조값 초기화
		3. GPIO Expander #4 ON(청소 전자변)
		4. GPIO Expander #8 ON(혼합탱크 입구 펌프)
		5. GPIO33 ON(기어펌프)
		6. 청소 시작후 설정된 청소 전환시간 비교후 시간이 일치하면
		7. GPIO Expander #4 OFF(청소 전자변)
		8. GPIO Expander #8 OFF(혼합탱크 입구 펌프)
		9. GPIO Expander #6 ON(혼합 3Way Valve)
		10. GPIO Expander #5 ON(출구 3Way Valve)
		11. 청소 시작후 설정된 청소 전체 시간 비교후 시간이 일치하면
		12. GPIO33 OFF(기어펌프)
		13. GPIO Expander #6 OFF(혼합 3Way Valve)
		14. GPIO Expander #5 OFF(출구 3Way Valve)
		16. 제조 완료 (isWorking false)
*/



/*
	카페라떼 테스트 #1 에스프레소 랑 우유가 동시에열리는데
	우유랑 에스프레소가 동시에열리면 안되지않나
	그런데 로직설명상으로 커피를 키고 우유를 같이키는데,
	이럼 우유랑 물이 동시에 들어온다


*/