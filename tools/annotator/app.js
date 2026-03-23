import {
  clamp,
  elements,
  inspectorFields,
  state,
  ZOOM_FACTOR,
  ZOOM_MAX,
  ZOOM_MIN,
} from "/annotator_common.js";
import {
  annotationById,
  clampAnnotationToImage,
  cloneAnnotations,
  defaultAnnotation,
  refreshSelectionStyles,
  renderAnnotationList,
  renderAnnotations,
  renderInspector,
  selectedAnnotation,
  serializeAnnotation,
  syncCurrentSheetIntoProject,
  updateAnnotationBox,
} from "/annotator_annotations.js";
import { renderGallery, startGalleryLoop } from "/annotator_gallery.js";

const NUMERIC_FIELDS = new Set(["frame", "x", "y", "width", "height"]);
let devRevision = null;
let hotReloadPending = false;

function setStatus(text) {
  elements.statusLabel.textContent = text;
}

function duplicateAnnotationId(annotation) {
  return crypto.randomUUID ? crypto.randomUUID() : `${annotation.id}-copy-${Date.now()}`;
}

function applyHotReloadState() {
  if (!hotReloadPending) {
    return;
  }

  if (!state.dirty) {
    window.location.reload();
    return;
  }

  setStatus("Annotator code changed. Save or reload to apply the latest UI.");
}

function markDirty(isDirty) {
  state.dirty = isDirty;
  if (!state.currentSheet) {
    setStatus(isDirty ? "Unsaved changes" : "Ready");
    applyHotReloadState();
    return;
  }
  setStatus(`${state.currentSheet.file} • ${isDirty ? "Unsaved changes" : "Saved"}`);
  applyHotReloadState();
}

async function fetchJson(url, options = {}) {
  const response = await fetch(url, options);
  if (!response.ok) {
    throw new Error(`HTTP ${response.status}`);
  }
  return response.json();
}

async function pollDevRevision() {
  try {
    const payload = await fetchJson("/api/dev_revision");
    const nextRevision = payload.revision ?? "";
    if (devRevision === null) {
      devRevision = nextRevision;
      return;
    }

    if (nextRevision !== devRevision) {
      devRevision = nextRevision;
      hotReloadPending = true;
      applyHotReloadState();
    }
  } catch (error) {
    console.debug("hot reload poll failed", error);
  }
}

async function loadProjectData() {
  setStatus("Loading sheets…");
  const currentFile = state.currentSheet?.file ?? null;
  const currentSelection = state.selectedId;
  const payload = await fetchJson("/api/project_annotations");
  state.projectSheets = (payload.sheets ?? []).map((sheet) => ({
    ...sheet,
    annotations: cloneAnnotations(sheet.annotations ?? []),
  }));
  state.sheets = state.projectSheets.map(({ annotations, ...sheet }) => ({ ...sheet }));
  filterSheets();
  if (currentFile && state.projectSheets.some((sheet) => sheet.file === currentFile)) {
    await openSheet(currentFile, currentSelection);
  } else if (state.filteredSheets.length > 0) {
    await openSheet(state.filteredSheets[0].file);
  }
  renderGalleryPanel();
}

function filterSheets() {
  const query = elements.sheetSearchInput.value.trim().toLowerCase();
  state.filteredSheets = state.sheets.filter((sheet) => {
    if (!query) {
      return true;
    }
    return sheet.file.toLowerCase().includes(query) || sheet.name.toLowerCase().includes(query);
  });
  renderSheets();
}

function renderSheets() {
  elements.sheetList.replaceChildren();
  for (const sheet of state.filteredSheets) {
    const card = document.createElement("button");
    card.type = "button";
    card.className = `sheet-card${state.currentSheet?.file === sheet.file ? " selected" : ""}`;
    card.innerHTML = `<div class="sheet-name">${sheet.name}</div><div class="sheet-meta">${sheet.file}</div>`;
    card.addEventListener("click", () => {
      openSheet(sheet.file).catch((error) => {
        console.error(error);
        setStatus(`Failed to open ${sheet.file}`);
      });
    });
    elements.sheetList.append(card);
  }
}

async function loadSheetImage(src, file) {
  return new Promise((resolve, reject) => {
    const image = elements.sheetImage;
    image.onload = () => {
      state.currentImage = image;
      if (!state.sampleCanvas) {
        state.sampleCanvas = document.createElement("canvas");
        state.sampleContext = state.sampleCanvas.getContext("2d", { willReadFrequently: true });
      }
      state.sampleCanvas.width = image.naturalWidth;
      state.sampleCanvas.height = image.naturalHeight;
      state.sampleContext.clearRect(0, 0, image.naturalWidth, image.naturalHeight);
      state.sampleContext.drawImage(image, 0, 0);
      updateCanvasStage();
      setStatus(`${file} • ${state.dirty ? "Unsaved changes" : "Saved"}`);
      resolve();
    };
    image.onerror = reject;
    image.src = `${src}?v=${Date.now()}`;
    image.alt = file;
  });
}

