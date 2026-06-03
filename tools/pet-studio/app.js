const canvas = document.querySelector("#pixelCanvas");
const ctx = canvas.getContext("2d");

const els = {
  status: document.querySelector("#status"),
  name: document.querySelector("#spriteName"),
  width: document.querySelector("#spriteWidth"),
  height: document.querySelector("#spriteHeight"),
  source: document.querySelector("#sourceInput"),
  output: document.querySelector("#output"),
  projectAssets: document.querySelector("#projectAssets"),
  zoom: document.querySelector("#zoom"),
  brushSize: document.querySelector("#brushSize"),
  brushSizeValue: document.querySelector("#brushSizeValue"),
  grid: document.querySelector("#gridToggle"),
  stats: document.querySelector("#stats"),
  frameInfo: document.querySelector("#frameInfo"),
  cursorPosition: document.querySelector("#cursorPosition"),
  sliceWidth: document.querySelector("#sliceWidth"),
  sliceHeight: document.querySelector("#sliceHeight"),
  pngInput: document.querySelector("#pngInput"),
  profileSelect: document.querySelector("#profileSelect"),
  newProfileName: document.querySelector("#newProfileName"),
  profileSummary: document.querySelector("#profileSummary"),
  profileForm: document.querySelector("#profileForm"),
  profileRaw: document.querySelector("#profileRaw"),
  profileJson: document.querySelector("#profileJson"),
  bannerPreview: document.querySelector("#bannerPreview"),
  bannerPath: document.querySelector("#bannerPath"),
  bannerInput: document.querySelector("#bannerInput"),
  projectAudio: document.querySelector("#projectAudio"),
  audioName: document.querySelector("#audioName"),
  audioRoll: document.querySelector("#audioRoll"),
  audioEvents: document.querySelector("#audioEvents"),
  audioStats: document.querySelector("#audioStats"),
  audioOutput: document.querySelector("#audioOutput"),
  preferencesWorkspace: document.querySelector("#preferencesWorkspace"),
  spriteWorkspace: document.querySelector("#spriteWorkspace"),
  audioWorkspace: document.querySelector("#audioWorkspace")
};

let state = {
  name: "sprite",
  frames: [blankFrame(16, 16)],
  frameIndex: 0,
  tool: "draw",
  drawing: false,
  strokeMode: "draw",
  lastPoint: null,
  lineStart: null,
  linePreview: null,
  projectAssets: [],
  frameClipboard: null,
  profiles: [],
  activeProfile: null,
  tones: [],
  audioEvents: [],
  audioView: "roll",
  preferencesView: "form",
  rollDrag: null
};

const AUDIO_NOTES = [
  "NOTE_C6", "NOTE_B5", "NOTE_AS5", "NOTE_A5", "NOTE_GS5", "NOTE_G5", "NOTE_FS5", "NOTE_F5", "NOTE_E5", "NOTE_DS5", "NOTE_D5", "NOTE_CS5",
  "NOTE_C5", "NOTE_B4", "NOTE_AS4", "NOTE_A4", "NOTE_GS4", "NOTE_G4", "NOTE_FS4", "NOTE_F4", "NOTE_E4", "NOTE_DS4", "NOTE_D4", "NOTE_CS4",
  "NOTE_C4", "NOTE_B3", "NOTE_AS3", "NOTE_A3", "NOTE_GS3", "NOTE_G3", "NOTE_FS3", "NOTE_F3", "NOTE_E3", "NOTE_DS3", "NOTE_D3", "NOTE_CS3",
  "NOTE_C3", "NOTE_B2", "NOTE_AS2", "NOTE_A2", "NOTE_GS2", "NOTE_G2", "NOTE_FS2", "NOTE_F2", "NOTE_E2", "NOTE_DS2", "NOTE_D2", "NOTE_CS2",
  "NOTE_C2"
];
const AUDIO_ROW_HEIGHT = 18;
const AUDIO_MS_PER_PX = 1.25;
const AUDIO_SNAP_MS = 25;

// Frames use booleans where true means a black Arduboy pixel.
function blankFrame(width, height) {
  return {
    width,
    height,
    pixels: Array.from({ length: height }, () => Array(width).fill(false))
  };
}

function currentFrame() {
  return state.frames[state.frameIndex];
}

function cloneFrame(frame) {
  return {
    width: frame.width,
    height: frame.height,
    pixels: frame.pixels.map((row) => row.slice())
  };
}

function setStatus(message) {
  els.status.textContent = message;
}

// Mode switches only change visible workspaces; state remains loaded.
function setMode(mode) {
  document.querySelectorAll("[data-mode]").forEach((button) => {
    button.classList.toggle("active", button.dataset.mode === mode);
  });
  els.preferencesWorkspace.classList.toggle("hidden", mode !== "preferences");
  els.spriteWorkspace.classList.toggle("hidden", mode !== "sprite");
  els.audioWorkspace.classList.toggle("hidden", mode !== "audio");
  if (mode === "audio") {
    renderAudioRoll();
  }
}

function setAudioView(view) {
  state.audioView = view;
  document.querySelectorAll("[data-audio-view]").forEach((button) => {
    button.classList.toggle("active", button.dataset.audioView === view);
  });
  els.audioRoll.classList.toggle("hidden", view !== "roll");
  els.audioEvents.classList.toggle("hidden", view !== "events");
}

// Preferences can be edited as a generated form or raw JSON.
function setPreferencesView(view) {
  if (els.profileJson.value.trim()) {
    if (view === "raw") {
      syncProfileRawFromForm();
    } else {
      syncProfileFormFromRaw();
    }
  }
  state.preferencesView = view;
  document.querySelectorAll("[data-preferences-view]").forEach((button) => {
    button.classList.toggle("active", button.dataset.preferencesView === view);
  });
  els.profileForm.classList.toggle("hidden", view !== "form");
  els.profileRaw.classList.toggle("hidden", view !== "raw");
}

function syncFields() {
  const frame = currentFrame();
  els.name.value = state.name;
  els.width.value = frame.width;
  els.height.value = frame.height;
  els.sliceWidth.value = frame.width;
  els.sliceHeight.value = frame.height;
}

