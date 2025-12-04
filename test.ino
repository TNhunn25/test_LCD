// #include <Arduino.h>
// #include <Arduino_GFX_Library.h>
// #include <Wire.h>
// #include <GT911.h>

// // ===== MÀU RGB565 =====
// #define BLACK 0x0000
// #define WHITE 0xFFFF
// #define RED   0xF800
// #define GREEN 0x07E0
// #define BLUE  0x001F

// // ===== LCD NV3041A (đang chạy tốt của bạn) =====
// Arduino_DataBus *bus = new Arduino_ESP32QSPI(
//   45, // cs
//   47, // sck
//   21, // d0
//   48, // d1
//   40, // d2
//   39  // d3
// );

// Arduino_GFX *gfx = new Arduino_NV3041A(
//   bus,
//   GFX_NOT_DEFINED,
//   2,        // rotation landscape
//   true
// );

// // ===== Touch GT911 (đúng chân JC4827W543) =====
// #define TOUCH_SCL 4
// #define TOUCH_SDA 8
// #define TOUCH_INT -1     // JC4827W543 không dùng INT
// #define TOUCH_RST -1     // không có chân RST
// #define TOUCH_ADDR 0x5D  // đã xác nhận bằng scanner

// GT911 ts(&Wire);  // dùng Wire I2C

// void setup() {
//   Serial.begin(115200);
//   delay(500);
//   Serial.println();
//   Serial.println("=== JC4827W543 NV3041A + GT911 Touch ===");

//   // ===== LCD =====
//   pinMode(1, OUTPUT);        // BL
//   digitalWrite(1, HIGH);     // bật đèn nền

//   if (!gfx->begin()) {
//     Serial.println("LCD init FAIL!");
//     while (1);
//   }
//   Serial.println("LCD OK");

//   gfx->fillScreen(BLACK);
//   gfx->setTextColor(WHITE);
//   gfx->setTextSize(2);
//   gfx->setCursor(10, 10);
//   gfx->println("Touch Test JC4827W543");
//   gfx->setCursor(10, 30);
//   gfx->println("Cham vao man hinh...");

//   // ===== TOUCH =====
//   Serial.println("Init GT911...");
//   Wire.begin(TOUCH_SDA, TOUCH_SCL);

//   // KHÔNG dùng INT/RST nên truyền -1
//   if (!ts.begin(TOUCH_INT, TOUCH_RST, TOUCH_ADDR, 400000)) {
//     Serial.println("GT911 init FAIL!");
//     gfx->setCursor(10, 50);
//     gfx->setTextColor(RED);
//     gfx->println("Touch init FAIL!");
//   } else {
//     Serial.println("GT911 init OK");
//   }
// }

// void loop() {
//   // đọc touch
//   uint8_t touches = ts.touched(GT911_MODE_POLLING);

//   if (touches > 0) {
//     GTPoint* p = ts.getPoints();

//     for (uint8_t i = 0; i < touches; i++) {
//       uint16_t x = p[i].x;
//       uint16_t y = p[i].y;

//       Serial.print("Touch ");
//       Serial.print(i);
//       Serial.print(": (");
//       Serial.print(x);
//       Serial.print(", ");
//       Serial.println(y);

//       // vẽ vòng tròn xanh
//       gfx->fillCircle(x, y, 10, GREEN);
//     }
//     delay(30);
//   }
// }

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <GT911.h>

// ================== CẤU HÌNH MÀN HÌNH ==================
#define TFT_WIDTH 480
#define TFT_HEIGHT 272
#define TFT_ORIENTATION 2 // landscape,

#define LCD_BL_PIN 1 // BL_CTRL (đèn nền)

// ================== MÀU RGB565 ==================
#define BLACK 0x0000
#define WHITE 0xFFFF
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F

// ================== TOUCH GT911 ==================
// ĐÃ SCAN: I2C addr = 0x5D, SDA=8, SCL=4
#define TOUCH_SCL    4
#define TOUCH_SDA    8
#define TOUCH_INT   -1           // board này không đưa INT ra
#define TOUCH_RST   -1           // không có RST riêng
#define TOUCH_ADDR   0x5D

// ==== GIỚI HẠN RAW (tạm đoán, sẽ tinh chỉnh dựa trên log) ====
// Sau khi xem log Raw(X,Y) bạn sửa 4 giá trị này cho khớp.
#define RAW_X_MIN 0
#define RAW_X_MAX 960 // nếu thấy luôn >400 thì có thể đổi thành 480
#define RAW_Y_MIN 0
#define RAW_Y_MAX 544

