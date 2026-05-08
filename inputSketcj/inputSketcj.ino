/*
  TCD Future Health survey UI for Elecrow CrowPanel 2.1 inch ESP32 rotary display.

  Target:
  - Elecrow CrowPanel 2.1inch-HMI ESP32 Rotary Display, model DHE03921D
  - ESP32-S3, ST7701 RGB 480x480 round IPS panel
  - LVGL 8.x for Arduino

  Arduino IDE notes:
  - Board: ESP32S3 Dev Module, or Elecrow's recommended ESP32-S3 board profile
  - Flash: 16 MB
  - PSRAM: OPI PSRAM enabled
  - Partition: a large app partition if your installed libraries require it
*/

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <lvgl.h>
#include <PCF8574.h>
#include <Adafruit_CST8XX.h>

#if LV_COLOR_DEPTH != 16
#error "Set LV_COLOR_DEPTH to 16 in lv_conf.h for Arduino_GFX RGB565 flushing."
#endif

// ==================================================
// Hardware pins and device addresses
// ==================================================

#define DISPLAY_WIDTH 480
#define DISPLAY_HEIGHT 480

#define I2C_SDA_PIN 38
#define I2C_SCL_PIN 39
#define I2C_TOUCH_ADDR 0x15
#define PCF8574_ADDR 0x21

#define ENCODER_A_PIN 42
#define ENCODER_B_PIN 4
#define SCREEN_BACKLIGHT_PIN 6

#define PCF_TOUCH_RESET_PIN 0
#define PCF_TOUCH_INTERRUPT_PIN 2
#define PCF_LCD_POWER_PIN 3
#define PCF_LCD_RESET_PIN 4
#define PCF_ENCODER_BUTTON_PIN 5

// Flip this if your physical knob direction is reversed.
#define ENCODER_REVERSED 0

// Flip this if the touch coordinates are mirrored on your panel/rotation.
#define TOUCH_SWAP_XY 0
#define TOUCH_INVERT_X 0
#define TOUCH_INVERT_Y 0

// ==================================================
// Visual constants
// ==================================================

static const lv_color_t COLOR_BLACK = lv_color_hex(0x000000);
static const lv_color_t COLOR_WHITE = lv_color_hex(0xffffff);
static const lv_color_t COLOR_MUTED = lv_color_hex(0xa8b2b8);
static const lv_color_t COLOR_MUTED_CYAN = lv_color_hex(0xc8f8ff);
static const lv_color_t COLOR_ACCENT = lv_color_hex(0x00fff0);
static const lv_color_t COLOR_ACCENT_DIM = lv_color_hex(0x054642);
static const lv_color_t COLOR_PANEL = lv_color_hex(0x050707);

static const uint16_t SAFE_W = 344;
static const uint16_t SAFE_TOP = 42;
static const uint16_t SAFE_BOTTOM = 36;
static const uint16_t CONTENT_GAP = 8;
static const uint16_t SELECTOR_W = 258;
static const uint16_t SELECTED_H = 76;
static const uint16_t BUTTON_H = 38;
static const uint16_t BUTTON_RADIUS = 8;
static const uint16_t PANEL_RADIUS = 10;

static const uint32_t LVGL_TICK_MS = 5;
static const uint32_t LOOP_DELAY_MS = 5;
static const uint32_t INACTIVITY_TIMEOUT_MS = 120000;
static const uint32_t BUTTON_DEBOUNCE_MS = 35;

// ==================================================
// Survey model
// ==================================================

enum QuestionType : uint8_t {
  QUESTION_RATE_1_5,
  QUESTION_NPS_0_10,
  QUESTION_LIST
};

struct Question {
  const char *id;
  const char *title;
  const char *body;
  QuestionType type;
  const char *const *options;
  uint8_t optionCount;
};