// Render the active sprite frame at editor zoom, plus optional line preview.
function render() {
  const frame = currentFrame();
  const displayFrame = state.linePreview || frame;
  const zoom = Number(els.zoom.value);
  canvas.width = frame.width * zoom;
  canvas.height = frame.height * zoom;
  ctx.fillStyle = "#ffffff";
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  ctx.fillStyle = "#000000";
  for (let y = 0; y < displayFrame.height; y += 1) {
    for (let x = 0; x < displayFrame.width; x += 1) {
      if (displayFrame.pixels[y][x]) {
        ctx.fillRect(x * zoom, y * zoom, zoom, zoom);
      }
    }
  }

  if (state.lastPoint && !state.linePreview) {
    const size = Number(els.brushSize.value);
    const radius = Math.floor(size / 2);
    ctx.strokeStyle = "rgba(22, 108, 134, 0.82)";
    ctx.lineWidth = 1;
    ctx.strokeRect(
      (state.lastPoint.x - radius) * zoom + 0.5,
      (state.lastPoint.y - radius) * zoom + 0.5,
      size * zoom,
      size * zoom
    );
  }

  if (els.grid.checked && zoom >= 6) {
    ctx.strokeStyle = "rgba(0, 0, 0, 0.16)";
    ctx.lineWidth = 1;
    for (let x = 0; x <= frame.width; x += 1) {
      ctx.beginPath();
      ctx.moveTo(x * zoom + 0.5, 0);
      ctx.lineTo(x * zoom + 0.5, canvas.height);
      ctx.stroke();
    }
    for (let y = 0; y <= frame.height; y += 1) {
      ctx.beginPath();
      ctx.moveTo(0, y * zoom + 0.5);
      ctx.lineTo(canvas.width, y * zoom + 0.5);
      ctx.stroke();
    }
  }

  const bytes = 2 + frame.width * Math.ceil(frame.height / 8);
  const blackPixels = frame.pixels.flat().filter(Boolean).length;
  els.stats.textContent = [
    `${frame.width}x${frame.height}`,
    `${bytes} flash bytes as Arduboy bitmap`,
    `${blackPixels} black pixels`,
    `${state.frames.length} frame(s)`
  ].join("\n");
  els.frameInfo.textContent = `Frame ${state.frameIndex + 1} / ${state.frames.length}`;
}

function refresh(message) {
  syncFields();
  render();
  if (message) {
    setStatus(message);
  }
}

// Translate browser coordinates into sprite pixel coordinates.
function canvasPixel(event) {
  const frame = currentFrame();
  const rect = canvas.getBoundingClientRect();
  const x = Math.floor((event.clientX - rect.left) * canvas.width / rect.width / Number(els.zoom.value));
  const y = Math.floor((event.clientY - rect.top) * canvas.height / rect.height / Number(els.zoom.value));
  if (x < 0 || y < 0 || x >= frame.width || y >= frame.height) {
    return null;
  }
  return { x, y };
}

function setCursorPosition(point) {
  els.cursorPosition.textContent = point ? `x ${point.x} y ${point.y}` : "x -- y --";
}

function paintPoint(frame, point, mode, size) {
  const radius = Math.floor(size / 2);
  for (let y = point.y - radius; y <= point.y + radius; y += 1) {
    for (let x = point.x - radius; x <= point.x + radius; x += 1) {
      if (x < 0 || y < 0 || x >= frame.width || y >= frame.height) {
        continue;
      }
      if (mode === "draw" || mode === "line") {
        frame.pixels[y][x] = true;
      } else if (mode === "erase") {
        frame.pixels[y][x] = false;
      } else {
        frame.pixels[y][x] = !frame.pixels[y][x];
      }
    }
  }
}

// Integer Bresenham keeps line drawing deterministic at one-bit resolution.
function paintLine(frame, from, to, mode, size) {
  let x0 = from.x;
  let y0 = from.y;
  const x1 = to.x;
  const y1 = to.y;
  const dx = Math.abs(x1 - x0);
  const sx = x0 < x1 ? 1 : -1;
  const dy = -Math.abs(y1 - y0);
  const sy = y0 < y1 ? 1 : -1;
  let error = dx + dy;

  while (true) {
    paintPoint(frame, { x: x0, y: y0 }, mode, size);
    if (x0 === x1 && y0 === y1) {
      break;
    }
    const e2 = 2 * error;
    if (e2 >= dy) {
      error += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      error += dx;
      y0 += sy;
    }
  }
}

function applyStroke(point) {
  const frame = currentFrame();
  const size = Number(els.brushSize.value);
  if (state.lastPoint) {
    paintLine(frame, state.lastPoint, point, state.strokeMode, size);
  } else {
    paintPoint(frame, point, state.strokeMode, size);
  }
  state.lastPoint = point;
  render();
}

// C arrays may use decimal or hex bytes; symbols are ignored by callers.
function parseNumber(token) {
  if (/^0x[0-9a-f]+$/i.test(token)) {
    return Number.parseInt(token, 16);
  }
  if (/^\d+$/.test(token)) {
    return Number.parseInt(token, 10);
  }
  return null;
}

// Decode Arduboy page-ordered bitmap data into editable row-major pixels.
function decodeArduboy(name, values) {
  if (values.length < 3) {
    throw new Error("Need width, height, and bitmap bytes.");
  }
  const width = values[0];
  const height = values[1];
  const pages = Math.ceil(height / 8);
  const frameBytes = width * pages;
  const data = values.slice(2);
  if (data.length < frameBytes || data.length % frameBytes !== 0) {
    throw new Error(`Expected a multiple of ${frameBytes} bitmap bytes, got ${data.length}.`);
  }

  const frames = [];
  for (let frameStart = 0; frameStart < data.length; frameStart += frameBytes) {
    const frame = blankFrame(width, height);
    for (let page = 0; page < pages; page += 1) {
      for (let x = 0; x < width; x += 1) {
        const byte = data[frameStart + page * width + x];
        for (let bit = 0; bit < 8; bit += 1) {
          const y = page * 8 + bit;
          if (y < height) {
            frame.pixels[y][x] = ((byte >> bit) & 1) === 0;
          }
        }
      }
    }
    frames.push(frame);
  }

  state.name = name || "sprite";
  state.frames = frames;
  state.frameIndex = 0;
  refresh(`Imported ${state.name} (${frames.length} frame(s))`);
}

function decodeArduboyData(name, width, height, data) {
    decodeArduboy(name, [width, height, ...data]);
}

// Asset loading goes through the local server so drafts and source arrays merge.
async function loadProjectAssets() {
  try {
    const response = await fetch("/api/assets", { cache: "no-store" });
    if (!response.ok) {
      throw new Error(`asset index HTTP ${response.status}`);
    }
    const payload = await response.json();
    state.projectAssets = payload.assets || [];
    els.projectAssets.replaceChildren();
    if (state.projectAssets.length === 0) {
      const option = new Option("No project sprites found", "");
      els.projectAssets.add(option);
      setStatus("No project sprites found");
      return;
    }
    state.projectAssets.forEach((asset) => {
      const frames = asset.frameCount || 1;
      const option = new Option(`${asset.name} (${asset.width}x${asset.height}, ${frames} frame(s), ${asset.bytes} bytes)`, asset.name);
      els.projectAssets.add(option);
    });
    loadSelectedProjectAsset();
  } catch (error) {
    const option = new Option("Project assets unavailable", "");
    els.projectAssets.replaceChildren(option);
    setStatus("Project assets unavailable; use make pet-studio");
  }
}

