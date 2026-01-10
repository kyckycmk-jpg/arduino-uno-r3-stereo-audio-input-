// ===== ステレオ高速版（交互読み・左右16kHz） =====
// 左: A4, 右: A5
// LED: A0, A1(元A2の役割), A3
// A2: 捨て読み専用（1.25〜1.67V入力）
// 出力: D7, D5, D2, D3

// ---- 左右別しきい値 ----
const int thresholdAL = 515;   // L SMALL
const int thresholdBL = 521;   // L BIG

const int thresholdAR = 513;   // R SMALL
const int thresholdBR = 519;   // R BIG

volatile uint16_t valL = 0;
volatile uint16_t valR = 0;
volatile uint16_t valA2L = 0;
volatile uint16_t valA2R = 0;

volatile bool lr = false;  // false = L, true = R

// ===== A2 捨て読み 1 回 → 本読み =====
inline uint16_t readStableADC(uint8_t mux, uint16_t* a2store) {

  // --- 捨て読み1回（A2） ---
  ADMUX = _BV(REFS0) | 2;
  ADCSRA |= _BV(ADSC);
  while (ADCSRA & _BV(ADSC));
  *a2store = ADC;   // A2 の値を保存（L/R別）

  // --- 本読み（A4 or A5） ---
  ADMUX = _BV(REFS0) | mux;
  ADCSRA |= _BV(ADSC);
  while (ADCSRA & _BV(ADSC));

  return ADC;
}

ISR(TIMER1_COMPA_vect) {

  if (!lr) {
    // ===== 左 =====
    valL = readStableADC(4, &valA2L);  // A4 + A2捨て読み1回

    if (valL < thresholdAL) {
      PORTD &= ~(_BV(7) | _BV(5));
      PORTC &= ~(_BV(1) | _BV(0));
    } else if (valL < thresholdBL) {
      PORTD |=  _BV(7);
      PORTD &= ~_BV(5);
      PORTC |=  _BV(1);
      PORTC &= ~_BV(0);
    } else {
      PORTD &= ~_BV(7);
      PORTD |=  _BV(5);
      PORTC &= ~_BV(1);
      PORTC |=  _BV(0);
    }

  } else {
    // ===== 右 =====
    valR = readStableADC(5, &valA2R);  // A5 + A2捨て読み1回

    if (valR < thresholdAR) {
      PORTD &= ~(_BV(2) | _BV(3));
      PORTC &= ~(_BV(3) | _BV(1));
    } else if (valR < thresholdBR) {
      PORTD |=  _BV(2);
      PORTD &= ~_BV(3);
      PORTC |=  _BV(1);
      PORTC &= ~_BV(3);
    } else {
      PORTD &= ~_BV(2);
      PORTD |=  _BV(3);
      PORTC &= ~_BV(1);
      PORTC |=  _BV(3);
    }
  }

  lr = !lr;
}

void setup() {
  Serial.begin(115200);

  DDRD |= _BV(7) | _BV(5) | _BV(2) | _BV(3);
  DDRC |= _BV(0) | _BV(1) | _BV(3);

  ADCSRA = _BV(ADEN) | _BV(ADPS2);
  ADCSRB = 0;

  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 62;  // 32kHz

  TCCR1B |= _BV(WGM12);
  TCCR1B |= _BV(CS11);
  TIMSK1 |= _BV(OCIE1A);
  interrupts();
}

void loop() {
  // ---- 最速シリアルプロッタ出力 ----
  char buf[60];
  int n = sprintf(buf, "L:%u R:%u A2L:%u A2R:%u\n",
                  valL, valR, valA2L, valA2R);
  Serial.write(buf, n);
}