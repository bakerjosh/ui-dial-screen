# Codex Context: Elecrow 2.1 inch ESP32 Rotary Display UI

## Project Goal

Build and improve the Arduino/LVGL UI for an Elecrow CrowPanel 2.1 inch ESP32 rotary display.

The display is part of a TCD healthcare innovation event prototype. Users answer a short survey on the physical rotary display. Their choices later feed into a shared aquarium visualisation, where each completed response adds one fish.

The target is a clean, stable, uploadable Arduino IDE project that can run on the Elecrow ESP32 panel.

---

## Target Hardware

- Elecrow CrowPanel 2.1 inch ESP32 rotary display
- 480x480 round IPS display
- Rotary encoder with push button
- Arduino IDE
- LVGL
- ESP32

Assume hardware pins may need adjustment. Define all hardware pins clearly at the top of the Arduino code or in a dedicated config/header file.

---

## Recommended Development Workflow

Use this workflow:

```text
HTML/CSS/JS emulator
→ LVGL XML/layout prototype
→ clean C/C++ LVGL UI module
→ integrate into Elecrow Arduino example sketch
→ flash ESP32
→ tune fonts, spacing, encoder behaviour on real hardware
```

### Step 1: Update GitHub first

Use GitHub as the source of truth.

Reasons:
- Version control makes changes recoverable.
- Codex can inspect and edit the repo.
- Changes can be reviewed before flashing hardware.
- Broken UI attempts can be rolled back.

Do not make large untracked local changes without committing or clearly separating them.

---

### Step 2: Test layout in the LVGL online viewer

Use the LVGL online viewer, XML workflow, or simulator-style layout testing to check:

- 480x480 layout proportions
- circular safe-zone spacing
- text wrapping
- selected option sizing
- colours
- visual hierarchy
- general UI clarity

The online viewer is useful for visual layout testing only.

It does not fully test:
- Elecrow display drivers
- ESP32 memory limits
- rotary encoder behaviour
- Arduino LVGL version differences
- real refresh rate
- hardware rotation
- uploaded sketch stability

Treat viewer success as layout validation, not hardware validation.

---

### Step 3: Convert the layout into C/C++ LVGL UI code

Once the layout is visually acceptable, convert it into normal LVGL code.

Preferred file structure:

```text
main_sketch.ino
ui.h
ui.cpp or ui.c
survey_state.h
survey_state.cpp or survey_state.c
config.h
```

Use:
- `.ino` for Arduino setup, loop, display init, encoder init, and LVGL timer handling.
- `.h` files for declarations, structs, enums, constants, and shared config.
- `.c` or `.cpp` files for UI rendering and survey state logic.

Keep UI logic separate from hardware setup where possible.

---

### Step 4: Integrate into the Elecrow Arduino example

Do not rewrite the full Elecrow base sketch from scratch.

Start from a known working Elecrow demo/example sketch, then replace only the UI/screen layer.

Preserve:
- display driver setup
- LVGL init
- touch/encoder/display hardware setup
- screen rotation settings
- flush/display buffer logic
- board-specific configuration

Modify:
- UI screen rendering
- survey state handling
- encoder event handling
- question data
- styling

This reduces the chance of breaking board-specific functionality.

---

### Step 5: Upload to ESP32 and tune on real hardware

Only flash the ESP32 after the layout is stable enough in the viewer.

On hardware, test:
- boot behaviour
- screen orientation
- display refresh
- encoder direction
- encoder press
- text readability
- circular edge clipping
- memory stability
- timeout/reset behaviour
- question navigation
- final completion screen

Expect final tuning to happen on the physical screen. A layout that looks correct in the viewer may still need adjustment on the actual round display.

---

## UI Design Target

The current prototype should be improved so it feels designed for a circular display, not like a rectangular UI cropped into a circle.

The UI should feel:
- modern
- minimal
- premium
- readable from a distance
- visually balanced
- stable and responsive

Use:
- black background
- white primary text
- muted grey secondary text
- cyan accent, close to `#00fff0`
- rounded rectangles
- clear spacing
- strong central focus

Avoid:
- tiny text
- excessive borders
- edge-clipping
- clutter
- debug-looking menus
- overly wide rectangular elements

---

## Round Display Layout Rules

All important content must remain inside the circular safe zone.

Prioritise:
- central alignment
- short titles
- wrapped body text
- large selected values
- muted previous/next values
- balanced vertical spacing

Avoid:
- placing text too close to the top/bottom edges
- long single-line labels
- wide button rows that feel rectangular
- excessive horizontal spacing

---

## Survey Screen Layout

Each question screen should contain:

1. Top progress text  
   Example: `Question 2 / 5`

2. Large bold title  
   Example: `Organisation`

3. Smaller wrapped question body text