function loadSelectedProjectAsset() {
  const name = els.projectAssets.value;
  const asset = state.projectAssets.find((item) => item.name === name);
  if (!asset) {
    throw new Error("Select a project sprite first.");
  }
  const numericMatch = asset.name.match(/^(.*?)(\d+)$/);
  if (asset.frameCount === 1 && numericMatch) {
    // Legacy assets named foo1/foo2 are grouped as frames when dimensions match.
    const prefix = numericMatch[1];
    const grouped = state.projectAssets
      .filter((item) => {
        const match = item.name.match(/^(.*?)(\d+)$/);
        return match
          && match[1] === prefix
          && item.frameCount === 1
          && item.width === asset.width
          && item.height === asset.height;
      })
      .sort((left, right) => Number(left.name.match(/(\d+)$/)[1]) - Number(right.name.match(/(\d+)$/)[1]));
    if (grouped.length > 1) {
      decodeArduboyData(prefix.replace(/_$/, "") || asset.name, asset.width, asset.height, grouped.flatMap((item) => item.data));
      return;
    }
  }
  decodeArduboyData(asset.name, asset.width, asset.height, asset.data);
}

// Profiles are loaded whole so unknown future personality fields stay editable.
async function loadProfiles() {
  try {
    const response = await fetch("/api/profiles", { cache: "no-store" });
    if (!response.ok) {
      throw new Error(`profile index HTTP ${response.status}`);
    }
    const payload = await response.json();
    state.profiles = payload.profiles || [];
    els.profileSelect.replaceChildren();
    if (state.profiles.length === 0) {
      els.profileSelect.add(new Option("No profiles found", ""));
      return;
    }
    state.profiles.forEach((profile) => {
      els.profileSelect.add(new Option(`${profile.name} (${profile.slug})`, profile.slug));
    });
    selectProfile(state.profiles[0].slug);
  } catch (error) {
    els.profileSelect.replaceChildren(new Option("Profiles unavailable", ""));
    setStatus("Profiles unavailable");
  }
}

function selectProfile(slug) {
  const profile = state.profiles.find((item) => item.slug === slug);
  if (!profile) {
    return;
  }
  state.activeProfile = profile;
  els.profileSelect.value = slug;
  els.profileSummary.textContent = [
    profile.path,
    `${profile.species || "cat"} profile`,
    profile.data?.description || ""
  ].filter(Boolean).join("\n");
  els.profileJson.value = JSON.stringify(profile.data, null, 2);
  renderProfileForm(profile.data);
  updateBannerPreview(profile.data);
}

function parseProfileRaw() {
    return JSON.parse(els.profileJson.value);
}

// Form fields store their intended JSON type in data attributes.
function profileInputValue(input) {
  if (input.dataset.type === "number") {
    return Number(input.value);
  }
  if (input.dataset.type === "array") {
    return input.value
      .split(/\r?\n/)
      .map((line) => line.trim())
      .filter(Boolean);
  }
  return input.value;
}

// Create missing objects while writing nested form values back to JSON.
function setPathValue(target, path, value) {
  let cursor = target;
  for (let i = 0; i < path.length - 1; i += 1) {
    const key = path[i];
    if (!cursor[key] || typeof cursor[key] !== "object" || Array.isArray(cursor[key])) {
      cursor[key] = {};
    }
    cursor = cursor[key];
  }
  cursor[path[path.length - 1]] = value;
}

function bannerPathFromProfile(data) {
  return data?.assets?.banner || "assets/fx/banner.png";
}

function updateBannerPreview(data) {
  const path = bannerPathFromProfile(data);
  els.bannerPath.textContent = path;
  els.bannerPreview.src = `/${path}?v=${Date.now()}`;
}

// Form is authoritative while editing in form mode.
function syncProfileRawFromForm() {
  if (!els.profileForm.dataset.ready) {
    return;
  }
  const data = parseProfileRaw();
  els.profileForm.querySelectorAll("[data-profile-path]").forEach((input) => {
    setPathValue(data, input.dataset.profilePath.split("."), profileInputValue(input));
  });
  els.profileJson.value = JSON.stringify(data, null, 2);
  updateBannerPreview(data);
}

// Raw JSON is authoritative while editing in raw mode.
function syncProfileFormFromRaw() {
  const data = parseProfileRaw();
  renderProfileForm(data);
  updateBannerPreview(data);
}

function profileLabel(key) {
  return key
    .replace(/([a-z])([A-Z])/g, "$1 $2")
    .replace(/^./, (letter) => letter.toUpperCase());
}

function appendProfileField(container, path, value) {
  const label = document.createElement("label");
  label.textContent = profileLabel(path[path.length - 1]);
  const isLongText = typeof value === "string" && value.length > 70;
  const input = isLongText || Array.isArray(value) ? document.createElement("textarea") : document.createElement("input");
  input.dataset.profilePath = path.join(".");
  if (typeof value === "number") {
    input.type = "number";
    input.dataset.type = "number";
    input.value = value;
  } else if (Array.isArray(value)) {
    input.dataset.type = "array";
    input.value = value.join("\n");
  } else {
    input.type = "text";
    input.dataset.type = "string";
    input.value = value ?? "";
  }
  input.addEventListener("input", () => {
    if (state.preferencesView === "form") {
      syncProfileRawFromForm();
    }
  });
  label.append(input);
  container.append(label);
}

// Nested profile objects become nested sections instead of flattened key names.
function appendProfileSection(parent, title, value, path = []) {
  const section = document.createElement("section");
  section.className = "profile-section";
  const heading = document.createElement("h2");
  heading.textContent = title;
  section.append(heading);

  const grid = document.createElement("div");
  grid.className = "profile-field-grid";
  Object.entries(value).forEach(([key, child]) => {
    const childPath = path.concat(key);
    if (child && typeof child === "object" && !Array.isArray(child)) {
      appendProfileSection(section, profileLabel(key), child, childPath);
    } else {
      appendProfileField(grid, childPath, child);
    }
  });
  if (grid.childElementCount > 0) {
    section.append(grid);
  }
  parent.append(section);
}

// Keep primary identity fields first, then render all remaining profile data.
function renderProfileForm(data) {
  els.profileForm.replaceChildren();
  els.profileForm.dataset.ready = "1";

  const primary = {};
  ["schemaVersion", "name", "species", "description", "notes"].forEach((key) => {
    if (Object.prototype.hasOwnProperty.call(data, key)) {
      primary[key] = data[key];
    }
  });
  appendProfileSection(els.profileForm, "Profile", primary);

  Object.entries(data).forEach(([key, value]) => {
    if (Object.prototype.hasOwnProperty.call(primary, key)) {
      return;
    }
    if (value && typeof value === "object" && !Array.isArray(value)) {
      appendProfileSection(els.profileForm, profileLabel(key), value, [key]);
    } else {
      const wrapper = {};
      wrapper[key] = value;
      appendProfileSection(els.profileForm, profileLabel(key), wrapper);
    }
  });
}