async function openSheet(file, selectedId = null) {
  if (state.currentSheet?.file !== file && state.dirty) {
    const confirmed = window.confirm("Discard unsaved changes on the current sheet?");
    if (!confirmed) {
      return;
    }
  }

  const record = state.projectSheets.find((sheet) => sheet.file === file);
  if (!record) {
    return;
  }

  state.currentSheet = record;
  state.annotations = cloneAnnotations(record.annotations);
  state.selectedId =
    selectedId && state.annotations.some((entry) => entry.id === selectedId)
      ? selectedId
      : (state.annotations[0]?.id ?? null);
  markDirty(false);
  renderSheets();
  await loadSheetImage(record.image_url, record.file);
  renderAnnotationList(selectAnnotation);
  renderInspector();
  renderAnnotations(onAnnotationPointerDown);
  renderGalleryPanel();
}

function updateCanvasStage() {
  const image = state.currentImage;
  if (!image) {
    return;
  }

  const width = Math.round(image.naturalWidth * state.zoom);
  const height = Math.round(image.naturalHeight * state.zoom);
  elements.sheetImage.style.width = `${width}px`;
  elements.sheetImage.style.height = `${height}px`;
  elements.annotationLayer.style.width = `${width}px`;
  elements.annotationLayer.style.height = `${height}px`;
  elements.canvasStage.style.width = `${width}px`;
  elements.canvasStage.style.height = `${height}px`;
  elements.zoomLabel.textContent = `${Math.round(state.zoom * 100)}%`;
}

function canvasPointToImagePixel(clientX, clientY) {
  if (!state.currentImage) {
    return null;
  }

  const rect = elements.annotationLayer.getBoundingClientRect();
  const x = clamp(Math.floor((clientX - rect.left) / state.zoom), 0, state.currentImage.naturalWidth - 1);
  const y = clamp(Math.floor((clientY - rect.top) / state.zoom), 0, state.currentImage.naturalHeight - 1);
  return { x, y };
}

function sampleHexColorAt(clientX, clientY) {
  const point = canvasPointToImagePixel(clientX, clientY);
  if (!point || !state.sampleContext) {
    return null;
  }

  const pixel = state.sampleContext.getImageData(point.x, point.y, 1, 1).data;
  const toHex = (value) => value.toString(16).padStart(2, "0");
  return `#${toHex(pixel[0])}${toHex(pixel[1])}${toHex(pixel[2])}`;
}

function setColorPickArmed(isArmed) {
  state.colorPickArmed = isArmed;
  elements.pickChromaButton.textContent = isArmed ? "Click Sheet" : "Pick";
  elements.annotationLayer.classList.toggle("color-pick-armed", isArmed);
  if (isArmed) {
    setStatus("Click the sheet to sample a chroma key.");
  } else if (state.currentSheet) {
    setStatus(`${state.currentSheet.file} • ${state.dirty ? "Unsaved changes" : "Saved"}`);
  }
}

function sampleChromaAt(clientX, clientY) {
  const color = sampleHexColorAt(clientX, clientY);
  if (!color) {
    return false;
  }

  updateSelected({ chroma_key: color });
  setColorPickArmed(false);
  return true;
}

function zoomTo(nextZoom, clientX = null, clientY = null) {
  const clamped = clamp(nextZoom, ZOOM_MIN, ZOOM_MAX);
  if (Math.abs(clamped - state.zoom) < 0.001 || !state.currentImage) {
    return;
  }

  const oldZoom = state.zoom;
  const scrollerRect = elements.canvasScroller.getBoundingClientRect();
  const stageRect = elements.canvasStage.getBoundingClientRect();
  const offsetX = clientX === null ? elements.canvasScroller.clientWidth / 2 : clientX - scrollerRect.left;
  const offsetY = clientY === null ? elements.canvasScroller.clientHeight / 2 : clientY - scrollerRect.top;
  const stageX = clientX === null ? elements.canvasScroller.scrollLeft + offsetX : clientX - stageRect.left;
  const stageY = clientY === null ? elements.canvasScroller.scrollTop + offsetY : clientY - stageRect.top;
  const worldX = stageX / oldZoom;
  const worldY = stageY / oldZoom;

  state.zoom = clamped;
  updateCanvasStage();
  renderAnnotations();

  elements.canvasScroller.scrollLeft = worldX * clamped - offsetX;
  elements.canvasScroller.scrollTop = worldY * clamped - offsetY;
}