4. Central rotary selector:
   - previous option above
   - selected option in middle
   - next option below
   - selected option inside cyan rounded rectangle
   - selected value large and readable
   - previous/next values smaller and muted

5. Bottom option index  
   Example: `3 of 5`

6. Bottom controls:
   - `Prev`
   - `OK`
   - `Next`

The currently selected value must always be the clearest item on screen.

---

## Interaction Logic

Rotary encoder:
- Clockwise moves selection forward.
- Anticlockwise moves selection backward.
- Press confirms current selection.

Navigation:
- `Prev` returns to the previous question.
- `Next` confirms and advances to the next question.
- On the final question, confirming shows the completion screen.

Behaviour:
- Store answers for all questions.
- Allow question-to-question navigation.
- Maintain current selected option per question.
- Add inactivity timeout.
- After 2 minutes of inactivity, reset to home/start screen.

---

## Survey Questions

Use these five questions.

### Question 1

Title:
```text
Overall Comparison
```

Full text:
```text
Think back to the last events you attended from other laboratories, especially in the same area. On a scale of 1 to 5, how would you rate this TCD event compared to those from other colleges?
```

Type:
```text
Rate 1-5
```

Options:
```text
1, 2, 3, 4, 5
```

Required:
```text
Yes
```

---

### Question 2

Title:
```text
Organisation
```

Full text:
```text
Think about recent events you attended with other laboratories. How does your experience with TCD compare to theirs for organisation during the event?
```

Type:
```text
Rate 1-5
```

Options:
```text
1, 2, 3, 4, 5
```

Required:
```text
Yes
```

---

### Question 3

Title:
```text
Recommendation
```

Full text:
```text
How likely are you to recommend a similar session to a colleague?
```

Type:
```text
NPS 0-10
```

Options:
```text
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
```

Required:
```text
Yes
```

---

### Question 4

Title:
```text
Fish Type
```

Full text:
```text
Choose a fish to add to the shared aquarium.
```

Type:
```text
Selection list
```

Options:
```text
Betta, Tetra, Gourami, Shark, Crab
```

Required:
```text
Yes
```

---

### Question 5

Title:
```text
Fish Colour
```

Full text:
```text
Choose the colour of your fish.
```

Type:
```text
Selection list
```

Options:
```text
Cyan, Blue, Purple, Green, Orange, Red, White
```

Required:
```text
Yes
```

---

## Completion Screen

After the final question is confirmed, show a simple completion screen.

Suggested text:

```text
Thank You
Your fish has been added to the aquarium.
```

Style:
- black background
- cyan accent
- simple confirmation message
- optional small fish icon if easy and stable

Do not add complex animation unless the base UI is already stable.

---

## Code Architecture

Use clear functions such as:

```cpp
setupDisplay();
setupEncoder();
renderHomeScreen();
renderQuestionScreen();
updateSelection();
confirmSelection();
goPrevious();
goNext();
renderCompleteScreen();
resetSurvey();
```

Use data structures for questions rather than hardcoding each screen separately.

Recommended concepts:
- `QuestionType`
- `Question`
- `SurveyState`
- `currentQuestionIndex`
- `selectedOptionIndex[]`
- `answers[]`
- `lastInteractionMs`

Keep constants grouped:
- colours
- font sizes
- spacing
- border radius
- animation timings
- timeout duration
- pin numbers

Avoid scattering hardcoded values throughout the code.

---

## Stability Priorities

Priority order:

1. Hardware stability
2. Survey logic correctness
3. Readable UI
4. Visual polish
5. Animation or advanced effects

Avoid:
- memory-heavy dynamic allocation
- large blocking delays
- unnecessary object creation
- repeatedly rebuilding the full screen if updating labels is enough
- changing Elecrow hardware setup unless necessary

Use simple, predictable LVGL widgets.

---

## Important Implementation Notes

- The online LVGL viewer is not a replacement for hardware testing.
- The `.ino`, `.h`, and `.c/.cpp` files must be updated only after the layout direction is stable.
- Keep the Elecrow-provided LVGL/display setup intact unless there is a clear error.
- The final UI must be judged on the physical 480x480 round screen, not just the browser/viewer.
- If there is a mismatch between emulator aesthetics and hardware readability, prioritise hardware readability.

---

## Codex Behaviour Instructions

When editing this project:

1. Inspect the existing repo before changing files.
2. Identify the current Elecrow/LVGL setup.
3. Do not delete working board-specific setup code without justification.
4. First improve layout and state structure.
5. Then improve styling.
6. Then improve hardware input handling.
7. Keep changes small and testable.
8. Explain which files were changed and why.
9. Mention any assumptions about LVGL version, board config, pins, or libraries.
10. Do not invent unsupported hardware features.

The intended result is a stable Arduino/LVGL survey UI that can be uploaded to the Elecrow 2.1 inch ESP32 rotary display and later connected to the wider aquarium visualisation system.