async function saveCurrentProfile() {
  if (state.preferencesView === "form") {
    syncProfileRawFromForm();
  } else {
    syncProfileFormFromRaw();
  }
  const data = parseProfileRaw();
  const slug = state.activeProfile?.slug || data.name || "profile";
  const response = await fetch("/api/save-profile", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ slug, data })
  });
  const payload = await response.json();
  if (!response.ok || !payload.ok) {
    throw new Error(payload.error || "Could not save profile.");
  }
  await loadProfiles();
  selectProfile(payload.slug);
  setStatus(`Saved ${payload.path}`);
}

function readFileDataUrl(file) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.addEventListener("load", () => resolve(reader.result));
    reader.addEventListener("error", () => reject(reader.error || new Error("Could not read file.")));
    reader.readAsDataURL(file);
  });
}

// Banner save reuses the sprite canvas, so it exports the current frame as PNG.
function frameToPngDataUrl(frame) {
  const scratch = document.createElement("canvas");
  scratch.width = frame.width;
  scratch.height = frame.height;
  const sctx = scratch.getContext("2d");
  sctx.fillStyle = "#ffffff";
  sctx.fillRect(0, 0, frame.width, frame.height);
  sctx.fillStyle = "#000000";
  for (let y = 0; y < frame.height; y += 1) {
    for (let x = 0; x < frame.width; x += 1) {
      if (frame.pixels[y][x]) {
        sctx.fillRect(x, y, 1, 1);
      }
    }
  }
  return scratch.toDataURL("image/png");
}

// Save banner first, then update and save the profile reference to it.
async function saveProfileBannerDataUrl(dataUrl) {
  if (!state.activeProfile) {
    throw new Error("Select a profile first.");
  }
  if (state.preferencesView === "form") {
    syncProfileRawFromForm();
  }
  const data = parseProfileRaw();
  const path = bannerPathFromProfile(data);
  const response = await fetch("/api/save-banner", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ slug: state.activeProfile.slug, path, dataUrl })
  });
  const payload = await response.json();
  if (!response.ok || !payload.ok) {
    throw new Error(payload.error || "Could not save banner.");
  }
  data.assets = data.assets || {};
  data.assets.banner = payload.path;
  els.profileJson.value = JSON.stringify(data, null, 2);
  renderProfileForm(data);
  updateBannerPreview(data);
  await saveCurrentProfile();
  return payload.path;
}

async function saveProfileBanner() {
  if (!state.activeProfile) {
    throw new Error("Select a profile first.");
  }
  if (!els.bannerInput.files.length) {
    throw new Error("Choose a PNG banner first.");
  }
  const dataUrl = await readFileDataUrl(els.bannerInput.files[0]);
  const path = await saveProfileBannerDataUrl(dataUrl);
  els.bannerInput.value = "";
  setStatus(`Saved banner ${path}`);
}

async function createProfile() {
  // New profiles clone the active one to keep all current schema keys present.
  const name = els.newProfileName.value.trim();
  if (!name) {
    throw new Error("Enter a profile name first.");
  }
  const base = state.activeProfile?.data || {
    schemaVersion: 1,
    species: "cat",
    traits: {},
    behavior: {},
    assets: {},
    audio: {
      menuMelody: ["E5", "E5", "F5", "G5", "G5", "F5", "E5", "D5"]
    }
  };
  const data = JSON.parse(JSON.stringify(base));
  data.name = name;
  data.description = data.description || "";
  data.notes = data.notes || "Created from Pet Studio.";
  const response = await fetch("/api/save-profile", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ slug: name, data })
  });
  const payload = await response.json();
  if (!response.ok || !payload.ok) {
    throw new Error(payload.error || "Could not create profile.");
  }
  els.newProfileName.value = "";
  await loadProfiles();
  selectProfile(payload.slug);
  setStatus(`Created ${payload.path}`);
}

// Load source tones and saved drafts into one selectable list.
async function loadAudio() {
  try {
    const response = await fetch("/api/audio", { cache: "no-store" });
    if (!response.ok) {
      throw new Error(`audio index HTTP ${response.status}`);
    }
    const payload = await response.json();
    const tones = (payload.tones || []).map((tone) => ({ ...tone, draft: false }));
    const drafts = (payload.drafts || []).map((tone) => ({ ...tone, draft: true }));
    state.tones = tones.concat(drafts);
    els.projectAudio.replaceChildren();
    if (state.tones.length === 0) {
      els.projectAudio.add(new Option("No sounds found", ""));
      return;
    }
    state.tones.forEach((tone) => {
      const label = `${tone.name}${tone.draft ? " draft" : ""} (${tone.events.length} event(s))`;
      els.projectAudio.add(new Option(label, tone.name));
    });
    loadSelectedAudio();
  } catch (error) {
    els.projectAudio.replaceChildren(new Option("Sounds unavailable", ""));
    setStatus("Sounds unavailable");
  }
}

function loadSelectedAudio() {
  const tone = state.tones.find((item) => item.name === els.projectAudio.value);
  if (!tone) {
    throw new Error("Select a project sound first.");
  }
  state.audioEvents = tone.events.map((event) => ({ ...event }));
  els.audioName.value = tone.name;
  renderAudioEvents();
  exportAudioJson();
  setStatus(`Loaded ${tone.name}`);
}

// Event view is the exact ArduboyTones sequence: note/rest plus duration.
function renderAudioEvents() {
  els.audioEvents.replaceChildren();
  state.audioEvents.forEach((event, index) => {
    const row = document.createElement("div");
    row.className = "audio-row";

    const noteLabel = document.createElement("label");
    noteLabel.textContent = "Note";
    const note = document.createElement("input");
    note.value = event.note;
    note.addEventListener("input", () => {
      state.audioEvents[index].note = note.value.trim() || "NOTE_REST";
      updateAudioStats();
    });
    noteLabel.append(note);

    const durationLabel = document.createElement("label");
    durationLabel.textContent = "Ms";
    const duration = document.createElement("input");
    duration.type = "number";
    duration.min = "1";
    duration.max = "2000";
    duration.value = event.duration;
    duration.addEventListener("input", () => {
      state.audioEvents[index].duration = Number(duration.value);
      updateAudioStats();
    });
    durationLabel.append(duration);

    const remove = document.createElement("button");
    remove.textContent = "X";
    remove.addEventListener("click", () => {
      state.audioEvents.splice(index, 1);
      renderAudioEvents();
    });

    row.append(noteLabel, durationLabel, remove);
    els.audioEvents.append(row);
  });
  updateAudioStats();
  renderAudioRoll();
}

