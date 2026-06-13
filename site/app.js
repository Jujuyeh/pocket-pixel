const logoPattern = [
  "########################",
  "########################",
  "########.####..##.##.###",
  "####.#.#######.##.###.##",
  "#.##.#.###.#.#.##.###.##",
  "####...#.#.#.##..####..#",
  "##.##..#.#...##.##..#.#.",
  "##..####.###.##.#.#.#.#.",
  "###.##.#.#####.##...#.#.",
  "###.##...########.######",
  "#..##############...####",
  "########################"
];

const keyConfig = {
  ArrowUp: { key: "ArrowUp", code: "ArrowUp", keyCode: 38 },
  ArrowRight: { key: "ArrowRight", code: "ArrowRight", keyCode: 39 },
  ArrowDown: { key: "ArrowDown", code: "ArrowDown", keyCode: 40 },
  ArrowLeft: { key: "ArrowLeft", code: "ArrowLeft", keyCode: 37 },
  z: { key: "z", code: "KeyZ", keyCode: 90 },
  x: { key: "x", code: "KeyX", keyCode: 88 }
};

const keyboardAliases = new Map([
  ["ArrowUp", "ArrowUp"],
  ["ArrowRight", "ArrowRight"],
  ["ArrowDown", "ArrowDown"],
  ["ArrowLeft", "ArrowLeft"],
  ["z", "z"],
  ["a", "z"],
  ["x", "x"],
  ["s", "x"],
  ["b", "x"]
]);

const activeKeys = new Set();
const pointerKeys = new Map();
const buttons = new Map();
let audioMuted = false;

function renderLogo() {
  const logo = document.querySelector("#jujuyeh-logo");
  if (!logo) return;

  logoPattern.join("").split("").forEach((pixel) => {
    const cell = document.createElement("i");
    if (pixel === ".") {
      cell.className = "on";
    }
    logo.append(cell);
  });
}

function gameFileUrl() {
  const params = new URLSearchParams(window.location.search);
  const override = params.get("file");
  if (override) return override;
  return new URL("build/pocket-pixel-latest.hex", window.location.href).href;
}

function setupPlayer() {
  const player = document.querySelector("#player");
  const hexLink = document.querySelector("#hex-link");
  const rom = gameFileUrl();
  const playerUrl = new URL("player.html", window.location.href);
  playerUrl.searchParams.set("file", rom);
  playerUrl.searchParams.set("grid", "none");
  playerUrl.searchParams.set("palette", "default");
  playerUrl.searchParams.set("touch", "0");

  player.src = playerUrl.href;
  hexLink.href = rom;
  player.addEventListener("load", () => {
    attachFrameKeyboardListeners();
    player.focus();
    setPlayerMuted(audioMuted);
  });
}

function updateDeviceScale() {
  const shell = document.querySelector(".device-shell");
  const device = document.querySelector(".device");
  if (!shell || !device) return;

  const available = Math.max(1, Math.min(520, document.documentElement.clientWidth - 24));
  const scale = Math.min(1, available / 520);
  shell.style.setProperty("--device-scale", String(scale));
  shell.style.setProperty("--device-height", `${device.offsetHeight}px`);
  shell.style.width = `${520 * scale}px`;
}

function eventTargets() {
  const iframe = document.querySelector("#player");
  const targets = [window];
  targets.push(...frameTargets(iframe));

  return targets;
}

function frameTargets(iframe = document.querySelector("#player")) {
  const targets = [];

  try {
    if (iframe.contentWindow) targets.push(iframe.contentWindow);
    const doc = iframe.contentDocument;
    if (doc) {
      targets.push(doc);
      const canvas = doc.querySelector("canvas");
      if (canvas) targets.push(canvas);
    }
  } catch (_) {
    // Same-origin Pages builds can receive synthetic input. Cross-origin local
    // previews still show the controller state but cannot forward input.
  }

  return targets;
}

function dispatchKey(type, logicalKey) {
  dispatchKeyToTargets(type, logicalKey, eventTargets());
}

function dispatchFrameKey(type, logicalKey) {
  dispatchKeyToTargets(type, logicalKey, frameTargets());
}

function focusPlayerCanvas() {
  const iframe = document.querySelector("#player");
  try {
    iframe?.focus();
    const canvas = iframe.contentDocument?.querySelector("canvas");
    canvas?.focus();
  } catch (_) {
    // Same-origin Pages builds can focus the canvas. Cross-origin previews cannot.
  }
}

function playerAudioApi() {
  const iframe = document.querySelector("#player");
  try {
    return iframe.contentWindow?.PocketPixelAudio;
  } catch (_) {
    return null;
  }
}

function postPlayerAudioMessage(message) {
  const iframe = document.querySelector("#player");
  try {
    iframe.contentWindow?.postMessage(message, window.location.origin);
  } catch (_) {
    // Cross-origin previews cannot receive direct postMessage control.
  }
}

function unlockPlayerAudio() {
  const api = playerAudioApi();
  if (api?.unlock) {
    return api.unlock();
  }

  postPlayerAudioMessage({ type: "pocket-pixel:audio-unlock" });
  return Promise.resolve();
}