// Mapping hướng (đã sửa để KHÔNG lật X nữa)
#define TOUCH_SWAP_XY 1  // 1 = đổi chỗ X/Y, 0 = giữ nguyên
#define TOUCH_INVERT_X 0 // 0 = không lật X (trái vẫn là trái)
#define TOUCH_INVERT_Y 1 // 1 = lật trục Y

// ================== LCD NV3041A dùng QSPI ==================
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    45, // CS
    47, // SCK
    21, // D0
    48, // D1
    40, // D2
    39  // D3
);

Arduino_GFX *gfx = new Arduino_NV3041A(
    bus,
    GFX_NOT_DEFINED, // RST (dùng chung trên board)
    TFT_ORIENTATION,
    true // IPS
);

// ================== CẢM ỨNG ==================
GT911 ts(&Wire);

// Hàm scale + map toạ độ touch về hệ toạ độ LCD
void transform_touch(uint16_t raw_x, uint16_t raw_y,
                     uint16_t &x, uint16_t &y)
{
    // --- 1. SCALE theo khoảng min/max ---
    // (dùng long để tránh tràn, không dùng float)
    long rx = raw_x;
    long ry = raw_y;

    if (rx < RAW_X_MIN)
        rx = RAW_X_MIN;
    if (rx > RAW_X_MAX)
        rx = RAW_X_MAX;
    if (ry < RAW_Y_MIN)
        ry = RAW_Y_MIN;
    if (ry > RAW_Y_MAX)
        ry = RAW_Y_MAX;

    long sx = (rx - RAW_X_MIN) * (TFT_WIDTH - 1) / (RAW_X_MAX - RAW_X_MIN);
    long sy = (ry - RAW_Y_MIN) * (TFT_HEIGHT - 1) / (RAW_Y_MAX - RAW_Y_MIN);

    x = (uint16_t)sx;
    y = (uint16_t)sy;

    // --- 2. XOAY / LẬT THEO CỜ CONFIG ---
#if TOUCH_SWAP_XY
    uint16_t tmp = x;
    x = y;
    y = tmp;
#endif

#if TOUCH_INVERT_X
    x = TFT_WIDTH - 1 - x;
#endif

#if TOUCH_INVERT_Y
    y = TFT_HEIGHT - 1 - y;
#endif

    // --- 3. KẸP BIÊN ---
    if (x >= TFT_WIDTH)
        x = TFT_WIDTH - 1;
    if (y >= TFT_HEIGHT)
        y = TFT_HEIGHT - 1;
}

// ================== SETUP ==================
void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println("=== JC4827W543 NV3041A + GT911 Touch ===");

    // ---- LCD ----
    pinMode(LCD_BL_PIN, OUTPUT);
    digitalWrite(LCD_BL_PIN, HIGH); // bật đèn nền

    Serial.println("Init LCD...");
    if (!gfx->begin())
    {
        Serial.println("gfx->begin() FAILED!");
        while (1)
            delay(1000);
    }
    Serial.println("LCD OK");

    gfx->fillScreen(BLACK);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(10, 10);
    gfx->println("Touch Test JC4827W543");
    gfx->setCursor(10, 30);
    gfx->println("Cham vao man hinh de test");

    // ---- I2C + Touch ----
    Serial.println("Init I2C for GT911...");
    Wire.begin(TOUCH_SDA, TOUCH_SCL);

    Serial.println("Init GT911...");
    if (!ts.begin(TOUCH_INT, TOUCH_RST, TOUCH_ADDR, 400000))
    {
        Serial.println("GT911 init FAIL!");
        gfx->setCursor(10, 50);
        gfx->setTextColor(RED);
        gfx->println("Touch init FAIL! Xem Serial");
    }
    else
    {
        Serial.println("GT911 init OK");
    }
}

// ================== LOOP ==================
void loop()
{
    // đọc số điểm chạm theo chế độ polling
    uint8_t touches = ts.touched(GT911_MODE_POLLING);

    if (touches)
    {
        GTPoint *pts = ts.getPoints();

        for (uint8_t i = 0; i < touches; i++)
        {
            uint16_t raw_x = pts[i].x;
            uint16_t raw_y = pts[i].y;

            uint16_t x, y;
            transform_touch(raw_x, raw_y, x, y);

            // log raw & map để bạn dễ debug nếu cần
            Serial.print("Raw(");
            Serial.print(raw_x);
            Serial.print(",");
            Serial.print(raw_y);
            Serial.print(") -> Map(");
            Serial.print(x);
            Serial.print(",");
            Serial.println(y);

            gfx->fillCircle(x, y, 10, GREEN); // vẽ chấm xanh tại vị trí chạm
        }

        delay(30); // nhỏ thôi cho cảm giác mượt
    }
}