function updateAudioStats() {
  const duration = state.audioEvents.reduce((sum, event) => sum + Number(event.duration || 0), 0);
  const bytes = (state.audioEvents.length * 2 + 1) * 2;
  els.audioStats.textContent = [
    `${state.audioEvents.length} event(s)`,
    `${duration} ms total`,
    `${bytes} flash bytes as ArduboyTones`
  ].join("\n");
}

function snapMs(ms) {
    return Math.max(0, Math.round(ms / AUDIO_SNAP_MS) * AUDIO_SNAP_MS);
}

// Convert sequential ArduboyTones events into positioned piano-roll notes.
function audioSequenceNotes() {
  let start = 0;
  const notes = [];
  audioDraft().events.forEach((event) => {
    const duration = Number(event.duration || 0);
    if (event.note !== "NOTE_REST" && duration > 0) {
      notes.push({ note: event.note, start, duration });
    }
    start += duration;
  });
  return notes;
}

// Convert piano-roll notes back into a sequential list with inserted rests.
function setAudioEventsFromSequence(notes) {
  const sorted = notes
    .filter((note) => note.duration > 0 && AUDIO_NOTES.includes(note.note))
    .sort((left, right) => left.start - right.start);
  const events = [];
  let cursor = 0;
  sorted.forEach((note) => {
    const start = Math.max(note.start, cursor);
    if (start > cursor) {
      events.push({ note: "NOTE_REST", duration: start - cursor });
    }
    events.push({ note: note.note, duration: note.duration });
    cursor = start + note.duration;
  });
  state.audioEvents = events.length ? events : [{ note: "NOTE_REST", duration: 50 }];
  renderAudioEvents();
}

function removeRollNote(target) {
  const noteToRemove = {
    note: target.dataset.note,
    start: Number(target.dataset.start),
    duration: Number(target.dataset.duration)
  };
  const next = audioSequenceNotes().filter((note) => !(
    note.note === noteToRemove.note
    && note.start === noteToRemove.start
    && note.duration === noteToRemove.duration
  ));
  setAudioEventsFromSequence(next);
  setStatus(`Removed ${noteToRemove.note}`);
}

// Roll grows with the song but is never narrower than the visible workspace.
function rollWidth(notes) {
  const total = state.audioEvents.reduce((sum, event) => sum + Number(event.duration || 0), 0);
  const lastNoteEnd = notes.reduce((max, note) => Math.max(max, note.start + note.duration), 0);
  const visibleWidth = Math.max(860, els.audioRoll.clientWidth - 64);
  const contentWidth = Math.ceil((Math.max(total, lastNoteEnd) + 600) / AUDIO_MS_PER_PX);
  return Math.max(visibleWidth, contentWidth);
}

function renderAudioRoll(preview = null) {
  els.audioRoll.replaceChildren();

  const notes = audioSequenceNotes();
  const width = rollWidth(notes.concat(preview ? [preview] : []));
  const labels = document.createElement("div");
  labels.className = "piano-labels";
  const grid = document.createElement("div");
  grid.className = "roll-grid";
  grid.style.width = `${width}px`;

  AUDIO_NOTES.forEach((note) => {
    const label = document.createElement("div");
    label.className = `piano-label${note.includes("S") ? " sharp" : ""}`;
    label.textContent = note.replace("NOTE_", "");
    labels.append(label);

    const row = document.createElement("div");
    row.className = `roll-row${note.includes("S") ? " sharp" : ""}`;
    row.dataset.note = note;
    row.style.width = `${width}px`;
    grid.append(row);
  });

  function addBlock(note, className) {
    const row = grid.querySelector(`[data-note="${note.note}"]`);
    if (!row) {
      return;
    }
    const block = document.createElement("div");
    block.className = className;
    block.style.left = `${note.start / AUDIO_MS_PER_PX}px`;
    block.style.width = `${Math.max(4, note.duration / AUDIO_MS_PER_PX)}px`;
    block.dataset.note = note.note;
    block.dataset.start = note.start;
    block.dataset.duration = note.duration;
    block.title = `${note.note} ${note.duration}ms`;
    row.append(block);
  }

  notes.forEach((note) => addBlock(note, "roll-note"));
  if (preview) {
    addBlock(preview, "roll-preview");
  }

  grid.addEventListener("contextmenu", (event) => {
    event.preventDefault();
  });

  grid.addEventListener("pointerdown", (event) => {
    if (event.button === 2) {
      const target = event.target.closest(".roll-note");
      if (target) {
        removeRollNote(target);
      }
      return;
    }
    const row = event.target.closest(".roll-row");
    if (!row) {
      return;
    }
    const rect = grid.getBoundingClientRect();
    const x = event.clientX - rect.left;
    state.rollDrag = {
      note: row.dataset.note,
      start: snapMs(x * AUDIO_MS_PER_PX),
      current: snapMs(x * AUDIO_MS_PER_PX)
    };
    grid.setPointerCapture(event.pointerId);
    renderAudioRoll({ note: state.rollDrag.note, start: state.rollDrag.start, duration: AUDIO_SNAP_MS });
  });

  els.audioRoll.append(labels, grid);
}

// Dragging previews the note without committing until pointerup.
function updateRollDrag(event) {
  if (!state.rollDrag) {
    return;
  }
  const grid = els.audioRoll.querySelector(".roll-grid");
  if (!grid) {
    return;
  }
  const rect = grid.getBoundingClientRect();
  const x = Math.max(0, event.clientX - rect.left);
  state.rollDrag.current = snapMs(x * AUDIO_MS_PER_PX);
  const start = Math.min(state.rollDrag.start, state.rollDrag.current);
  const end = Math.max(state.rollDrag.start, state.rollDrag.current) + AUDIO_SNAP_MS;
  renderAudioRoll({ note: state.rollDrag.note, start, duration: end - start });
}

function finishRollDrag() {
  if (!state.rollDrag) {
    return;
  }
  const start = Math.min(state.rollDrag.start, state.rollDrag.current);
  const end = Math.max(state.rollDrag.start, state.rollDrag.current) + AUDIO_SNAP_MS;
  const notes = audioSequenceNotes();
  notes.push({ note: state.rollDrag.note, start, duration: end - start });
  state.rollDrag = null;
  setAudioEventsFromSequence(notes);
}

// Normalize audio state before export/save.
function audioDraft() {
  return {
    name: (els.audioName.value.trim() || "tone").replace(/[^A-Za-z0-9_]/g, "_"),
    events: state.audioEvents.map((event) => ({
      note: String(event.note || "NOTE_REST"),
      duration: Number(event.duration || 1)
    }))
  };
}