function setPlayerMuted(muted) {
  audioMuted = Boolean(muted);
  updateAudioIndicator();
  const api = playerAudioApi();
  if (api?.setMuted) {
    return api.setMuted(audioMuted);
  }

  postPlayerAudioMessage({ type: "pocket-pixel:audio-muted", muted: audioMuted });
  return Promise.resolve(audioMuted);
}

function togglePlayerMuted() {
  return setPlayerMuted(!audioMuted);
}

function updateAudioIndicator() {
  const button = document.querySelector(".audio-toggle");
  if (!button) return;

  button.classList.toggle("is-muted", audioMuted);
  button.setAttribute("aria-pressed", String(audioMuted));
  button.setAttribute("aria-label", audioMuted ? "Unmute audio" : "Mute audio");
}

function dispatchKeyToTargets(type, logicalKey, targets) {
  const config = keyConfig[logicalKey];
  if (!config) return;

  const init = {
    key: config.key,
    code: config.code,
    keyCode: config.keyCode,
    which: config.keyCode,
    bubbles: true,
    cancelable: true
  };

  for (const target of targets) {
    target.dispatchEvent(new KeyboardEvent(type, init));
  }
}

function setButtonState(logicalKey, pressed) {
  const control = buttons.get(logicalKey);
  if (control) {
    control.classList.toggle("is-active", pressed);
  }
}

function pressKey(logicalKey) {
  if (activeKeys.has(logicalKey)) return;
  activeKeys.add(logicalKey);
  setButtonState(logicalKey, true);
  dispatchKey("keydown", logicalKey);
}

function releaseKey(logicalKey) {
  if (!activeKeys.has(logicalKey)) return;
  activeKeys.delete(logicalKey);
  setButtonState(logicalKey, false);
  dispatchKey("keyup", logicalKey);
}

function keyFromPoint(x, y) {
  const element = document.elementFromPoint(x, y);
  const control = element?.closest?.("[data-key]");
  return control?.dataset.key;
}

function setupControls() {
  document.querySelectorAll("[data-key]").forEach((control) => {
    buttons.set(control.dataset.key, control);

    control.addEventListener("pointerdown", (event) => {
      event.preventDefault();
      unlockPlayerAudio();
      control.setPointerCapture(event.pointerId);
      pointerKeys.set(event.pointerId, control.dataset.key);
      pressKey(control.dataset.key);
    });

    control.addEventListener("pointermove", (event) => {
      event.preventDefault();
      const previous = pointerKeys.get(event.pointerId);
      const next = keyFromPoint(event.clientX, event.clientY);
      if (previous === next) return;
      if (previous) releaseKey(previous);
      if (next) {
        pointerKeys.set(event.pointerId, next);
        pressKey(next);
      } else {
        pointerKeys.delete(event.pointerId);
      }
    });

    const stop = (event) => {
      event.preventDefault();
      const key = pointerKeys.get(event.pointerId);
      if (key) releaseKey(key);
      pointerKeys.delete(event.pointerId);
    };

    control.addEventListener("pointerup", stop);
    control.addEventListener("pointercancel", stop);
  });

  window.addEventListener("keydown", (event) => {
    if (event.key.toLowerCase() === "m") {
      event.preventDefault();
      focusPlayerCanvas();
      togglePlayerMuted();
      return;
    }

    const logicalKey = keyboardAliases.get(event.key);
    if (!logicalKey) return;
    event.preventDefault();
    focusPlayerCanvas();
    unlockPlayerAudio();
    setButtonState(logicalKey, true);
    if (event.isTrusted) {
      dispatchFrameKey("keydown", logicalKey);
    }
  });

  window.addEventListener("keyup", (event) => {
    const logicalKey = keyboardAliases.get(event.key);
    if (!logicalKey) return;
    event.preventDefault();
    if (!activeKeys.has(logicalKey)) {
      setButtonState(logicalKey, false);
    }
    if (event.isTrusted) {
      dispatchFrameKey("keyup", logicalKey);
    }
  });
}

function setupAudioControl() {
  const button = document.querySelector(".audio-toggle");
  if (!button) return;

  updateAudioIndicator();
  button.addEventListener("click", async (event) => {
    event.preventDefault();
    await togglePlayerMuted();
  });
}

function setupResponsiveDevice() {
  updateDeviceScale();
  window.addEventListener("resize", updateDeviceScale);
  if (window.ResizeObserver) {
    new ResizeObserver(updateDeviceScale).observe(document.querySelector(".device"));
  }
}

function reflectKeyboardEvent(event, pressed) {
  const logicalKey = keyboardAliases.get(event.key);
  if (!logicalKey) return;
  if (!pressed && activeKeys.has(logicalKey)) return;
  setButtonState(logicalKey, pressed);
}

function attachFrameKeyboardListeners() {
  const iframe = document.querySelector("#player");
  try {
    focusPlayerCanvas();
    iframe.contentWindow.addEventListener("keydown", (event) => reflectKeyboardEvent(event, true));
    iframe.contentWindow.addEventListener("keyup", (event) => reflectKeyboardEvent(event, false));
  } catch (_) {
    // Cross-origin previews cannot mirror iframe keyboard state.
  }
}

renderLogo();
setupPlayer();
setupControls();
setupAudioControl();
setupResponsiveDevice();
