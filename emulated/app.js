(function () {
  "use strict";

  const DEVICE_ID = "crowpanel-emulator-001";

  const screens = {
    START: "start",
    QUESTION: "question",
    COMPLETE: "complete"
  };

  const questions = [
    {
      id: "overall_comparison",
      title: "Overall Comparison",
      body: "Think back to the last events you attended from other laboratories, especially in the same area. On a scale of 1 to 5, how would you rate this TCD event compared to those from other colleges?",
      options: ["1", "2", "3", "4", "5"],
      mapAnswer: Number
    },
    {
      id: "organisation",
      title: "Organisation",
      body: "Think about recent events you attended with other laboratories. How does your experience with TCD compare to theirs for organisation during the event?",
      options: ["1", "2", "3", "4", "5"],
      mapAnswer: Number
    },
    {
      id: "recommendation",
      title: "Recommendation",
      body: "How likely are you to recommend a similar session to a colleague?",
      options: ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"],
      mapAnswer: Number
    },
    {
      id: "fish_type",
      title: "Fish Type",
      body: "Choose a fish to add to the shared aquarium.",
      options: ["Betta", "Tetra", "Gourami", "Shark", "Crab"],
      mapAnswer: String
    },
    {
      id: "fish_colour",
      title: "Fish Colour",
      body: "Choose the colour of your fish.",
      options: ["Cyan", "Blue", "Purple", "Green", "Orange", "Red", "White"],
      mapAnswer: String
    }
  ];

  const display = document.getElementById("display");
  const resetButton = document.getElementById("resetButton");

  const state = createInitialState();

  function createInitialState() {
    return {
      screen: screens.START,
      sessionId: makeId("session"),
      submissionCounter: 0,
      currentQuestionIndex: 0,
      selectedOptionIndices: questions.map(function () {
        return 0;
      }),
      answers: {},
      lastSubmission: null
    };
  }

  function resetState() {
    const nextState = createInitialState();
    state.screen = nextState.screen;
    state.sessionId = nextState.sessionId;
    state.submissionCounter = nextState.submissionCounter;
    state.currentQuestionIndex = nextState.currentQuestionIndex;
    state.selectedOptionIndices = nextState.selectedOptionIndices;
    state.answers = nextState.answers;
    state.lastSubmission = nextState.lastSubmission;
    render();
  }

  function rotateForward() {
    if (state.screen !== screens.QUESTION) {
      return;
    }

    moveSelection(1);
  }

  function rotateBackward() {
    if (state.screen !== screens.QUESTION) {
      return;
    }

    moveSelection(-1);
  }

  function moveSelection(direction) {
    const question = getCurrentQuestion();
    const currentIndex = state.selectedOptionIndices[state.currentQuestionIndex];
    const optionCount = getQuestionOptions(question).length;
    const nextIndex = (currentIndex + direction + optionCount) % optionCount;

    state.selectedOptionIndices[state.currentQuestionIndex] = nextIndex;
    render();
  }

  function pressConfirm() {
    if (state.screen === screens.START) {
      state.screen = screens.QUESTION;
      render();
      return;
    }

    if (state.screen === screens.QUESTION) {
      saveCurrentAnswer();

      if (state.currentQuestionIndex < questions.length - 1) {
        state.currentQuestionIndex += 1;
        render();
        return;
      }

      completeSubmission();
      return;
    }

    if (state.screen === screens.COMPLETE) {
      resetState();
    }
  }

  function backOrReset() {
    if (state.screen === screens.COMPLETE) {
      resetState();
      return;
    }

    if (state.screen === screens.QUESTION && state.currentQuestionIndex > 0) {
      state.currentQuestionIndex -= 1;
      render();
      return;
    }

    state.screen = screens.START;
    render();
  }

  function saveCurrentAnswer() {
    const question = getCurrentQuestion();
    const selectedIndex = state.selectedOptionIndices[state.currentQuestionIndex];
    const selectedValue = getQuestionOptions(question)[selectedIndex];

    state.answers[question.id] = question.mapAnswer(selectedValue);

    if (question.id === "fish_type") {
      resetFishColourSelection();
    }
  }

  function completeSubmission() {
    state.submissionCounter += 1;
    state.lastSubmission = buildSubmission();
    state.screen = screens.COMPLETE;

    render();
  }

  function buildSubmission() {
    return {
      session_id: state.sessionId,
      device_id: DEVICE_ID,
      submission_id: makeSubmissionId(),
      timestamp: new Date().toISOString(),
      answers: {
        overall_comparison: state.answers.overall_comparison,
        organisation: state.answers.organisation,
        recommendation: state.answers.recommendation,
        fish_type: state.answers.fish_type,
        fish_colour: state.answers.fish_colour
      }
    };
  }

  function makeSubmissionId() {
    const paddedCount = String(state.submissionCounter).padStart(3, "0");
    return state.sessionId + "-submission-" + paddedCount;
  }

  function getCurrentQuestion() {
    return questions[state.currentQuestionIndex];
  }

  function getQuestionOptions(question) {
    if (typeof question.getOptions === "function") {
      return question.getOptions();
    }

    return question.options;
  }

  function resetFishColourSelection() {
    const fishColourQuestionIndex = questions.findIndex(function (question) {
      return question.id === "fish_colour";
    });

    if (fishColourQuestionIndex >= 0) {
      state.selectedOptionIndices[fishColourQuestionIndex] = 0;
    }

    delete state.answers.fish_colour;
  }

  function clampSelectedIndex(index, optionCount) {
    if (optionCount <= 0) {
      return 0;
    }

    if (index < optionCount) {
      return index;
    }

    return 0;
  }

  function render() {
    if (state.screen === screens.START) {
      display.innerHTML = renderStartScreen();
    } else if (state.screen === screens.QUESTION) {
      display.innerHTML = renderQuestionScreen();
    } else {
      display.innerHTML = renderCompleteScreen();
    }
  }

  function renderStartScreen() {
    return [
      '<div class="screen">',
      '  <h1 class="title">Feedback Device</h1>',
      '  <p class="copy">Turn the dial to choose. Press to begin.</p>',
      "</div>"
    ].join("");
  }

  function renderQuestionScreen() {
    const question = getCurrentQuestion();
    const options = getQuestionOptions(question);
    const selectedIndex = clampSelectedIndex(state.selectedOptionIndices[state.currentQuestionIndex], options.length);
    const optionMarkup = renderDialOptions(options, selectedIndex);
    const screenClass = hasImageOptions(options) ? "screen screen-image-question" : "screen";

    return [
      '<div class="' + screenClass + '">',
      '  <div class="question-count">Question ',
      String(state.currentQuestionIndex + 1),
      " / ",
      String(questions.length),
      "</div>",
      '  <h2 class="question-title">',
      escapeHtml(question.title),
      "</h2>",
      '  <p class="copy question-copy">',
      escapeHtml(question.body),
      "</p>",
      '  <div class="dial-picker" role="listbox" aria-label="Selected value">',
      optionMarkup,
      "  </div>",
      '  <div class="dial-index">',
      String(selectedIndex + 1),
      " of ",
      String(options.length),
      "</div>",
      "</div>"
    ].join("");
  }

  function renderDialOptions(options, selectedIndex) {
    const previousIndex = wrapOptionIndex(selectedIndex - 1, options.length);
    const nextIndex = wrapOptionIndex(selectedIndex + 1, options.length);

    return [
      renderDialOption("previous", options[previousIndex], false),
      renderDialOption("selected", options[selectedIndex], true),
      renderDialOption("next", options[nextIndex], false)
    ].join("");
  }

  function renderDialOption(position, value, isSelected) {
    const selectedAttr = isSelected ? ' aria-selected="true"' : ' aria-selected="false"';
    const imageClass = getOptionImage(value) ? " dial-option-image" : "";
    const labelClass = getOptionImage(value) ? " dial-option-label visually-hidden" : " dial-option-label";

    return (
      '<div class="dial-option dial-option-' +
      position +
      imageClass +
      '"' +
      selectedAttr +
      ' aria-label="' +
      escapeHtml(getOptionLabel(value)) +
      '"' +
      ' role="option">' +
      renderOptionVisual(value) +
      '<span class="' +
      labelClass +
      '">' +
      escapeHtml(getOptionLabel(value)) +
      "</span>" +
      "</div>"
    );
  }

  function renderOptionVisual(option) {
    const image = getOptionImage(option);
    const frame = getOptionFrame(option);
    const offset = getOptionOffset(option);

    if (!image) {
      return "";
    }

    return (
      '<span class="fish-frame" aria-hidden="true" style="--frame-col: ' +
      String(frame.col) +
      "; --frame-row: " +
      String(frame.row) +
      "; --fish-offset-x: " +
      String(offset.x) +
      "%; --fish-offset-y: " +
      String(offset.y) +
      ';">' +
      '<img class="fish-sprite" src="' +
      escapeHtml(image) +
      '" alt="" />' +
      "</span>"
    );
  }

  function getOptionValue(option) {
    return typeof option === "object" ? option.value : option;
  }

  function getOptionLabel(option) {
    return typeof option === "object" ? option.label : String(option);
  }

  function getOptionImage(option) {
    return typeof option === "object" ? option.image : "";
  }

  function getOptionFrame(option) {
    return typeof option === "object" && option.frame ? option.frame : { col: 0, row: 0 };
  }

  function getOptionOffset(option) {
    return typeof option === "object" && option.offset ? option.offset : { x: 0, y: 0 };
  }

  function hasImageOptions(options) {
    return options.some(function (option) {
      return Boolean(getOptionImage(option));
    });
  }

  function wrapOptionIndex(index, optionCount) {
    return (index + optionCount) % optionCount;
  }

  function renderCompleteScreen() {
    return [
      '<div class="screen">',
      '  <div class="status-ring" aria-hidden="true">',
      checkIcon(),
      "  </div>",
      '  <h2 class="question-title">Submission sent</h2>',
      "</div>"
    ].join("");
  }

  function checkIcon() {
    return [
      '<svg viewBox="0 0 24 24" role="img" aria-label="">',
      '  <path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" stroke-width="2.2" d="M20 6 9 17l-5-5"/>',
      "</svg>"
    ].join("");
  }

  function makeId(prefix) {
    if (window.crypto && typeof window.crypto.randomUUID === "function") {
      return prefix + "-" + window.crypto.randomUUID();
    }

    return prefix + "-" + Date.now().toString(36) + "-" + Math.random().toString(36).slice(2, 10);
  }

  function escapeHtml(value) {
    return String(value)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;")
      .replace(/'/g, "&#039;");
  }

  document.addEventListener("keydown", function (event) {
    if (event.target === resetButton && (event.key === "Enter" || event.key === " ")) {
      return;
    }

    const keyHandlers = {
      ArrowRight: rotateForward,
      ArrowDown: rotateForward,
      ArrowLeft: rotateBackward,
      ArrowUp: rotateBackward,
      Enter: pressConfirm,
      Escape: backOrReset
    };

    const handler = keyHandlers[event.key];

    if (!handler) {
      return;
    }

    event.preventDefault();
    handler();
  });

  resetButton.addEventListener("click", resetState);

  render();
  display.focus({ preventScroll: true });
})();