function exportAudioJson() {
  els.audioOutput.value = JSON.stringify(audioDraft(), null, 2);
  updateAudioStats();
  setStatus("Exported audio JSON");
}

function exportAudioC() {
  const draft = audioDraft();
  const lines = [`const uint16_t PROGMEM ${draft.name}[] = {`];
  draft.events.forEach((event) => {
    lines.push(`${event.note}, ${event.duration},`);
  });
  lines.push("TONES_END };");
  els.audioOutput.value = lines.join("\n");
  updateAudioStats();
  setStatus("Exported audio C array");
}

async function saveAudioDraft() {
  const draft = audioDraft();
  const response = await fetch("/api/save-audio", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(draft)
  });
  const payload = await response.json();
  if (!response.ok || !payload.ok) {
    throw new Error(payload.error || "Could not save audio draft.");
  }
  els.audioOutput.value = payload.cSource;
  await loadAudio();
  els.projectAudio.value = draft.name;
  setStatus(`Saved ${payload.json}`);
}

function noteFrequency(note) {
  if (note === "NOTE_REST") {
    return 0;
  }
  const match = String(note).match(/^NOTE_([A-G])([S#]?)(\d)$/);
  if (!match) {
    return 0;
  }
  const semitones = { C: -9, D: -7, E: -5, F: -4, G: -2, A: 0, B: 2 };
  const octave = Number(match[3]);
  const sharp = match[2] ? 1 : 0;
  const distance = semitones[match[1]] + sharp + (octave - 4) * 12;
  return 440 * Math.pow(2, distance / 12);
}

// Browser preview approximates ArduboyTones with a quiet square wave.
async function playAudioDraft() {
  const context = new AudioContext();
  let time = context.currentTime;
  audioDraft().events.forEach((event) => {
    const duration = Number(event.duration || 1) / 1000;
    const frequency = noteFrequency(event.note);
    if (frequency > 0) {
      const oscillator = context.createOscillator();
      const gain = context.createGain();
      oscillator.type = "square";
      oscillator.frequency.value = frequency;
      gain.gain.setValueAtTime(0.04, time);
      gain.gain.exponentialRampToValueAtTime(0.0001, time + duration);
      oscillator.connect(gain);
      gain.connect(context.destination);
      oscillator.start(time);
      oscillator.stop(time + duration);
    }
    time += duration;
  });
  setStatus("Played browser preview");
}

// Import the first C initializer body found in the pasted source.
function importC() {
  const text = els.source.value;
  const nameMatch = text.match(/(?:const\s+)?(?:uint8_t|byte)\s+(?:PROGMEM\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*\[\]/)
    || text.match(/([A-Za-z_][A-Za-z0-9_]*)\s*\[\]\s*=\s*\{/);
  const bodyMatch = text.match(/\{([\s\S]*?)\}/);
  if (!bodyMatch) {
    throw new Error("Could not find a C initializer body.");
  }
  const values = bodyMatch[1]
    .split(/[^0-9A-Fa-fx]+/)
    .map(parseNumber)
    .filter((value) => value !== null);
  decodeArduboy(nameMatch ? nameMatch[1] : els.name.value.trim(), values);
}

function importAscii() {
  const lines = els.source.value
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter((line) => line.length > 0 && !line.startsWith("//"));
  if (lines.length === 0) {
    throw new Error("Paste at least one ASCII grid line.");
  }
  const width = Math.max(...lines.map((line) => line.length));
  const frame = blankFrame(width, lines.length);
  lines.forEach((line, y) => {
    for (let x = 0; x < width; x += 1) {
      frame.pixels[y][x] = line[x] === "#";
    }
  });
  state.name = els.name.value.trim() || "sprite";
  state.frames = [frame];
  state.frameIndex = 0;
  refresh("Imported ASCII grid");
}

// Encode row-major booleans back into Arduboy vertical page bytes.
function encodeFrame(frame) {
  const pages = Math.ceil(frame.height / 8);
  const bytes = [];
  for (let page = 0; page < pages; page += 1) {
    for (let x = 0; x < frame.width; x += 1) {
      let byte = 0xff;
      for (let bit = 0; bit < 8; bit += 1) {
        const y = page * 8 + bit;
        if (y < frame.height && frame.pixels[y][x]) {
          byte &= ~(1 << bit);
        }
      }
      bytes.push(byte);
    }
  }
  return bytes;
}

function hexByte(value) {
  return `0x${value.toString(16).padStart(2, "0")}`;
}

function cArrayFor(frame, name) {
  return cArrayForFrames([frame], name);
}

function cArrayForFrames(frames, name) {
  const first = frames[0];
  const sameDimensions = frames.every((frame) => frame.width === first.width && frame.height === first.height);
  if (!sameDimensions) {
    // Arduboy multi-frame arrays require equal dimensions; split otherwise.
    return frames.map((frame, index) => cArrayFor(frame, `${name}_${index}`)).join("\n\n");
  }
  const bytes = frames.flatMap(encodeFrame);
  const lines = [`const uint8_t PROGMEM ${name}[] = {`, `${first.width}, ${first.height},`];
  for (let i = 0; i < bytes.length; i += first.width) {
    lines.push(`${bytes.slice(i, i + first.width).map(hexByte).join(", ")},`);
  }
  lines.push("};");
  return lines.join("\n");
}

function exportC() {
  const base = (els.name.value.trim() || state.name || "sprite").replace(/[^A-Za-z0-9_]/g, "_");
  const text = cArrayForFrames(state.frames, base);
  els.output.value = text;
  setStatus("Exported C array");
}

function exportAscii() {
  const frame = currentFrame();
  els.output.value = frame.pixels
    .map((row) => row.map((pixel) => (pixel ? "#" : ".")).join(""))
    .join("\n");
  setStatus("Exported ASCII grid");
}

function asciiForAllFrames() {
  return state.frames.map((frame, index) => {
    const body = frame.pixels
      .map((row) => row.map((pixel) => (pixel ? "#" : ".")).join(""))
      .join("\n");
    return state.frames.length === 1 ? body : `// frame ${index}\n${body}`;
  }).join("\n\n");
}

async function saveProjectDraft() {
  const name = (els.name.value.trim() || state.name || "sprite").replace(/[^A-Za-z0-9_ -]/g, "_");
  const cSource = cArrayForFrames(state.frames, name.replace(/[^A-Za-z0-9_]/g, "_"));
  const response = await fetch("/api/save-sprite", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      name,
      cSource,
      asciiSource: asciiForAllFrames()
    })
  });
  const payload = await response.json();
  if (!response.ok || !payload.ok) {
    throw new Error(payload.error || "Could not save sprite draft.");
  }
  els.output.value = cSource;
  setStatus(`Saved ${payload.header}`);
}

// Resize preserves overlapping pixels and drops anything outside the new bounds.
function resize(width, height) {
  const old = currentFrame();
  const frame = blankFrame(width, height);
  for (let y = 0; y < Math.min(height, old.height); y += 1) {
    for (let x = 0; x < Math.min(width, old.width); x += 1) {
      frame.pixels[y][x] = old.pixels[y][x];
    }
  }
  state.frames[state.frameIndex] = frame;
  refresh("Resized current frame");
}

function copyCurrentFrame() {
  state.frameClipboard = cloneFrame(currentFrame());
  setStatus(`Copied frame ${state.frameIndex + 1}`);
}

function pasteFrame(blackOnly) {
  if (!state.frameClipboard) {
    throw new Error("Copy a frame first.");
  }

  const target = currentFrame();
  const source = state.frameClipboard;
  const width = Math.min(target.width, source.width);
  const height = Math.min(target.height, source.height);

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      if (blackOnly) {
        target.pixels[y][x] = target.pixels[y][x] || source.pixels[y][x];
      } else {
        target.pixels[y][x] = source.pixels[y][x];
      }
    }
  }

  refresh(blackOnly ? "Pasted black pixels" : "Pasted frame");
}

