import { clamp, elements, inspectorFields, makeId, snap, state } from "/annotator_common.js";

function annotationById(id) {
  return state.annotations.find((entry) => entry.id === id) ?? null;
}

function selectedAnnotation() {
  return annotationById(state.selectedId);
}

function setStatus(text) {
  elements.statusLabel.textContent = text;
}

function markDirty(isDirty) {
  state.dirty = isDirty;
  const suffix = isDirty ? "Unsaved changes" : "Saved";
  if (state.currentSheet) {
    setStatus(`${state.currentSheet.file} • ${suffix}`);
  } else {
    setStatus(isDirty ? "Unsaved changes" : "Ready");
  }
}

function defaultAnnotation() {
  const image = state.currentImage;
  const width = image ? Math.min(32, image.naturalWidth || 32) : 32;
  const height = image ? Math.min(32, image.naturalHeight || 32) : 32;
  const centerX = image ? Math.floor((image.naturalWidth - width) / 2) : 0;
  const centerY = image ? Math.floor((image.naturalHeight - height) / 2) : 0;
  return {
    id: makeId(),
    name: "new_sprite",
    type: "sprite",
    x: snap(centerX),
    y: snap(centerY),
    width,
    height,
    frame_width: width,
    frame_height: height,
    frame_count: 1,
    direction: "",
    variant: "",
    tags: "",
    notes: "",
  };
}

function normalizeAnnotation(raw) {
  return {
    id: typeof raw.id === "string" && raw.id ? raw.id : makeId(),
    name: typeof raw.name === "string" ? raw.name : "unnamed",
    type: typeof raw.type === "string" && raw.type ? raw.type : "sprite",
    x: Number.isFinite(raw.x) ? raw.x : 0,
    y: Number.isFinite(raw.y) ? raw.y : 0,
    width: Number.isFinite(raw.width) && raw.width > 0 ? raw.width : 16,
    height: Number.isFinite(raw.height) && raw.height > 0 ? raw.height : 16,
    frame_width: Number.isFinite(raw.frame_width) && raw.frame_width > 0 ? raw.frame_width : 16,
    frame_height: Number.isFinite(raw.frame_height) && raw.frame_height > 0 ? raw.frame_height : 16,
    frame_count: Number.isFinite(raw.frame_count) && raw.frame_count > 0 ? raw.frame_count : 1,
    direction: typeof raw.direction === "string" ? raw.direction : "",
    variant: typeof raw.variant === "string" ? raw.variant : "",
    tags: typeof raw.tags === "string" ? raw.tags : "",
    notes: typeof raw.notes === "string" ? raw.notes : "",
  };
}

async function fetchJson(url, options = {}) {
  const response = await fetch(url, options);
  if (!response.ok) {
    throw new Error(`HTTP ${response.status}`);
  }

  return response.json();
}

async function loadSheets() {
  setStatus("Loading sheets…");
  const payload = await fetchJson("/api/sheets");
  state.sheets = payload.sheets ?? [];
  filterSheets();
  if (!state.currentSheet && state.filteredSheets.length > 0) {
    await openSheet(state.filteredSheets[0].file);
  } else {
    renderSheets();
  }
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
    card.innerHTML = `
      <div class="sheet-name">${sheet.name}</div>
      <div class="sheet-meta">${sheet.file}</div>
    `;
    card.addEventListener("click", () => {
      openSheet(sheet.file).catch((error) => {
        console.error(error);
        setStatus(`Failed to open ${sheet.file}`);
      });
    });
    elements.sheetList.append(card);
  }
}

async function openSheet(file) {
  const sheet = state.sheets.find((entry) => entry.file === file);
  if (!sheet) {
    return;
  }

  state.currentSheet = sheet;
  renderSheets();
  const payload = await fetchJson(`/api/annotations?sheet=${encodeURIComponent(file)}`);
  state.annotations = Array.isArray(payload.annotations)
    ? payload.annotations.map(normalizeAnnotation)
    : [];
  state.selectedId = state.annotations[0]?.id ?? null;
  markDirty(false);
  await loadSheetImage(sheet.image_url, sheet.file);
  renderAnnotationList();
  renderInspector();
  renderAnnotations();
}