static const char *const RATE_1_5[] = {"1", "2", "3", "4", "5"};
static const char *const NPS_0_10[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
static const char *const FISH_TYPES[] = {"Betta", "Tetra", "Gourami", "Shark", "Crab"};
static const char *const FISH_COLORS[] = {"Cyan", "Blue", "Purple", "Green", "Orange", "Red", "White"};

static const Question QUESTIONS[] = {
  {
    "overall_comparison",
    "Overall Comparison",
    "Think back to the last events you attended from other laboratories, especially in the same area. On a scale of 1 to 5, how would you rate this TCD event compared to those from other colleges?",
    QUESTION_RATE_1_5,
    RATE_1_5,
    5
  },
  {
    "organisation",
    "Organisation",
    "Think about recent events you attended with other laboratories. How does your experience with TCD compare to theirs for organisation during the event?",
    QUESTION_RATE_1_5,
    RATE_1_5,
    5
  },
  {
    "recommendation",
    "Recommendation",
    "How likely are you to recommend a similar session to a colleague?",
    QUESTION_NPS_0_10,
    NPS_0_10,
    11
  },
  {
    "fish_type",
    "Fish Type",
    "Choose a fish to add to the shared aquarium.",
    QUESTION_LIST,
    FISH_TYPES,
    5
  },
  {
    "fish_colour",
    "Fish Colour",
    "Choose the colour of your fish.",
    QUESTION_LIST,
    FISH_COLORS,
    7
  }
};

static const uint8_t QUESTION_COUNT = sizeof(QUESTIONS) / sizeof(QUESTIONS[0]);

enum ScreenState : uint8_t {
  SCREEN_HOME,
  SCREEN_QUESTION,
  SCREEN_COMPLETE
};

static ScreenState screenState = SCREEN_HOME;
static uint8_t currentQuestion = 0;
static uint8_t selectedIndices[QUESTION_COUNT] = {0};
static uint8_t answers[QUESTION_COUNT] = {0};
static bool hasAnswer[QUESTION_COUNT] = {false};
static uint32_t lastActivityMs = 0;

// ==================================================
// Display and input objects
// ==================================================

PCF8574 pcf8574(PCF8574_ADDR);
Adafruit_CST8XX touchPanel = Adafruit_CST8XX();
static bool touchReady = false;

Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
  16 /* CS */, 2 /* SCK */, 1 /* SDA */,
  40 /* DE */, 7 /* VSYNC */, 15 /* HSYNC */, 41 /* PCLK */,
  46 /* R0 */, 3 /* R1 */, 8 /* R2 */, 18 /* R3 */, 17 /* R4 */,
  14 /* G0 */, 13 /* G1 */, 12 /* G2 */, 11 /* G3 */, 10 /* G4 */, 9 /* G5 */,
  5 /* B0 */, 45 /* B1 */, 48 /* B2 */, 47 /* B3 */, 21 /* B4 */
);

Arduino_ST7701_RGBPanel *gfx = new Arduino_ST7701_RGBPanel(
  bus,
  GFX_NOT_DEFINED,
  0,
  false,
  DISPLAY_WIDTH,
  DISPLAY_HEIGHT,
  st7701_type5_init_operations,
  sizeof(st7701_type5_init_operations),
  true,
  10, 4, 20,
  10, 4, 20
);

static lv_disp_draw_buf_t drawBuf;
static lv_color_t lvglBuf1[DISPLAY_WIDTH * 40];
static lv_color_t lvglBuf2[DISPLAY_WIDTH * 40];
static lv_disp_drv_t dispDrv;
static lv_indev_drv_t touchDrv;

static volatile int32_t encoderDelta = 0;
static volatile uint8_t encoderLastA = 0;
static bool lastButtonRaw = true;
static bool stableButtonState = true;
static uint32_t lastButtonChangeMs = 0;

// ==================================================
// LVGL widgets
// ==================================================

static lv_obj_t *homeRoot = nullptr;
static lv_obj_t *questionRoot = nullptr;
static lv_obj_t *completeRoot = nullptr;

static lv_obj_t *questionCountLabel = nullptr;
static lv_obj_t *questionTitleLabel = nullptr;
static lv_obj_t *questionBodyLabel = nullptr;
static lv_obj_t *previousOptionLabel = nullptr;
static lv_obj_t *selectedOptionPanel = nullptr;
static lv_obj_t *selectedOptionLabel = nullptr;
static lv_obj_t *nextOptionLabel = nullptr;
static lv_obj_t *progressLabel = nullptr;
static lv_obj_t *prevButton = nullptr;
static lv_obj_t *okButton = nullptr;
static lv_obj_t *nextButton = nullptr;

