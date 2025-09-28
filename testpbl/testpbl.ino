#include <Servo.h>
#include <Stepper.h>

const int stepsPerRevolution = 2048;
Stepper stepper(stepsPerRevolution, 2, 4, 3, 5);

Servo servo1;
Servo servo2;

// Chân cảm biến siêu âm
const int trigPin = 6;
const int echoPin = 7;

long khoangCachBanDau = 0;
bool choPhepNhap = false;

void setup() {
  Serial.begin(9600);

  servo1.attach(9);
  servo2.attach(10);
  stepper.setSpeed(10);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Đóng cửa ban đầu
  servo1.write(90);
  servo2.write(90);

delay(4000); // Đợi cảm biến ổn định kỹ hơn

// Đo trung bình 5 lần để giảm nhiễu
long tong = 0;
for (int i = 0; i < 5; i++) {
  tong += doKhoangCach();
  delay(100);  // chờ một chút giữa các lần đo
}
khoangCachBanDau = tong / 5;

  Serial.print("Khoang cach ban dau: ");
  Serial.print(khoangCachBanDau);
  Serial.println(" cm");
  Serial.println("Dang doi nguoi lai gan hon 5cm...");
}

void loop() {
  long khoangCachHienTai = doKhoangCach();


  // Kiểm tra nếu có vật đến gần hơn 5cm so với ban đầu
  if (!choPhepNhap && (khoangCachBanDau - khoangCachHienTai > 5)) {
    Serial.println("Chup anh");
    choPhepNhap = true;

    while (Serial.available()) Serial.read(); // Xóa dữ liệu cũ từ bộ đệm Serial

    Serial.println("Nhap so (0-3) de mo cua:");
  }

  // Khi được phép nhập và có dữ liệu từ Serial
  if (choPhepNhap && Serial.available() > 0) {
    char input = Serial.read();
    int steps = 0;

    switch (input) {
      case '0': steps = 0; break;
      case '1': steps = 1536; break;
      case '2': steps = 1024; break;
      case '3': steps = 512; break;
      default:
        Serial.println("Chi nhap so tu 0 den 3.");
        return;
    }

    Serial.print("Dang quay "); Serial.print(steps); Serial.println(" buoc...");
    stepper.step(steps);
    Serial.println("Quay xong. Mo cua...");

    servo1.write(0);
    servo2.write(180);
    Serial.println("Cua da mo.");

    delay(3000);

    servo1.write(90);
    servo2.write(90);
    Serial.println("Cua da dong.");
    Serial.println("Hoan tat.\n");

    // Reset trạng thái và đo lại khoảng cách ban đầu
    choPhepNhap = false;
    delay(1000);
    khoangCachBanDau = doKhoangCach();
    Serial.print("Cap nhat khoang cach ban dau: ");
    Serial.print(khoangCachBanDau);
    Serial.println(" cm");
    Serial.println("Dang doi nguoi lai gan hon 5cm...");
  }

  delay(200); // Giảm tần suất đo để tránh nhiễu
}

// Hàm đo khoảng cách bằng cảm biến siêu âm
long doKhoangCach() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // timeout 30ms để tránh treo
  long distance = duration * 0.034 / 2;

  if (distance == 0 || distance > 400) {
    // nếu không có vật hoặc vật quá xa thì trả về khoảng cách lớn
    return 999;
  }

  return distance;
}