function loadSheetImage(src, file) {
  return new Promise((resolve, reject) => {
    const image = elements.sheetImage;
    image.onload = () => {
      state.currentImage = image;
      updateCanvasStage();
      setStatus(`${file} • Saved`);
      resolve();
    };
    image.onerror = reject;
    image.src = `${src}?v=${Date.now()}`;
    image.alt = file;
  });
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

function applyAnnotationBoxLayout(box, annotation) {
  box.classList.toggle("selected", annotation.id === state.selectedId);
  box.style.left = `${annotation.x * state.zoom}px`;
  box.style.top = `${annotation.y * state.zoom}px`;
  box.style.width = `${annotation.width * state.zoom}px`;
  box.style.height = `${annotation.height * state.zoom}px`;
  const label = box.querySelector(".annotation-label");
  if (label) {
    label.textContent = annotation.name;
  }
}

function refreshSelectionStyles() {
  for (const child of elements.annotationLayer.children) {
    const id = child.dataset.id;
    child.classList.toggle("selected", id === state.selectedId);
  }
}

function renderAnnotations() {
  elements.annotationLayer.replaceChildren();
  for (const annotation of state.annotations) {
    const box = document.createElement("button");
    box.type = "button";
    box.className = "annotation-box";
    box.dataset.id = annotation.id;

    const label = document.createElement("div");
    label.className = "annotation-label";
    box.append(label);

    const handle = document.createElement("div");
    handle.className = "resize-handle";
    handle.dataset.resize = "true";
    box.append(handle);

    box.addEventListener("pointerdown", onAnnotationPointerDown);
    applyAnnotationBoxLayout(box, annotation);
    elements.annotationLayer.append(box);
  }
}

function renderAnnotationList() {
  elements.annotationList.replaceChildren();
  elements.annotationCountLabel.textContent = `${state.annotations.length}`;
  for (const annotation of state.annotations) {
    const card = document.createElement("button");
    card.type = "button";
    card.className = `annotation-card${annotation.id === state.selectedId ? " selected" : ""}`;
    card.innerHTML = `
      <div class="annotation-name">${annotation.name}</div>
      <div class="annotation-meta">${annotation.type} • ${annotation.x},${annotation.y} • ${annotation.width}x${annotation.height}</div>
    `;
    card.addEventListener("click", () => {
      state.selectedId = annotation.id;
      renderInspector();
      renderAnnotationList();
      renderAnnotations();
    });
    elements.annotationList.append(card);
  }
}

function renderInspector() {
  const annotation = selectedAnnotation();
  const hasSelection = Boolean(annotation);
  elements.selectionEmpty.hidden = hasSelection;
  elements.inspectorForm.hidden = !hasSelection;
  elements.duplicateAnnotationButton.disabled = !hasSelection;
  elements.deleteAnnotationButton.disabled = !hasSelection;
  if (!annotation) {
    return;
  }

  for (const field of inspectorFields) {
    const key = field.dataset.field;
    field.value = annotation[key] ?? "";
  }
}

function updateSelected(patch) {
  const annotation = selectedAnnotation();
  if (!annotation) {
    return;
  }

  Object.assign(annotation, patch);
  if (annotation.width < 1) {
    annotation.width = 1;
  }
  if (annotation.height < 1) {
    annotation.height = 1;
  }
  if (annotation.frame_width < 1) {
    annotation.frame_width = 1;
  }
  if (annotation.frame_height < 1) {
    annotation.frame_height = 1;
  }
  if (annotation.frame_count < 1) {
    annotation.frame_count = 1;
  }
  markDirty(true);
  renderInspector();
  renderAnnotationList();
  renderAnnotations();
}

function addAnnotation() {
  const annotation = defaultAnnotation();
  state.annotations.push(annotation);
  state.selectedId = annotation.id;
  markDirty(true);
  renderAnnotationList();
  renderInspector();
  renderAnnotations();
}

function duplicateSelected() {
  const annotation = selectedAnnotation();
  if (!annotation) {
    return;
  }

  const copy = {
    ...annotation,
    id: makeId(),
    name: `${annotation.name}_copy`,
    x: annotation.x + state.gridSize,
    y: annotation.y + state.gridSize,
  };
  state.annotations.push(copy);
  state.selectedId = copy.id;
  markDirty(true);
  renderAnnotationList();
  renderInspector();
  renderAnnotations();
}

function deleteSelected() {
  if (!state.selectedId) {
    return;
  }

  const next = state.annotations.filter((entry) => entry.id !== state.selectedId);
  state.annotations = next;
  state.selectedId = state.annotations[0]?.id ?? null;
  markDirty(true);
  renderAnnotationList();
  renderInspector();
  renderAnnotations();
}

async function saveAnnotations() {
  if (!state.currentSheet) {
    return;
  }

  const payload = {
    image: state.currentSheet.file,
    annotations: state.annotations,
  };
  setStatus(`Saving ${state.currentSheet.file}…`);
  await fetchJson(`/api/annotations?sheet=${encodeURIComponent(state.currentSheet.file)}`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(payload),
  });
  markDirty(false);
}