// Flip and shift operate on copies to avoid in-place coordinate collisions.
function flip(horizontal) {
  const frame = currentFrame();
  const next = blankFrame(frame.width, frame.height);
  for (let y = 0; y < frame.height; y += 1) {
    for (let x = 0; x < frame.width; x += 1) {
      const nx = horizontal ? frame.width - 1 - x : x;
      const ny = horizontal ? y : frame.height - 1 - y;
      next.pixels[ny][nx] = frame.pixels[y][x];
    }
  }
  state.frames[state.frameIndex] = next;
  refresh(horizontal ? "Flipped horizontally" : "Flipped vertically");
}

function shift(dx, dy) {
  const frame = currentFrame();
  const next = blankFrame(frame.width, frame.height);
  for (let y = 0; y < frame.height; y += 1) {
    for (let x = 0; x < frame.width; x += 1) {
      const nx = x + dx;
      const ny = y + dy;
      if (nx >= 0 && ny >= 0 && nx < frame.width && ny < frame.height) {
        next.pixels[ny][nx] = frame.pixels[y][x];
      }
    }
  }
  state.frames[state.frameIndex] = next;
  refresh("Shifted frame");
}

function crop() {
  const frame = currentFrame();
  let minX = frame.width;
  let minY = frame.height;
  let maxX = -1;
  let maxY = -1;
  for (let y = 0; y < frame.height; y += 1) {
    for (let x = 0; x < frame.width; x += 1) {
      if (frame.pixels[y][x]) {
        minX = Math.min(minX, x);
        minY = Math.min(minY, y);
        maxX = Math.max(maxX, x);
        maxY = Math.max(maxY, y);
      }
    }
  }
  if (maxX < 0) {
    setStatus("Nothing to crop");
    return;
  }
  const next = blankFrame(maxX - minX + 1, maxY - minY + 1);
  for (let y = minY; y <= maxY; y += 1) {
    for (let x = minX; x <= maxX; x += 1) {
      next.pixels[y - minY][x - minX] = frame.pixels[y][x];
    }
  }
  state.frames[state.frameIndex] = next;
  refresh("Cropped to black pixels");
}

// Slice reads left-to-right, top-to-bottom and replaces the frame list.
function sliceSprite() {
  const frame = currentFrame();
  const width = Number(els.sliceWidth.value);
  const height = Number(els.sliceHeight.value);
  if (width < 1 || height < 1 || width > frame.width || height > frame.height) {
    throw new Error("Slice size must fit inside the current image.");
  }
  const frames = [];
  for (let y0 = 0; y0 + height <= frame.height; y0 += height) {
    for (let x0 = 0; x0 + width <= frame.width; x0 += width) {
      const next = blankFrame(width, height);
      for (let y = 0; y < height; y += 1) {
        for (let x = 0; x < width; x += 1) {
          next.pixels[y][x] = frame.pixels[y0 + y][x0 + x];
        }
      }
      frames.push(next);
    }
  }
  state.frames = frames;
  state.frameIndex = 0;
  refresh(`Sliced into ${frames.length} frame(s)`);
}

function downloadPng() {
  const link = document.createElement("a");
  link.download = `${els.name.value.trim() || "sprite"}.png`;
  link.href = frameToPngDataUrl(currentFrame());
  link.click();
}

function importImageElement(image, name) {
  const scratch = document.createElement("canvas");
  scratch.width = image.width;
  scratch.height = image.height;
  const sctx = scratch.getContext("2d");
  sctx.drawImage(image, 0, 0);
  const data = sctx.getImageData(0, 0, image.width, image.height).data;
  const frame = blankFrame(image.width, image.height);
  for (let y = 0; y < image.height; y += 1) {
    for (let x = 0; x < image.width; x += 1) {
      const i = (y * image.width + x) * 4;
      const alpha = data[i + 3];
      const luma = data[i] * 0.299 + data[i + 1] * 0.587 + data[i + 2] * 0.114;
      frame.pixels[y][x] = alpha >= 128 && luma < 128;
    }
  }
  state.name = name.replace(/\.[^.]+$/, "").replace(/[^A-Za-z0-9_]/g, "_") || "sprite";
  state.frames = [frame];
  state.frameIndex = 0;
  refresh(`Imported ${name}`);
}

function importPng(file) {
  const image = new Image();
  image.onload = () => {
    importImageElement(image, file.name);
  };
  image.src = URL.createObjectURL(file);
}

async function loadProfileBannerSprite() {
  if (!state.activeProfile) {
    throw new Error("Select a profile first.");
  }
  const path = bannerPathFromProfile(parseProfileRaw());
  const image = new Image();
  image.onload = () => {
    importImageElement(image, `${state.activeProfile.slug}_banner.png`);
    if (image.width !== 128 || image.height !== 64) {
      setStatus(`Loaded banner ${image.width}x${image.height}; FX expects 128x64`);
    } else {
      setStatus(`Loaded banner ${path}`);
    }
  };
  image.onerror = () => setStatus(`Could not load banner ${path}`);
  image.src = `/${path}?v=${Date.now()}`;
}

async function saveCurrentSpriteAsBanner() {
  const frame = currentFrame();
  if (frame.width !== 128 || frame.height !== 64) {
    throw new Error("FX banners must be 128x64. Resize or load a 128x64 banner first.");
  }
  const path = await saveProfileBannerDataUrl(frameToPngDataUrl(frame));
  updateBannerPreview(parseProfileRaw());
  setStatus(`Saved banner ${path}`);
}

