// Microwriter reboot on Arduino (c)2016 vik@diamondage.co.nz
// Released under the GPLv3 licence http://www.gnu.org/licenses/gpl-3.0.en.html
// Uses key tables loosely borrowed from the Microwriter https://en.wikipedia.org/wiki/Microwriter
// Requires a Leonardo or Micro (ATmega32u4 CPU) to emulate keyboard and mouse.

#include <Keyboard.h>
#include <Mouse.h>

// After moving this number of ticks, the mouse will accelerate
#define MOUSE_ACCELERATION_POINT 40
// Maximum speedup on mouse
#define MOUSE_MAX_ACCELERATION 7
// Interval before repeating the key in 10ms increments
#define REPEAT_START_DELAY 220
// Interval between repeating chords in 10ms increments
#define REPEAT_INTERVAL_DELAY 8
#define NUM_KEYS  6
#define KEYS_SHIFT_ON  1000
#define KEYS_SHIFT_OFF  1001
#define KEYS_NUMERIC_SHIFT 1002
#define KEYS_CONTROL_SHIFT  1003
#define KEYS_ALT_SHIFT  1004
#define KEYS_EXTRA_SHIFT  1005
#define KEYS_MOUSE_MODE_ON  1006
#define KEYS_FUNC_SHIFT  1007
#define KEYS_ALTGR_SHIFT  1008

// Ctrl, pinkie, ring, middle, 1st, thumb pins.
// Activating a switch grounds the respective pin.
const int keyPorts[] = {8, 7, 6, 5, 4, 9};

const char alphaTable[] = " eiocadsktrny.fuhvlqz-'gj,wbxmp";
const char numericTable[] = " 120(*3$/+;\"?.46-&#)%!@7=,:8`95";
const char extraTable[] = {0, KEY_ESC, 0, 0, '[', 0, KEY_DELETE, 0,
                           // k
                           KEY_HOME, 0, 0, 0, 0, 0, KEY_END, 0,
                           0, '\\', 0, ']', KEY_PAGE_DOWN, 0, 0, 0,
                           KEY_PAGE_UP, 0, 0, 0, 0, 0, 0, 0
                          };
const char funcTable[] = {0, KEY_F1, KEY_F2, KEY_F10, 0, KEY_F11, KEY_F3, 0,
                          0, 0, 0, 0, 0, KEY_F12, KEY_F4, KEY_F6,
                          0, 0, 0, 0, 0, 0, 0, KEY_F7,
                          0, 0, 0, KEY_F8, 0, KEY_F9, KEY_F5
                         };
                         // Ctrl          Ctrl-Space
const int shiftTable[] = {KEYS_SHIFT_ON, KEYS_SHIFT_OFF, KEY_INSERT, KEYS_MOUSE_MODE_ON, KEY_RETURN, 0, KEY_BACKSPACE, 0,
                          KEY_LEFT_ARROW, 0, KEY_TAB, 0, KEYS_NUMERIC_SHIFT, 0, KEY_RIGHT_ARROW, 0,
                          // Ctrl-H         Ctrl-space-H      Ctrl-L              Ctrl-Z
                          KEYS_EXTRA_SHIFT, KEYS_ALTGR_SHIFT, KEYS_FUNC_SHIFT, 0, KEY_DOWN_ARROW, 0, 0, 0,
                          KEY_UP_ARROW, 0, 0, 0, KEYS_CONTROL_SHIFT, 0, KEYS_ALT_SHIFT, 0
                         };

// Chord to repeat, 0 if none.
char repeatingChord = 0;
// >0 when shift is on.
char shifted = 0;
// >0 when using numeric table
char numericed = 0;
// >0 when CTRL is on
char controlled = 0;
// >0 when ALT is on
char alted = 0;
// >0 When extra shift on
char extraed = 0;
// >0 when using function key table
char funced = 0;
// >0 When right Alt is down
char altgred=0;

void setup() {
  int i = 0;
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    delay(200);
    // wait for serial port to connect. Needed for Leonardo only
    if (i++ > 12) break;
  }

  for (i = 0; i < NUM_KEYS; i++) pinMode(keyPorts[i], INPUT_PULLUP);
  while (digitalRead(keyPorts[0]) == LOW) Serial.println("Testing...");
  // Prevent keyboard lockout.
  delay(5000);
  Keyboard.begin();
}

// Return the raw keybits
int keyBits() {
  int i, k = 0;

  for (i = 0; i < NUM_KEYS; i++) {
    k *= 2;
    if (digitalRead(keyPorts[i]) == LOW)
      k++;
  }
  return (k);
}

/**************************************************************************
 * If the repeating chord is still held down, return it after a brief delay.
 * Otherwise, wait until all keys are released and return zero.
 */
int getRepeatingChord() {
  int k = 0;

  while (true) {
    // Debounce
    while (k != keyBits()) {
      delay(10);
      k = keyBits();
    }
    
    // If the chord matches the repeat, delay a bit and return the chord
    if (k == repeatingChord) {
      delay(10*REPEAT_INTERVAL_DELAY);
      return(k);
    }

    // Whatever the chord changed to, we wait for it to go away
    while (keyBits() != 0) delay(10);
    repeatingChord = 0;
    return(0);
  }
}