// ==================================================
// Font helpers
// ==================================================

static const lv_font_t *fontTiny() {
#if LV_FONT_MONTSERRAT_12
  return &lv_font_montserrat_12;
#else
  return LV_FONT_DEFAULT;
#endif
}

static const lv_font_t *fontBody() {
#if LV_FONT_MONTSERRAT_14
  return &lv_font_montserrat_14;
#else
  return LV_FONT_DEFAULT;
#endif
}

static const lv_font_t *fontButton() {
#if LV_FONT_MONTSERRAT_16
  return &lv_font_montserrat_16;
#else
  return LV_FONT_DEFAULT;
#endif
}

static const lv_font_t *fontTitle() {
#if LV_FONT_MONTSERRAT_26
  return &lv_font_montserrat_26;
#elif LV_FONT_MONTSERRAT_24
  return &lv_font_montserrat_24;
#else
  return LV_FONT_DEFAULT;
#endif
}

static const lv_font_t *fontSelected() {
#if LV_FONT_MONTSERRAT_34
  return &lv_font_montserrat_34;
#elif LV_FONT_MONTSERRAT_32
  return &lv_font_montserrat_32;
#elif LV_FONT_MONTSERRAT_28
  return &lv_font_montserrat_28;
#else
  return LV_FONT_DEFAULT;
#endif
}

static const lv_font_t *fontHero() {
#if LV_FONT_MONTSERRAT_40
  return &lv_font_montserrat_40;
#elif LV_FONT_MONTSERRAT_36
  return &lv_font_montserrat_36;
#elif LV_FONT_MONTSERRAT_34
  return &lv_font_montserrat_34;
#else
  return LV_FONT_DEFAULT;
#endif
}

// ==================================================
// Forward declarations
// ==================================================

void setupDisplay();
void setupEncoder();
void renderHomeScreen();
void renderQuestionScreen();
void updateSelection();
void confirmSelection();
void goPrevious();
void goNext();
void renderCompleteScreen();
void resetSurvey();

// ==================================================
// Low-level callbacks
// ==================================================

void IRAM_ATTR encoderISR() {
  uint8_t a = digitalRead(ENCODER_A_PIN);

  if (a == encoderLastA) {
    return;
  }

  uint8_t b = digitalRead(ENCODER_B_PIN);
  int8_t step = (b == a) ? 1 : -1;
#if ENCODER_REVERSED
  step = -step;
#endif
  encoderDelta += step;
  encoderLastA = a;
}

static void lvglFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *colorP) {
  uint32_t width = area->x2 - area->x1 + 1;
  uint32_t height = area->y2 - area->y1 + 1;

  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)colorP, width, height);
  lv_disp_flush_ready(disp);
}

static void lvglTouchRead(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  (void)drv;

  if (!touchReady || !touchPanel.touched()) {
    data->state = LV_INDEV_STATE_REL;
    return;
  }

  CST_TS_Point point = touchPanel.getPoint(0);
  int16_t x = point.x;
  int16_t y = point.y;

#if TOUCH_SWAP_XY
  int16_t temp = x;
  x = y;
  y = temp;
#endif
#if TOUCH_INVERT_X
  x = DISPLAY_WIDTH - 1 - x;
#endif
#if TOUCH_INVERT_Y
  y = DISPLAY_HEIGHT - 1 - y;
#endif

  data->state = LV_INDEV_STATE_PR;
  data->point.x = constrain(x, 0, DISPLAY_WIDTH - 1);
  data->point.y = constrain(y, 0, DISPLAY_HEIGHT - 1);
}

static void buttonEvent(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);
  if (code != LV_EVENT_CLICKED) {
    return;
  }

  lv_obj_t *target = lv_event_get_target(event);
  lastActivityMs = millis();

  if (target == prevButton) {
    goPrevious();
  } else if (target == okButton) {
    confirmSelection();
  } else if (target == nextButton) {
    goNext();
  }
}