function onAnnotationPointerDown(event) {
  event.preventDefault();
  const box = event.currentTarget;
  const annotation = annotationById(box.dataset.id);
  if (!annotation) {
    return;
  }

  state.selectedId = annotation.id;
  renderInspector();
  renderAnnotationList();
  refreshSelectionStyles();

  const isResize = event.target instanceof HTMLElement && event.target.dataset.resize === "true";
  state.drag = {
    id: annotation.id,
    mode: isResize ? "resize" : "move",
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

  const image = state.currentImage;
  const annotation = annotationById(state.drag.id);
  const box = event.currentTarget;
  if (!image || !annotation) {
    return;
  }

  const deltaX = (event.clientX - state.drag.startClientX) / state.zoom;
  const deltaY = (event.clientY - state.drag.startClientY) / state.zoom;

  if (state.drag.mode === "move") {
    annotation.x = clamp(snap(state.drag.startX + deltaX), 0, image.naturalWidth - annotation.width);
    annotation.y = clamp(snap(state.drag.startY + deltaY), 0, image.naturalHeight - annotation.height);
  } else {
    const maxWidth = image.naturalWidth - annotation.x;
    const maxHeight = image.naturalHeight - annotation.y;
    annotation.width = clamp(snap(state.drag.startWidth + deltaX), 1, maxWidth);
    annotation.height = clamp(snap(state.drag.startHeight + deltaY), 1, maxHeight);
  }

  markDirty(true);
  renderInspector();
  renderAnnotationList();
  applyAnnotationBoxLayout(box, annotation);
}

function onAnnotationPointerUp(event) {
  const box = event.currentTarget;
  box.releasePointerCapture(event.pointerId);
  box.removeEventListener("pointermove", onAnnotationPointerMove);
  box.removeEventListener("pointerup", onAnnotationPointerUp);
  box.removeEventListener("pointercancel", onAnnotationPointerUp);
  state.drag = null;
}

function bindEvents() {
  elements.sheetSearchInput.addEventListener("input", filterSheets);
  elements.refreshSheetsButton.addEventListener("click", () => {
    loadSheets().catch((error) => {
      console.error(error);
      setStatus("Failed to load sheets");
    });
  });
  elements.addAnnotationButton.addEventListener("click", addAnnotation);
  elements.duplicateAnnotationButton.addEventListener("click", duplicateSelected);
  elements.deleteAnnotationButton.addEventListener("click", deleteSelected);
  elements.saveButton.addEventListener("click", () => {
    saveAnnotations().catch((error) => {
      console.error(error);
      setStatus("Save failed");
    });
  });
  elements.zoomInput.addEventListener("input", () => {
    state.zoom = Number(elements.zoomInput.value);
    updateCanvasStage();
    renderAnnotations();
  });
  elements.snapEnabledInput.addEventListener("change", () => {
    state.snapEnabled = elements.snapEnabledInput.checked;
  });
  elements.gridSizeInput.addEventListener("change", () => {
    state.gridSize = clamp(Number(elements.gridSizeInput.value) || 8, 1, 128);
    elements.gridSizeInput.value = String(state.gridSize);
  });

  for (const field of inspectorFields) {
    field.addEventListener("input", () => {
      const key = field.dataset.field;
      const numericKeys = new Set(["x", "y", "width", "height", "frame_width", "frame_height", "frame_count"]);
      const value = numericKeys.has(key) ? Number(field.value) || 0 : field.value;
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
}

async function main() {
  bindEvents();
  elements.duplicateAnnotationButton.disabled = true;
  elements.deleteAnnotationButton.disabled = true;
  await loadSheets();
}

main().catch((error) => {
  console.error(error);
  setStatus("Startup failed");
});