function renderGalleryPanel() {
  renderGallery({
    countLabel: elements.galleryCountLabel,
    container: elements.galleryList,
    onOpenFrame: focusGalleryFrame,
    state,
  });
}

function rerenderAfterAnnotationChange(includeGallery = true) {
  syncCurrentSheetIntoProject();
  renderAnnotationList(selectAnnotation);
  renderInspector();
  renderAnnotations(onAnnotationPointerDown);
  if (includeGallery) {
    renderGalleryPanel();
  }
}

function updateSelected(patch, includeGallery = true) {
  const annotation = selectedAnnotation();
  if (!annotation) {
    return;
  }

  Object.assign(annotation, patch);
  clampAnnotationToImage(annotation);
  markDirty(true);
  rerenderAfterAnnotationChange(includeGallery);
}

function addAnnotation() {
  const annotation = defaultAnnotation();
  state.annotations.push(annotation);
  state.selectedId = annotation.id;
  markDirty(true);
  rerenderAfterAnnotationChange();
}

function duplicateSelected() {
  const annotation = selectedAnnotation();
  if (!annotation) {
    return;
  }

  const copy = {
    ...annotation,
    id: duplicateAnnotationId(annotation),
    frame: annotation.frame + 1,
    x: annotation.x + 4,
    y: annotation.y + 4,
  };
  state.annotations.push(copy);
  state.selectedId = copy.id;
  markDirty(true);
  rerenderAfterAnnotationChange();
}

function deleteSelected() {
  if (!state.selectedId) {
    return;
  }

  state.annotations = state.annotations.filter((entry) => entry.id !== state.selectedId);
  state.selectedId = state.annotations[0]?.id ?? null;
  markDirty(true);
  rerenderAfterAnnotationChange();
}