// Keep UI event handlers thin; guarded() reports failures in the status bar.
document.querySelectorAll("[data-tool]").forEach((button) => {
  button.addEventListener("click", () => {
    state.tool = button.dataset.tool;
    document.querySelectorAll("[data-tool]").forEach((item) => item.classList.remove("active"));
    button.classList.add("active");
  });
});
document.querySelectorAll("[data-mode]").forEach((button) => {
  button.addEventListener("click", () => setMode(button.dataset.mode));
});
document.querySelectorAll("[data-audio-view]").forEach((button) => {
  button.addEventListener("click", () => setAudioView(button.dataset.audioView));
});
document.querySelectorAll("[data-preferences-view]").forEach((button) => {
  button.addEventListener("click", guarded(() => setPreferencesView(button.dataset.preferencesView)));
});

canvas.addEventListener("contextmenu", (event) => event.preventDefault());
canvas.addEventListener("pointerdown", (event) => {
  const point = canvasPixel(event);
  if (!point) {
    return;
  }
  canvas.setPointerCapture(event.pointerId);
  setCursorPosition(point);
  state.drawing = true;
  state.strokeMode = event.button === 2 ? "erase" : state.tool;
  state.lastPoint = point;
  if (state.strokeMode === "line") {
    state.lineStart = point;
    state.linePreview = cloneFrame(currentFrame());
    paintPoint(state.linePreview, point, "draw", Number(els.brushSize.value));
    render();
  } else {
    applyStroke(point);
  }
});
canvas.addEventListener("pointermove", (event) => {
  const point = canvasPixel(event);
  setCursorPosition(point);
  if (!point) {
    return;
  }
  if (!state.drawing) {
    state.lastPoint = point;
    render();
    return;
  }
  if (state.lineStart) {
    state.lastPoint = point;
    state.linePreview = cloneFrame(currentFrame());
    paintLine(state.linePreview, state.lineStart, point, state.strokeMode, Number(els.brushSize.value));
    render();
  } else {
    applyStroke(point);
  }
});
canvas.addEventListener("pointerleave", () => {
  if (!state.drawing) {
    state.lastPoint = null;
    setCursorPosition(null);
    render();
  }
});
window.addEventListener("pointerup", (event) => {
  finishRollDrag();
  if (state.lineStart && state.lastPoint) {
    paintLine(currentFrame(), state.lineStart, state.lastPoint, state.strokeMode, Number(els.brushSize.value));
  }
  state.lineStart = null;
  state.linePreview = null;
  state.lastPoint = null;
  state.drawing = false;
  if (canvas.hasPointerCapture(event.pointerId)) {
    canvas.releasePointerCapture(event.pointerId);
  }
  render();
});
window.addEventListener("pointermove", updateRollDrag);

function guarded(fn) {
  return () => {
    try {
      Promise.resolve(fn()).catch((error) => setStatus(error.message));
    } catch (error) {
      setStatus(error.message);
    }
  };
}

document.querySelector("#newSprite").addEventListener("click", guarded(() => {
  state.name = els.name.value.trim() || "sprite";
  state.frames = [blankFrame(Number(els.width.value), Number(els.height.value))];
  state.frameIndex = 0;
  refresh("Created blank sprite");
}));
document.querySelector("#resizeSprite").addEventListener("click", guarded(() => resize(Number(els.width.value), Number(els.height.value))));
document.querySelector("#importC").addEventListener("click", guarded(importC));
document.querySelector("#importAscii").addEventListener("click", guarded(importAscii));
els.projectAssets.addEventListener("change", guarded(loadSelectedProjectAsset));
document.querySelector("#exportC").addEventListener("click", exportC);
document.querySelector("#exportAscii").addEventListener("click", exportAscii);
document.querySelector("#copyOutput").addEventListener("click", guarded(async () => {
  await navigator.clipboard.writeText(els.output.value);
  setStatus("Copied output");
}));
document.querySelector("#saveProject").addEventListener("click", guarded(saveProjectDraft));
document.querySelector("#downloadPng").addEventListener("click", downloadPng);
document.querySelector("#loadBannerSprite").addEventListener("click", guarded(loadProfileBannerSprite));
document.querySelector("#saveBannerSprite").addEventListener("click", guarded(saveCurrentSpriteAsBanner));
document.querySelector("#flipH").addEventListener("click", () => flip(true));
document.querySelector("#flipV").addEventListener("click", () => flip(false));
document.querySelector("#crop").addEventListener("click", crop);
document.querySelector("#clear").addEventListener("click", () => {
  const frame = currentFrame();
  state.frames[state.frameIndex] = blankFrame(frame.width, frame.height);
  refresh("Cleared frame");
});
document.querySelector("#sliceSprite").addEventListener("click", guarded(sliceSprite));
document.querySelector("#copyFrame").addEventListener("click", copyCurrentFrame);
document.querySelector("#pasteFrame").addEventListener("click", guarded(() => pasteFrame(false)));
document.querySelector("#pasteFrameBlack").addEventListener("click", guarded(() => pasteFrame(true)));
document.querySelector("#createProfile").addEventListener("click", guarded(createProfile));
document.querySelector("#saveProfile").addEventListener("click", guarded(saveCurrentProfile));
document.querySelector("#saveBanner").addEventListener("click", guarded(saveProfileBanner));
els.profileSelect.addEventListener("change", () => selectProfile(els.profileSelect.value));
els.projectAudio.addEventListener("change", guarded(loadSelectedAudio));
document.querySelector("#addAudioEvent").addEventListener("click", () => {
  state.audioEvents.push({ note: "NOTE_REST", duration: 50 });
  renderAudioEvents();
});
document.querySelector("#exportAudioJson").addEventListener("click", exportAudioJson);
document.querySelector("#exportAudioC").addEventListener("click", exportAudioC);
document.querySelector("#saveAudioDraft").addEventListener("click", guarded(saveAudioDraft));
document.querySelector("#playAudio").addEventListener("click", guarded(playAudioDraft));
document.querySelector("#prevFrame").addEventListener("click", () => {
  state.frameIndex = (state.frameIndex + state.frames.length - 1) % state.frames.length;
  refresh();
});
document.querySelector("#nextFrame").addEventListener("click", () => {
  state.frameIndex = (state.frameIndex + 1) % state.frames.length;
  refresh();
});
document.querySelectorAll("[data-shift]").forEach((button) => {
  button.addEventListener("click", () => {
    const [dx, dy] = button.dataset.shift.split(",").map(Number);
    shift(dx, dy);
  });
});
els.zoom.addEventListener("input", render);
els.brushSize.addEventListener("input", () => {
  els.brushSizeValue.textContent = `${els.brushSize.value}px`;
  render();
});
els.grid.addEventListener("change", render);
els.pngInput.addEventListener("change", () => {
  if (els.pngInput.files.length > 0) {
    importPng(els.pngInput.files[0]);
  }
});

refresh("Ready");
setAudioView("roll");
loadProjectAssets();
loadProfiles();
loadAudio();
