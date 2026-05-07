(function () {
  "use strict";

  const DEVICE_ID = "crowpanel-emulator-001";

  const screens = {
    START: "start",
    QUESTION: "question",
    COMPLETE: "complete"
  };

  const fishTypeOptions = [
    {
      value: "betta",
      label: "Betta",
      image: "../../assets/sprites/betta-1.png",
      offset: { x: -7, y: -12 }
    },
    {
      value: "tetra",
      label: "Tetra",
      image: "../../assets/sprites/tetra-1.png",
      offset: { x: -3, y: -8 }
    },
    {
      value: "gourami",
      label: "Gourami",
      image: "../../assets/sprites/gourami-1.png",
      offset: { x: -4, y: -9 }
    },
    {
      value: "shark",
      label: "Shark",
      image: "../../assets/sprites/shark-1.png",
      frame: { col: 0, row: 1 },
      offset: { x: -5, y: -7 }
    }
  ];

  const fishColourOptionsByType = {
    betta: [
      makeFishVariantOption("betta1", "Betta 1"),
      makeFishVariantOption("betta2", "Betta 2"),
      makeFishVariantOption("betta3", "Betta 3"),
      makeFishVariantOption("betta4", "Betta 4"),
      makeFishVariantOption("betta5", "Betta 5")
    ],
    tetra: [
      makeFishVariantOption("tetra1", "Tetra 1"),
      makeFishVariantOption("tetra2", "Tetra 2"),
      makeFishVariantOption("tetra3", "Tetra 3"),
      makeFishVariantOption("tetra4", "Tetra 4"),
      makeFishVariantOption("tetra5", "Tetra 5"),
      makeFishVariantOption("tetra6", "Tetra 6")
    ],
    gourami: [
      makeFishVariantOption("gourami1", "Gourami 1"),
      makeFishVariantOption("gourami2", "Gourami 2"),
      makeFishVariantOption("gourami3", "Gourami 3"),
      makeFishVariantOption("gourami4", "Gourami 4"),
      makeFishVariantOption("gourami5", "Gourami 5"),
      makeFishVariantOption("gourami6", "Gourami 6")
    ],
    shark: [
      makeFishVariantOption("shark1", "Shark", { col: 0, row: 1 })
    ]
  };

  const questions = [
    {
      id: "innovation",
      title: "Innovation",
      options: ["1", "2", "3", "4", "5"],
      mapAnswer: Number
    },
    {
      id: "satisfaction",
      title: "Satisfaction",
      options: ["Very dissatisfied", "Dissatisfied", "Neutral", "Satisfied", "Very satisfied"],
      mapAnswer: String
    },
    {
      id: "recommend",
      title: "Recommend",
      options: ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"],
      mapAnswer: Number
    },
    {
      id: "fish_type",
      title: "Fish type",
      options: fishTypeOptions,
      imageOnly: true,
      mapAnswer: getOptionValue
    },
    {
      id: "fish_colour",
      title: "Fish colour",
      getOptions: getFishColourOptions,
      imageOnly: true,
      mapAnswer: getOptionValue
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
        innovation: state.answers.innovation,
        satisfaction: state.answers.satisfaction,
        recommend: state.answers.recommend,
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

  function getFishColourOptions() {
    const fishType = state.answers.fish_type || "betta";
    return fishColourOptionsByType[fishType] || fishColourOptionsByType.betta;
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
      '  <h1 class="title">Feedback device</h1>',
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

  function makeFishVariantOption(variant, label, frame) {
    const species = getVariantSpecies(variant);

    return {
      value: variant,
      label,
      image: "../../assets/sprites/" + getSpriteFilename(variant),
      frame: frame || { col: 0, row: 0 },
      offset: getVariantOffset(species)
    };
  }

  function getSpriteFilename(variant) {
    return String(variant).replace(/([a-z]+)(\d+)/, "$1-$2") + ".png";
  }

  function getVariantSpecies(variant) {
    const match = String(variant).match(/^(betta|tetra|gourami|shark)/);
    return match ? match[1] : "betta";
  }

  function getVariantOffset(species) {
    const offsets = {
      betta: { x: -7, y: -12 },
      tetra: { x: -3, y: -8 },
      gourami: { x: -4, y: -9 },
      shark: { x: -5, y: -7 }
    };

    return offsets[species] || { x: 0, y: 0 };
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