async function saveAnnotations() {
  if (!state.currentSheet) {
    return;
  }

  const payload = {
    image: state.currentSheet.file,
    annotations: state.annotations.map(serializeAnnotation),
  };
  setStatus(`Saving ${state.currentSheet.file}…`);
  await fetchJson(`/api/annotations?sheet=${encodeURIComponent(state.currentSheet.file)}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  syncCurrentSheetIntoProject();
  markDirty(false);
  renderGalleryPanel();
}

function selectAnnotation(annotationId) {
  state.selectedId = annotationId;
  setColorPickArmed(false);
  renderInspector();
  renderAnnotationList(selectAnnotation);
  refreshSelectionStyles();
}

function onAnnotationPointerDown(event) {
  event.preventDefault();
  event.stopPropagation();
  if (state.colorPickArmed) {
    sampleChromaAt(event.clientX, event.clientY);
    return;
  }

  const box = event.currentTarget;
  const annotation = annotationById(box.dataset.id);
  if (!annotation) {
    return;
  }

  state.selectedId = annotation.id;
  renderInspector();
  renderAnnotationList(selectAnnotation);
  refreshSelectionStyles();

  state.drag = {
    id: annotation.id,
    mode: event.target instanceof HTMLElement && event.target.dataset.resize === "true" ? "resize" : "move",
    pointerId: event.pointerId,
    startClientX: event.clientX,
    startClientY: event.clientY,
    startX: annotation.x,
    startY: annotation.y,
    startWidth: annotation.width,
    startHeight: annotation.height,
  };
  box.setPointerCapture(event.pointerId);
  box.addEventListener("pointermove", onAnnotationPointerMove);
  box.addEventListener("pointerup", onAnnotationPointerUp);
  box.addEventListener("pointercancel", onAnnotationPointerUp);
}

function onAnnotationPointerMove(event) {
  if (!state.drag || state.drag.pointerId !== event.pointerId) {
    return;
  }

  const annotation = annotationById(state.drag.id);
  const image = state.currentImage;
  if (!annotation || !image) {
    return;
  }

  const deltaX = (event.clientX - state.drag.startClientX) / state.zoom;
  const deltaY = (event.clientY - state.drag.startClientY) / state.zoom;
  if (state.drag.mode === "move") {
    annotation.x = clamp(Math.round(state.drag.startX + deltaX), 0, image.naturalWidth - annotation.width);
    annotation.y = clamp(Math.round(state.drag.startY + deltaY), 0, image.naturalHeight - annotation.height);
  } else {
    annotation.width = clamp(Math.round(state.drag.startWidth + deltaX), 1, image.naturalWidth - annotation.x);
    annotation.height = clamp(Math.round(state.drag.startHeight + deltaY), 1, image.naturalHeight - annotation.y);
  }

  markDirty(true);
  syncCurrentSheetIntoProject();
  renderInspector();
  renderAnnotationList(selectAnnotation);
  updateAnnotationBox(event.currentTarget, annotation);
}

function onAnnotationPointerUp(event) {
  const box = event.currentTarget;
  box.releasePointerCapture(event.pointerId);
  box.removeEventListener("pointermove", onAnnotationPointerMove);
  box.removeEventListener("pointerup", onAnnotationPointerUp);
  box.removeEventListener("pointercancel", onAnnotationPointerUp);
  state.drag = null;
  renderGalleryPanel();
}

function onAnnotationLayerPointerDown(event) {
  if (!state.colorPickArmed || event.target !== elements.annotationLayer) {
    return;
  }

  event.preventDefault();
  event.stopPropagation();
  sampleChromaAt(event.clientX, event.clientY);
}

async function focusGalleryFrame(frame) {
  if (!frame) {
    return;
  }

  if (state.currentSheet?.file !== frame.sheetFile) {
    await openSheet(frame.sheetFile, frame.annotationId);
    return;
  }

  state.selectedId = frame.annotationId;
  renderInspector();
  renderAnnotationList(selectAnnotation);
  refreshSelectionStyles();
}

function bindEvents() {
  elements.sheetSearchInput.addEventListener("input", filterSheets);
  elements.refreshSheetsButton.addEventListener("click", () => {
    if (state.dirty) {
      const confirmed = window.confirm("Discard unsaved changes and reload project annotations?");
      if (!confirmed) {
        return;
      }
    }
    loadProjectData().catch((error) => {
      console.error(error);
      setStatus("Failed to load sheets");
    });
  });
  elements.addAnnotationButton.addEventListener("click", addAnnotation);
  elements.duplicateAnnotationButton.addEventListener("click", duplicateSelected);
  elements.deleteAnnotationButton.addEventListener("click", deleteSelected);
  elements.pickChromaButton.addEventListener("click", () => {
    if (!selectedAnnotation()) {
      return;
    }
    setColorPickArmed(!state.colorPickArmed);
  });
  elements.saveButton.addEventListener("click", () => {
    saveAnnotations().catch((error) => {
      console.error(error);
      setStatus("Save failed");
    });
  });
  elements.zoomOutButton.addEventListener("click", () => zoomTo(state.zoom / ZOOM_FACTOR));
  elements.zoomInButton.addEventListener("click", () => zoomTo(state.zoom * ZOOM_FACTOR));
  elements.canvasScroller.addEventListener(
    "wheel",
    (event) => {
      if (!state.currentImage) {
        return;
      }
      event.preventDefault();
      zoomTo(state.zoom * (event.deltaY < 0 ? ZOOM_FACTOR : 1 / ZOOM_FACTOR), event.clientX, event.clientY);
    },
    { passive: false },
  );

  for (const field of inspectorFields) {
    field.addEventListener("input", () => {
      const key = field.dataset.field;
      const value = NUMERIC_FIELDS.has(key) ? Number(field.value) || 0 : field.value;
      updateSelected({ [key]: value });
    });
  }

  window.addEventListener("keydown", (event) => {
    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "s") {
      event.preventDefault();
      saveAnnotations().catch((error) => {
        console.error(error);
        setStatus("Save failed");
      });
      return;
    }

    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "d") {
      const tag = document.activeElement?.tagName ?? "";
      if (tag !== "INPUT" && tag !== "TEXTAREA" && tag !== "SELECT") {
        event.preventDefault();
        duplicateSelected();
      }
      return;
    }

    if (event.key === "Delete" || event.key === "Backspace") {
      const tag = document.activeElement?.tagName ?? "";
      if (tag !== "INPUT" && tag !== "TEXTAREA" && tag !== "SELECT") {
        event.preventDefault();
        deleteSelected();
      }
    }
  });

  window.addEventListener("beforeunload", (event) => {
    if (!state.dirty) {
      return;
    }
    event.preventDefault();
    event.returnValue = "";
  });

  elements.annotationLayer.addEventListener("pointerdown", onAnnotationLayerPointerDown);
}

async function main() {
  bindEvents();
  startGalleryLoop();
  elements.duplicateAnnotationButton.disabled = true;
  elements.deleteAnnotationButton.disabled = true;
  elements.pickChromaButton.disabled = true;
  await pollDevRevision();
  window.setInterval(() => {
    pollDevRevision().catch((error) => {
      console.debug("hot reload interval failed", error);
    });
  }, 1000);
  await loadProjectData();
}

main().catch((error) => {
  console.error(error);
  setStatus("Startup failed");
});
