#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(32, 16, 20);

const int BTN_HIT      = 2;
const int BTN_STAND    = 3;
const int BTN_BET_DOWN = 4;
const int BTN_BET_UP   = 5;
const int BTN_ALLIN    = 9;

const int LED_R = 6;
const int LED_G = 7;
const int LED_B = 8;

enum GameState { BETTING, PLAYING };
GameState state = BETTING;

int balance = 1000;
int bet = 0;
int deckIndex = 0;

String deck[] = {
  "A","2","3","4","5","6","7","8","9","10","J","Q","K",
  "A","2","3","4","5","6","7","8","9","10","J","Q","K",
  "A","2","3","4","5","6","7","8","9","10","J","Q","K",
  "A","2","3","4","5","6","7","8","9","10","J","Q","K"
};

String playerHand[12];
String dealerHand[12];
int playerCards = 0;
int dealerCards = 0;

bool dealerReveal = false;

unsigned long debounceDelay = 200;

void setup() {
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);

  pinMode(BTN_HIT, INPUT_PULLUP);
  pinMode(BTN_STAND, INPUT_PULLUP);
  pinMode(BTN_BET_UP, INPUT_PULLUP);
  pinMode(BTN_BET_DOWN, INPUT_PULLUP);
  pinMode(BTN_ALLIN, INPUT_PULLUP);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  clearLED();

  randomSeed(analogRead(0));
  shuffleDeck();
  updateDisplay();
}

void loop() {
  switch (state) {
    case BETTING: handleBetting(); break;
    case PLAYING: handleGame(); break;
  }
}

void handleBetting() {
  clearLED();
  if (balance <= 0) {
    lcd.clear(); lcd.print("Game over");
    lcd.setCursor(0, 1); lcd.print("Hit to restart");
    while (true) {
      if (isButtonPressed(BTN_HIT)) {
        balance = 1000;
        updateDisplay();
        break;
      }
    }
  }

  if (isButtonPressed(BTN_BET_UP)) { bet = min(bet + 25, balance); updateDisplay(); }
  else if (isButtonPressed(BTN_BET_DOWN)) { bet = max(bet - 25, 0); updateDisplay(); }
  else if (isButtonPressed(BTN_ALLIN)) { bet = balance; updateDisplay(); }
  else if (isButtonPressed(BTN_HIT) && bet > 0) {
    startRound();
    state = PLAYING;
    updateDisplay();
  }
}

void handleGame() {
  if (isButtonPressed(BTN_HIT)) {
    playerHand[playerCards++] = drawCard();
    printHands();
    if (calculateHand(playerHand, playerCards) > 21) {
      checkResult();
      delay(2000);
      state = BETTING;
      updateDisplay();
    } else {
      updateDisplay();
    }
  } else if (isButtonPressed(BTN_STAND)) {
    dealerReveal = true; 
    dealerTurn();
    checkResult();
    delay(2000);
    dealerReveal = false;
    state = BETTING;
    updateDisplay();
  }
}

void startRound() {
  playerCards = dealerCards = 0;
  playerHand[playerCards++] = drawCard();
  playerHand[playerCards++] = drawCard();
  dealerHand[dealerCards++] = drawCard();
  dealerHand[dealerCards++] = drawCard();
  dealerReveal = false; 
  printHands();
}

void checkResult() {
  int p = calculateHand(playerHand, playerCards);
  int d = calculateHand(dealerHand, dealerCards);

  if (p > 21) {
    balance -= bet;
    setLED(255, 0, 0);
  } else if (d > 21 || p > d) {
    balance += bet;
    setLED(0, 255, 0);
  } else if (p == d) {
    setLED(255, 255, 0);
  } else {
    balance -= bet;
    setLED(255, 0, 0);
  }

  bet = 0;
}

void dealerTurn() {
  updateDisplay(); 
  delay(1000);
  while (calculateHand(dealerHand, dealerCards) < 17) {
    dealerHand[dealerCards++] = drawCard();
    printHands();
    updateDisplay();
    delay(1000);
  }
}

void shuffleDeck() {
  for (int i = 0; i < 52; i++) {
    int r = random(0, 52);
    String temp = deck[i];
    deck[i] = deck[r];
    deck[r] = temp;
  }
  deckIndex = 0;
}

String drawCard() {
  if (deckIndex >= 52) shuffleDeck();
  return deck[deckIndex++];
}

int cardValue(String card) {
  if (card == "A") return 11;
  if (card == "K" || card == "Q" || card == "J") return 10;
  return card.toInt();
}

int calculateHand(String hand[], int count) {
  int sum = 0;
  int aces = 0;
  for (int i = 0; i < count; i++) {
    int val = cardValue(hand[i]);
    sum += val;
    if (hand[i] == "A") aces++;
  }
  while (sum > 21 && aces > 0) {
    sum -= 10;
    aces--;
  }
  return sum;
}

void updateDisplay() {
  lcd.clear();
  if (state == BETTING) {
    lcd.setCursor(0, 0); lcd.print("Bet: "); lcd.print(bet);
    lcd.setCursor(0, 1); lcd.print("Balance: "); lcd.print(balance);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("P: ");
    for (int i = 0; i < playerCards && i < 6; i++) {
      lcd.print(playerHand[i]);
      lcd.print(" ");
    }
    lcd.setCursor(0, 1);
    lcd.print("D: ");
    if (!dealerReveal) {
      lcd.print(dealerHand[0]);
      lcd.print(" ??");
    } else {
      for (int i = 0; i < dealerCards && i < 6; i++) {
        lcd.print(dealerHand[i]);
        lcd.print(" ");
      }
    }
  }
}

void printHands() {
  Serial.print("Player: ");
  for (int i = 0; i < playerCards; i++) Serial.print(playerHand[i] + " ");
  Serial.print("=> ");
  Serial.println(calculateHand(playerHand, playerCards));

  Serial.print("Dealer: ");
  for (int i = 0; i < dealerCards; i++) Serial.print(dealerHand[i] + " ");
  Serial.print("=> ");
  Serial.println(calculateHand(dealerHand, dealerCards));
}

void setLED(int r, int g, int b) {
  analogWrite(LED_R, r);
  analogWrite(LED_G, g);
  analogWrite(LED_B, b);
}

void clearLED() {
  setLED(0, 0, 0);
}

bool isButtonPressed(int pin) {
  static unsigned long lastTime = 0;
  static bool lastButtonState = HIGH;

  bool currentButtonState = digitalRead(pin);
  if (currentButtonState == LOW && lastButtonState == HIGH && (millis() - lastTime) > debounceDelay) {
    lastTime = millis();
    lastButtonState = LOW;
    return true;
  }

  lastButtonState = currentButtonState;
  return false;
}