// ==================================================
// Style helpers
// ==================================================

static void styleLabel(lv_obj_t *label, lv_color_t color, const lv_font_t *font, lv_text_align_t align) {
  lv_obj_set_style_text_color(label, color, 0);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_align(label, align, 0);
  lv_obj_set_style_text_letter_space(label, 0, 0);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
}

static lv_obj_t *makeLabel(lv_obj_t *parent, lv_color_t color, const lv_font_t *font, lv_coord_t width) {
  lv_obj_t *label = lv_label_create(parent);
  lv_obj_set_width(label, width);
  styleLabel(label, color, font, LV_TEXT_ALIGN_CENTER);
  return label;
}

static lv_obj_t *makeButton(lv_obj_t *parent, const char *text, lv_coord_t width) {
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_set_size(button, width, BUTTON_H);
  lv_obj_set_style_bg_color(button, COLOR_BLACK, 0);
  lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(button, COLOR_ACCENT_DIM, 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_radius(button, BUTTON_RADIUS, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_add_event_cb(button, buttonEvent, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *label = lv_label_create(button);
  lv_label_set_text(label, text);
  styleLabel(label, COLOR_WHITE, fontButton(), LV_TEXT_ALIGN_CENTER);
  lv_obj_center(label);
  return button;
}

static void configureRoot(lv_obj_t *root) {
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(root, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  lv_obj_set_style_bg_color(root, COLOR_BLACK, 0);
  lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(root, 0, 0);
  lv_obj_set_style_pad_all(root, 0, 0);
}

static void createSafeColumn(lv_obj_t *root, lv_coord_t y, lv_coord_t h, lv_coord_t gap) {
  lv_obj_set_size(root, SAFE_W, h);
  lv_obj_align(root, LV_ALIGN_TOP_MID, 0, y);
  lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(root, gap, 0);
}

static uint8_t optionCountForCurrentQuestion() {
  return QUESTIONS[currentQuestion].optionCount;
}

static const char *optionLabel(uint8_t questionIndex, uint8_t optionIndex) {
  const Question &question = QUESTIONS[questionIndex];
  optionIndex %= question.optionCount;
  return question.options[optionIndex];
}

static uint8_t wrapIndex(int16_t index, uint8_t count) {
  while (index < 0) {
    index += count;
  }
  return index % count;
}

static void markActivity() {
  lastActivityMs = millis();
}

// ==================================================
// Screen rendering
// ==================================================

void renderHomeScreen() {
  lv_obj_clean(lv_scr_act());
  screenState = SCREEN_HOME;

  homeRoot = lv_obj_create(lv_scr_act());
  configureRoot(homeRoot);
  createSafeColumn(homeRoot, 72, 330, 16);

  lv_obj_t *accent = lv_obj_create(homeRoot);
  lv_obj_set_size(accent, 104, 104);
  lv_obj_set_style_radius(accent, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(accent, COLOR_BLACK, 0);
  lv_obj_set_style_bg_opa(accent, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(accent, COLOR_ACCENT, 0);
  lv_obj_set_style_border_width(accent, 2, 0);
  lv_obj_set_style_shadow_color(accent, COLOR_ACCENT, 0);
  lv_obj_set_style_shadow_width(accent, 18, 0);
  lv_obj_set_style_shadow_opa(accent, LV_OPA_30, 0);

  lv_obj_t *mark = lv_label_create(accent);
  lv_label_set_text(mark, LV_SYMBOL_RIGHT);
  styleLabel(mark, COLOR_ACCENT, fontHero(), LV_TEXT_ALIGN_CENTER);
  lv_obj_center(mark);

  lv_obj_t *title = makeLabel(homeRoot, COLOR_WHITE, fontHero(), SAFE_W);
  lv_label_set_text(title, "Feedback Device");

  lv_obj_t *copy = makeLabel(homeRoot, COLOR_MUTED_CYAN, fontBody(), 260);
  lv_label_set_text(copy, "Turn the dial to choose. Press to begin.");

  markActivity();
}

void renderQuestionScreen() {
  lv_obj_clean(lv_scr_act());
  screenState = SCREEN_QUESTION;

  questionRoot = lv_obj_create(lv_scr_act());
  configureRoot(questionRoot);
  createSafeColumn(questionRoot, SAFE_TOP, DISPLAY_HEIGHT - SAFE_TOP - SAFE_BOTTOM, CONTENT_GAP);

  questionCountLabel = makeLabel(questionRoot, COLOR_ACCENT, fontTiny(), SAFE_W);
  lv_obj_set_style_text_opa(questionCountLabel, LV_OPA_COVER, 0);

  questionTitleLabel = makeLabel(questionRoot, COLOR_WHITE, fontTitle(), SAFE_W);

  questionBodyLabel = makeLabel(questionRoot, COLOR_MUTED, fontBody(), 314);
  lv_obj_set_style_text_line_space(questionBodyLabel, 2, 0);
  lv_obj_set_style_text_opa(questionBodyLabel, LV_OPA_90, 0);

  previousOptionLabel = makeLabel(questionRoot, COLOR_WHITE, fontButton(), SELECTOR_W);
  lv_obj_set_style_text_opa(previousOptionLabel, 115, 0);

  selectedOptionPanel = lv_obj_create(questionRoot);
  lv_obj_clear_flag(selectedOptionPanel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(selectedOptionPanel, SELECTOR_W, SELECTED_H);
  lv_obj_set_style_bg_color(selectedOptionPanel, COLOR_PANEL, 0);
  lv_obj_set_style_bg_opa(selectedOptionPanel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(selectedOptionPanel, COLOR_ACCENT, 0);
  lv_obj_set_style_border_width(selectedOptionPanel, 2, 0);
  lv_obj_set_style_radius(selectedOptionPanel, PANEL_RADIUS, 0);
  lv_obj_set_style_pad_all(selectedOptionPanel, 6, 0);

  selectedOptionLabel = lv_label_create(selectedOptionPanel);
  lv_obj_set_width(selectedOptionLabel, SELECTOR_W - 24);
  styleLabel(selectedOptionLabel, COLOR_WHITE, fontSelected(), LV_TEXT_ALIGN_CENTER);
  lv_obj_center(selectedOptionLabel);

  nextOptionLabel = makeLabel(questionRoot, COLOR_WHITE, fontButton(), SELECTOR_W);
  lv_obj_set_style_text_opa(nextOptionLabel, 115, 0);

  progressLabel = makeLabel(questionRoot, COLOR_MUTED_CYAN, fontTiny(), SAFE_W);
  lv_obj_set_style_text_opa(progressLabel, 216, 0);

  lv_obj_t *buttonRow = lv_obj_create(questionRoot);
  lv_obj_clear_flag(buttonRow, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(buttonRow, 302, BUTTON_H);
  lv_obj_set_style_bg_opa(buttonRow, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(buttonRow, 0, 0);
  lv_obj_set_style_pad_all(buttonRow, 0, 0);
  lv_obj_set_flex_flow(buttonRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(buttonRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  prevButton = makeButton(buttonRow, "Prev", 82);
  okButton = makeButton(buttonRow, "OK", 102);
  nextButton = makeButton(buttonRow, "Next", 82);

  updateSelection();
  markActivity();
}

void updateSelection() {
  if (screenState != SCREEN_QUESTION) {
    return;
  }

  const Question &question = QUESTIONS[currentQuestion];
  uint8_t selected = selectedIndices[currentQuestion] % question.optionCount;
  uint8_t previous = wrapIndex(selected - 1, question.optionCount);
  uint8_t next = wrapIndex(selected + 1, question.optionCount);

  char buffer[28];
  snprintf(buffer, sizeof(buffer), "Question %u / %u", currentQuestion + 1, QUESTION_COUNT);
  lv_label_set_text(questionCountLabel, buffer);

  lv_label_set_text(questionTitleLabel, question.title);
  lv_label_set_text(questionBodyLabel, question.body);
  lv_label_set_text(previousOptionLabel, optionLabel(currentQuestion, previous));
  lv_label_set_text(selectedOptionLabel, optionLabel(currentQuestion, selected));
  lv_label_set_text(nextOptionLabel, optionLabel(currentQuestion, next));

  snprintf(buffer, sizeof(buffer), "%u of %u", selected + 1, question.optionCount);
  lv_label_set_text(progressLabel, buffer);

  if (question.type == QUESTION_LIST) {
    lv_obj_set_style_text_font(selectedOptionLabel, fontTitle(), 0);
  } else {
    lv_obj_set_style_text_font(selectedOptionLabel, fontSelected(), 0);
  }
}

void renderCompleteScreen() {
  lv_obj_clean(lv_scr_act());
  screenState = SCREEN_COMPLETE;

  completeRoot = lv_obj_create(lv_scr_act());
  configureRoot(completeRoot);
  createSafeColumn(completeRoot, 74, 328, 15);

  lv_obj_t *ring = lv_obj_create(completeRoot);
  lv_obj_set_size(ring, 112, 112);
  lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(ring, COLOR_BLACK, 0);
  lv_obj_set_style_bg_opa(ring, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(ring, COLOR_ACCENT, 0);
  lv_obj_set_style_border_width(ring, 2, 0);
  lv_obj_set_style_shadow_color(ring, COLOR_ACCENT, 0);
  lv_obj_set_style_shadow_width(ring, 22, 0);
  lv_obj_set_style_shadow_opa(ring, LV_OPA_40, 0);

  lv_obj_t *check = lv_label_create(ring);
  lv_label_set_text(check, LV_SYMBOL_OK);
  styleLabel(check, COLOR_ACCENT, fontHero(), LV_TEXT_ALIGN_CENTER);
  lv_obj_center(check);

  lv_obj_t *title = makeLabel(completeRoot, COLOR_WHITE, fontHero(), SAFE_W);
  lv_label_set_text(title, "Thank You");

  lv_obj_t *copy = makeLabel(completeRoot, COLOR_MUTED_CYAN, fontBody(), 272);
  lv_label_set_text(copy, "Your fish has been added to the aquarium.");

  markActivity();
}

// ==================================================
// Survey interaction
// ==================================================

static void moveSelection(int8_t direction) {
  if (screenState == SCREEN_HOME) {
    screenState = SCREEN_QUESTION;
    renderQuestionScreen();
    return;
  }

  if (screenState != SCREEN_QUESTION) {
    return;
  }

  uint8_t count = optionCountForCurrentQuestion();
  int16_t selected = selectedIndices[currentQuestion];
  selectedIndices[currentQuestion] = wrapIndex(selected + direction, count);
  updateSelection();
  markActivity();
}

void confirmSelection() {
  if (screenState == SCREEN_HOME) {
    renderQuestionScreen();
    return;
  }

  if (screenState == SCREEN_COMPLETE) {
    resetSurvey();
    return;
  }

  if (screenState != SCREEN_QUESTION) {
    return;
  }

  answers[currentQuestion] = selectedIndices[currentQuestion];
  hasAnswer[currentQuestion] = true;

  if (currentQuestion + 1 < QUESTION_COUNT) {
    currentQuestion++;
    renderQuestionScreen();
    return;
  }

  renderCompleteScreen();
}

void goPrevious() {
  if (screenState == SCREEN_COMPLETE) {
    resetSurvey();
    return;
  }

  if (screenState != SCREEN_QUESTION) {
    renderHomeScreen();
    return;
  }

  if (currentQuestion == 0) {
    renderHomeScreen();
    return;
  }

  currentQuestion--;
  renderQuestionScreen();
}

void goNext() {
  if (screenState == SCREEN_HOME) {
    renderQuestionScreen();
    return;
  }

  if (screenState == SCREEN_COMPLETE) {
    resetSurvey();
    return;
  }

  if (screenState != SCREEN_QUESTION) {
    return;
  }

  answers[currentQuestion] = selectedIndices[currentQuestion];
  hasAnswer[currentQuestion] = true;

  if (currentQuestion + 1 < QUESTION_COUNT) {
    currentQuestion++;
    renderQuestionScreen();
    return;
  }

  renderCompleteScreen();
}

void resetSurvey() {
  currentQuestion = 0;
  for (uint8_t i = 0; i < QUESTION_COUNT; i++) {
    selectedIndices[i] = 0;
    answers[i] = 0;
    hasAnswer[i] = false;
  }
  renderHomeScreen();
}

// ==================================================
// Setup and loop
// ==================================================

void setupDisplay() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  pcf8574.pinMode(PCF_LCD_POWER_PIN, OUTPUT);
  pcf8574.pinMode(PCF_LCD_RESET_PIN, OUTPUT);
  pcf8574.pinMode(PCF_TOUCH_RESET_PIN, OUTPUT);
  pcf8574.pinMode(PCF_TOUCH_INTERRUPT_PIN, INPUT);
  pcf8574.pinMode(PCF_ENCODER_BUTTON_PIN, INPUT_PULLUP);
  pcf8574.begin();

  pcf8574.digitalWrite(PCF_LCD_POWER_PIN, HIGH);
  pcf8574.digitalWrite(PCF_LCD_RESET_PIN, LOW);
  pcf8574.digitalWrite(PCF_TOUCH_RESET_PIN, LOW);
  delay(20);
  pcf8574.digitalWrite(PCF_LCD_RESET_PIN, HIGH);
  pcf8574.digitalWrite(PCF_TOUCH_RESET_PIN, HIGH);
  delay(120);

  pinMode(SCREEN_BACKLIGHT_PIN, OUTPUT);
  analogWrite(SCREEN_BACKLIGHT_PIN, 220);

  gfx->begin();
  gfx->fillScreen(BLACK);

  touchReady = touchPanel.begin(&Wire, I2C_TOUCH_ADDR);

  lv_init();
  lv_disp_draw_buf_init(&drawBuf, lvglBuf1, lvglBuf2, DISPLAY_WIDTH * 40);

  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = DISPLAY_WIDTH;
  dispDrv.ver_res = DISPLAY_HEIGHT;
  dispDrv.flush_cb = lvglFlush;
  dispDrv.draw_buf = &drawBuf;
  lv_disp_drv_register(&dispDrv);

  if (touchReady) {
    lv_indev_drv_init(&touchDrv);
    touchDrv.type = LV_INDEV_TYPE_POINTER;
    touchDrv.read_cb = lvglTouchRead;
    lv_indev_drv_register(&touchDrv);
  }
}

void setupEncoder() {
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  encoderLastA = digitalRead(ENCODER_A_PIN);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);
}

static void pollEncoderButton() {
  bool rawPressed = pcf8574.digitalRead(PCF_ENCODER_BUTTON_PIN) == LOW;
  uint32_t now = millis();

  if (rawPressed != lastButtonRaw) {
    lastButtonRaw = rawPressed;
    lastButtonChangeMs = now;
  }

  if ((now - lastButtonChangeMs) < BUTTON_DEBOUNCE_MS) {
    return;
  }

  if (rawPressed != stableButtonState) {
    stableButtonState = rawPressed;
    if (stableButtonState) {
      markActivity();
      confirmSelection();
    }
  }
}

static void processEncoder() {
  int32_t delta = 0;

  noInterrupts();
  delta = encoderDelta;
  encoderDelta = 0;
  interrupts();

  if (delta == 0) {
    return;
  }

  moveSelection(delta > 0 ? 1 : -1);
  markActivity();
}

static void checkInactivityTimeout() {
  if (screenState == SCREEN_HOME) {
    return;
  }

  if (millis() - lastActivityMs >= INACTIVITY_TIMEOUT_MS) {
    resetSurvey();
  }
}

void setup() {
  Serial.begin(115200);
  setupDisplay();
  setupEncoder();
  renderHomeScreen();
}

void loop() {
  static uint32_t lastTickMs = millis();
  uint32_t now = millis();

  if (now - lastTickMs >= LVGL_TICK_MS) {
    lv_tick_inc(now - lastTickMs);
    lastTickMs = now;
  }

  processEncoder();
  pollEncoderButton();
  checkInactivityTimeout();

  lv_timer_handler();
  delay(LOOP_DELAY_MS);
}
