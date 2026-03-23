import { clamp, elements, inspectorFields, makeId, state } from "/annotator_common.js";

export function annotationById(id) {
  return state.annotations.find((entry) => entry.id === id) ?? null;
}

export function selectedAnnotation() {
  return annotationById(state.selectedId);
}

export function normalizeAnnotation(raw) {
  return {
    id: typeof raw.id === "string" && raw.id ? raw.id : makeId(),
    name: typeof raw.name === "string" ? raw.name : "unnamed",
    type: raw.type === "tile" ? "tile" : "sprite",
    frame: Number.isFinite(raw.frame) && raw.frame >= 0 ? raw.frame : 0,
    x: Number.isFinite(raw.x) ? raw.x : 0,
    y: Number.isFinite(raw.y) ? raw.y : 0,
    width: Number.isFinite(raw.width) && raw.width > 0 ? raw.width : 16,
    height: Number.isFinite(raw.height) && raw.height > 0 ? raw.height : 16,
    direction: typeof raw.direction === "string" ? raw.direction : "",
    variant: typeof raw.variant === "string" ? raw.variant : "",
    chroma_key: typeof raw.chroma_key === "string" ? raw.chroma_key : "",
    tags: typeof raw.tags === "string" ? raw.tags : "",
    notes: typeof raw.notes === "string" ? raw.notes : "",
  };
}

export function serializeAnnotation(annotation) {
  return {
    id: annotation.id,
    name: annotation.name,
    type: annotation.type,
    frame: annotation.frame,
    x: annotation.x,
    y: annotation.y,
    width: annotation.width,
    height: annotation.height,
    direction: annotation.direction,
    variant: annotation.variant,
    chroma_key: annotation.chroma_key,
    tags: annotation.tags,
    notes: annotation.notes,
  };
}

export function cloneAnnotations(annotations) {
  return annotations.map((entry) => normalizeAnnotation(entry));
}

export function syncCurrentSheetIntoProject() {
  if (!state.currentSheet) {
    return;
  }

  const record = state.projectSheets.find((sheet) => sheet.file === state.currentSheet.file);
  if (!record) {
    return;
  }

  record.annotations = cloneAnnotations(state.annotations);
  state.currentSheet = record;
}

export function defaultAnnotation() {
  const image = state.currentImage;
  const width = image ? Math.min(16, image.naturalWidth || 16) : 16;
  const height = image ? Math.min(16, image.naturalHeight || 16) : 16;
  const viewX = elements.canvasScroller.scrollLeft / state.zoom;
  const viewY = elements.canvasScroller.scrollTop / state.zoom;
  const centerX = viewX + elements.canvasScroller.clientWidth / (2 * state.zoom) - width / 2;
  const centerY = viewY + elements.canvasScroller.clientHeight / (2 * state.zoom) - height / 2;
  return {
    id: makeId(),
    name: "new_sprite",
    type: "sprite",
    frame: 0,
    x: Math.max(0, Math.round(centerX)),
    y: Math.max(0, Math.round(centerY)),
    width,
    height,
    direction: "",
    variant: "",
    chroma_key: "",
    tags: "",
    notes: "",
  };
}

export function clampAnnotationToImage(annotation) {
  annotation.frame = Math.max(0, Math.round(annotation.frame));
  annotation.x = Math.max(0, Math.round(annotation.x));
  annotation.y = Math.max(0, Math.round(annotation.y));
  annotation.width = Math.max(1, Math.round(annotation.width));
  annotation.height = Math.max(1, Math.round(annotation.height));
  if (!state.currentImage) {
    return;
  }

  annotation.width = clamp(annotation.width, 1, state.currentImage.naturalWidth);
  annotation.height = clamp(annotation.height, 1, state.currentImage.naturalHeight);
  annotation.x = clamp(annotation.x, 0, Math.max(0, state.currentImage.naturalWidth - annotation.width));
  annotation.y = clamp(annotation.y, 0, Math.max(0, state.currentImage.naturalHeight - annotation.height));
}

export function refreshSelectionStyles() {
  for (const child of elements.annotationLayer.children) {
    child.classList.toggle("selected", child.dataset.id === state.selectedId);
  }
}

function applyAnnotationBoxLayout(box, annotation) {
  box.classList.toggle("selected", annotation.id === state.selectedId);
  const order = Number(box.dataset.order) || 0;
  box.style.left = `${annotation.x * state.zoom}px`;
  box.style.top = `${annotation.y * state.zoom}px`;
  box.style.width = `${annotation.width * state.zoom}px`;
  box.style.height = `${annotation.height * state.zoom}px`;
  box.style.zIndex = annotation.id === state.selectedId ? String(state.annotations.length + 10) : String(order + 1);
  const label = box.querySelector(".annotation-label");
  if (label) {
    label.textContent = `${annotation.name} [${annotation.frame}]`;
  }
}

export function updateAnnotationBox(box, annotation) {
  applyAnnotationBoxLayout(box, annotation);
}

export function renderAnnotations(onPointerDown) {
  elements.annotationLayer.replaceChildren();
  for (const [index, annotation] of state.annotations.entries()) {
    const box = document.createElement("button");
    box.type = "button";
    box.className = "annotation-box";
    box.dataset.id = annotation.id;
    box.dataset.order = String(index);

    const label = document.createElement("div");
    label.className = "annotation-label";
    box.append(label);

    const handle = document.createElement("div");
    handle.className = "resize-handle";
    handle.dataset.resize = "true";
    box.append(handle);

    box.addEventListener("pointerdown", onPointerDown);
    applyAnnotationBoxLayout(box, annotation);
    elements.annotationLayer.append(box);
  }
}

export function renderAnnotationList(onSelect) {
  elements.annotationList.replaceChildren();
  elements.annotationCountLabel.textContent = `${state.annotations.length}`;
  const sorted = [...state.annotations].sort((left, right) => {
    const nameDiff = left.name.localeCompare(right.name);
    if (nameDiff !== 0) {
      return nameDiff;
    }
    return left.frame - right.frame;
  });

  for (const annotation of sorted) {
    const card = document.createElement("button");
    card.type = "button";
    card.className = `annotation-card${annotation.id === state.selectedId ? " selected" : ""}`;
    card.innerHTML = `
      <div class="annotation-name">${annotation.name}</div>
      <div class="annotation-meta">frame ${annotation.frame} • ${annotation.x},${annotation.y} • ${annotation.width}x${annotation.height}</div>
    `;
    card.addEventListener("click", () => onSelect(annotation.id));
    elements.annotationList.append(card);
  }
}

export function renderInspector() {
  const annotation = selectedAnnotation();
  const hasSelection = Boolean(annotation);
  elements.selectionEmpty.hidden = hasSelection;
  elements.inspectorForm.hidden = !hasSelection;
  elements.duplicateAnnotationButton.disabled = !hasSelection;
  elements.deleteAnnotationButton.disabled = !hasSelection;
  elements.pickChromaButton.disabled = !hasSelection;
  if (!annotation) {
    return;
  }

  for (const field of inspectorFields) {
    field.value = annotation[field.dataset.field] ?? "";
  }
}