int keyWait() {
  int k, x;
  int timer=0;
  x = 0;
  k = 0;

  // If repeating a chord, get it after a delay etc.
  if (repeatingChord != 0) {
    x = getRepeatingChord();
    if (x != 0)
      return(x);
      // If we drop out there, the repeat ended and we need a new chord.
  }

  
  while (true) {
    // Debounce
    while (k != keyBits()) {
      delay(10);
      k = keyBits();
    }
    // Key bit map is now stable.
    // If the chord changed, reset the repeat timer.
    if (x != k)
      timer = 0;
    // Accumulate key bit changes
    x |= k;
    // If all keys released, return accumulated chord
    if ((x != 0) && (k == 0))
      return(x);
    // Do the next repeat time increment
    delay(10);
    // If keys are still down and we have exceeded repeat time, enter
    // repeat mode and return the chord.
    if (k != 0)
      timer++;
    if (timer > REPEAT_START_DELAY) {
      repeatingChord = x;
      break;
    }
    
  }
  return(x);
}

void everythingOff() {
  Keyboard.releaseAll();
  shifted = 0;
  numericed = 0;
  controlled = 0;
  alted = 0;
  extraed = 0;
  funced = 0;
  altgred=0;
}

/* Uses finger keys as LUDR, thumb as click 1 & 2.
   All 4 fingers exit mouse mode. */
void mouseMode() {
  int k = 0;
  int x, y;
  int mouseTicks = 0;
  int mouseMove=1;

  Mouse.begin();
  while (true) {
    // Determine if the mouse is accelerating or not. If it is, bump up movement factor
    if (mouseTicks > MOUSE_ACCELERATION_POINT) {
        mouseMove = int(mouseTicks/MOUSE_ACCELERATION_POINT);
        if (mouseMove > MOUSE_MAX_ACCELERATION)
            mouseMove = MOUSE_MAX_ACCELERATION;
    } else
        mouseMove = 1;
        
    k = keyBits();
    if (k == 30) break; // Quit mousing if all 4 move keys hit.
    if (k == 0)
      // Mouse stopped moving.
      mouseTicks = 0;

    if ((k & 2) != 0) {
      // Mouse left
      x = -mouseMove;
    }
    if ((k & 16) != 0) {
      // Mouse right
      x = mouseMove;
    }
    if ((k & 4) != 0) {
      // Mouse up
      y = -mouseMove;
    }
    if ((k & 8) != 0) {
      // Mouse down
      y = mouseMove;
    }

    // Mouse clicks
    if ((k & 1) != 0) {
      Mouse.press(MOUSE_LEFT);
      delay(50);
      while ((keyBits() & 1) != 0);
      Mouse.release(MOUSE_LEFT);
    }

    if ((k & 32) != 0) {
      Mouse.press(MOUSE_RIGHT);
      delay(50);
      while ((keyBits() & 32) != 0);
      Mouse.release(MOUSE_RIGHT);
    }

    // If keys moved, move mouse.
    if ((x != 0) || (y != 0)) {
      Mouse.move(x, y, 0);
      delay(10);
      mouseTicks++;
      x = y = 0;
    }
  }
  Mouse.end();
  // Wait for all keys up
  while (keyBits() != 0);
}

void loop() {
  int x;

  x = keyWait();
  //Serial.println(x);
  if (x < 32) {
    if (numericed != 0) {
      // Numeric character
      Keyboard.write(numericTable[x - 1]);
      // Some numeric characters press the shift key,
      // so make sure it is put back down if locked.
      if (shifted == 2) Keyboard.press(KEY_LEFT_SHIFT);
    } else if (extraed != 0) {
      //Serial.print("Extra ");
      //Serial.println(x);
      Keyboard.write(extraTable[x - 1]);
    } else if (funced != 0) {
      //Serial.print("Func ");
      //Serial.println(x);
      Keyboard.write(funcTable[x - 1]);
    } else
      Keyboard.write(alphaTable[x - 1]);

    if (shifted == 1) { // Clear a single shift.
      shifted = 0;
      Keyboard.release(KEY_LEFT_SHIFT);
    }
    if (controlled == 1) { // Clear a single control shift.
      controlled = 0;
      Keyboard.release(KEY_LEFT_CTRL);
    }
    if (alted == 1) { // Clear a single alt shift.
      alted = 0;
      Keyboard.release(KEY_LEFT_ALT);
    }
    if (altgred == 1) { // Clear a single alt shift.
      altgred = 0;
      Keyboard.release(KEY_RIGHT_ALT);
    }
    // Clear temporary numeric, extra and func shift
    numericed &= 2;
    extraed &= 2;
    funced &= 2;
  } else {
    // Must be a shift function then
    x = shiftTable[x - 32];
    if (x < 256) {
      Keyboard.write(x);
    } else {
      // Its a more complicated keypress requiring a function.
      switch (x) {
        case KEYS_SHIFT_ON:
          if (++shifted > 2) shifted = 2;
          Keyboard.press(KEY_LEFT_SHIFT);
          break;

        case KEYS_CONTROL_SHIFT:
          if (++controlled > 2) controlled = 2;
          Keyboard.press(KEY_LEFT_CTRL);
          break;

        case KEYS_SHIFT_OFF:
          everythingOff();
          break;

        case KEYS_NUMERIC_SHIFT:
          if (++numericed > 2) numericed = 2;
          break;

        case KEYS_EXTRA_SHIFT:
          if (++extraed > 2) extraed = 2;
          break;

        case KEYS_ALT_SHIFT:
          if (++alted > 2) alted = 2;
          Keyboard.press(KEY_LEFT_ALT);
          break;

        case KEYS_ALTGR_SHIFT:
          if (++altgred > 2) altgred = 2;
          Keyboard.press(KEY_RIGHT_ALT);
          break;

        case KEYS_FUNC_SHIFT:
          if (++funced > 2) funced = 2;
          break;

        case KEYS_MOUSE_MODE_ON:
          mouseMode();
          break;
      }
    }
  }
}
