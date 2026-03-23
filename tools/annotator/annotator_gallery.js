let galleryTick = 0;
let galleryTimer = null;
let activePreviews = [];
const imageCache = new Map();
const sourceCanvas = document.createElement("canvas");
const sourceContext = sourceCanvas.getContext("2d", { willReadFrequently: true });
const PREVIEW_SCALE = 3;

function groupKey(annotation) {
  return [
    annotation.type ?? "sprite",
    annotation.name?.trim() ?? "",
    annotation.direction?.trim() ?? "",
    annotation.variant?.trim() ?? "",
  ].join("|");
}

function sortFrames(frames) {
  frames.sort((left, right) => {
    const frameDiff = (left.annotation.frame ?? 0) - (right.annotation.frame ?? 0);
    if (frameDiff !== 0) {
      return frameDiff;
    }
    if (left.sheetFile !== right.sheetFile) {
      return left.sheetFile.localeCompare(right.sheetFile);
    }
    if (left.annotation.y !== right.annotation.y) {
      return left.annotation.y - right.annotation.y;
    }
    return left.annotation.x - right.annotation.x;
  });
}

function buildGroups(state) {
  const groups = new Map();
  for (const sheet of state.projectSheets) {
    const annotations = Array.isArray(sheet.annotations) ? sheet.annotations : [];
    for (const annotation of annotations) {
      const name = annotation.name?.trim() ?? "";
      if (!name) {
        continue;
      }

      const key = groupKey(annotation);
      let group = groups.get(key);
      if (!group) {
        group = {
          key,
          type: annotation.type ?? "sprite",
          name,
          direction: annotation.direction?.trim() ?? "",
          variant: annotation.variant?.trim() ?? "",
          inCurrentSheet: false,
          sheetFiles: new Set(),
          frames: [],
        };
        groups.set(key, group);
      }

      group.inCurrentSheet ||= sheet.file === state.currentSheet?.file;
      group.sheetFiles.add(sheet.file);
      group.frames.push({
        annotation,
        annotationId: annotation.id,
        imageUrl: sheet.image_url,
        sheetFile: sheet.file,
      });
    }
  }

  const built = Array.from(groups.values());
  for (const group of built) {
    sortFrames(group.frames);
  }

  built.sort((left, right) => {
    if (left.inCurrentSheet !== right.inCurrentSheet) {
      return left.inCurrentSheet ? -1 : 1;
    }
    return left.key.localeCompare(right.key);
  });

  return built;
}

function loadImage(imageUrl) {
  const cached = imageCache.get(imageUrl);
  if (cached) {
    return cached;
  }

  const promise = new Promise((resolve, reject) => {
    const image = new Image();
    image.onload = () => resolve(image);
    image.onerror = reject;
    image.src = `${imageUrl}?v=gallery`;
  });
  imageCache.set(imageUrl, promise);
  return promise;
}

function clearCanvas(canvas) {
  const context = canvas.getContext("2d");
  context.clearRect(0, 0, canvas.width, canvas.height);
  return context;
}

function drawPlaceholder(canvas, label) {
  const context = clearCanvas(canvas);
  context.fillStyle = "#9da3ad";
  context.font = "11px sans-serif";
  context.fillText(label, 8, 16);
}

function parseHexColor(value) {
  if (typeof value !== "string") {
    return null;
  }

  const normalized = value.trim().toLowerCase();
  const match = normalized.match(/^#?([0-9a-f]{6})$/);
  if (!match) {
    return null;
  }

  const hex = match[1];
  return {
    r: Number.parseInt(hex.slice(0, 2), 16),
    g: Number.parseInt(hex.slice(2, 4), 16),
    b: Number.parseInt(hex.slice(4, 6), 16),
  };
}

function drawFrameWithChroma({ canvas, frame, image }) {
  const width = Math.max(1, frame.annotation.width);
  const height = Math.max(1, frame.annotation.height);
  sourceCanvas.width = width;
  sourceCanvas.height = height;
  sourceContext.clearRect(0, 0, width, height);
  sourceContext.drawImage(image, frame.annotation.x, frame.annotation.y, width, height, 0, 0, width, height);

  const chroma = parseHexColor(frame.annotation.chroma_key);
  if (chroma) {
    const imageData = sourceContext.getImageData(0, 0, width, height);
    const data = imageData.data;
    for (let index = 0; index < data.length; index += 4) {
      if (data[index] === chroma.r && data[index + 1] === chroma.g && data[index + 2] === chroma.b) {
        data[index + 3] = 0;
      }
    }
    sourceContext.putImageData(imageData, 0, 0);
  }

  const context = clearCanvas(canvas);
  context.imageSmoothingEnabled = false;
  context.drawImage(sourceCanvas, 0, 0, width, height, 0, 0, canvas.width, canvas.height);
}

function drawPreview(preview) {
  const { canvas, group } = preview;
  if (group.frames.length === 0) {
    drawPlaceholder(canvas, "No frames");
    return;
  }

  const frameIndex = group.frames.length === 1 ? 0 : galleryTick % group.frames.length;
  const frame = group.frames[frameIndex];
  const width = Math.max(1, frame.annotation.width);
  const height = Math.max(1, frame.annotation.height);
  canvas.width = width * PREVIEW_SCALE;
  canvas.height = height * PREVIEW_SCALE;

  loadImage(frame.imageUrl)
    .then((image) => {
      drawFrameWithChroma({ canvas, frame, image });
    })
    .catch(() => {
      drawPlaceholder(canvas, "Load failed");
    });
}

function renderPreviews() {
  for (const preview of activePreviews) {
    drawPreview(preview);
  }
}

export function startGalleryLoop() {
  if (galleryTimer !== null) {
    return;
  }

  galleryTimer = window.setInterval(() => {
    galleryTick += 1;
    renderPreviews();
  }, 250);
}

export function renderGallery({ countLabel, container, onOpenFrame, state }) {
  const groups = buildGroups(state);
  activePreviews = [];
  countLabel.textContent = `${groups.length}`;
  container.replaceChildren();

  for (const group of groups) {
    const frameTarget =
      group.frames.find((frame) => frame.sheetFile === state.currentSheet?.file) ?? group.frames[0];
    const card = document.createElement("button");
    card.type = "button";
    card.className = `gallery-card${group.inCurrentSheet ? " current-sheet" : ""}`;
    card.addEventListener("click", () => onOpenFrame(frameTarget));

    const canvas = document.createElement("canvas");
    canvas.className = "gallery-preview";
    card.append(canvas);

    const name = document.createElement("div");
    name.className = "gallery-name";
    name.textContent = group.name;
    card.append(name);

    const meta = document.createElement("div");
    const parts = [`${group.frames.length} frame${group.frames.length === 1 ? "" : "s"}`];
    if (group.direction) {
      parts.push(group.direction);
    }
    if (group.variant) {
      parts.push(group.variant);
    }
    if (group.inCurrentSheet) {
      parts.push("current sheet");
    }
    meta.className = "gallery-meta";
    meta.textContent = parts.join(" • ");
    card.append(meta);

    activePreviews.push({ canvas, group });
    drawPreview({ canvas, group });
    container.append(card);
  }
}
