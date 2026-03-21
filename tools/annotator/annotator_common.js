export const state = {
  sheets: [],
  filteredSheets: [],
  currentSheet: null,
  currentImage: null,
  annotations: [],
  selectedId: null,
  zoom: 2,
  gridSize: 8,
  snapEnabled: true,
  dirty: false,
  drag: null,
};

export const elements = {
  sheetList: document.querySelector("#sheetList"),
  sheetSearchInput: document.querySelector("#sheetSearchInput"),
  refreshSheetsButton: document.querySelector("#refreshSheetsButton"),
  addAnnotationButton: document.querySelector("#addAnnotationButton"),
  duplicateAnnotationButton: document.querySelector("#duplicateAnnotationButton"),
  deleteAnnotationButton: document.querySelector("#deleteAnnotationButton"),
  saveButton: document.querySelector("#saveButton"),
  zoomInput: document.querySelector("#zoomInput"),
  zoomLabel: document.querySelector("#zoomLabel"),
  snapEnabledInput: document.querySelector("#snapEnabledInput"),
  gridSizeInput: document.querySelector("#gridSizeInput"),
  statusLabel: document.querySelector("#statusLabel"),
  canvasScroller: document.querySelector("#canvasScroller"),
  canvasStage: document.querySelector("#canvasStage"),
  sheetImage: document.querySelector("#sheetImage"),
  annotationLayer: document.querySelector("#annotationLayer"),
  selectionEmpty: document.querySelector("#selectionEmpty"),
  inspectorForm: document.querySelector("#inspectorForm"),
  annotationList: document.querySelector("#annotationList"),
  annotationCountLabel: document.querySelector("#annotationCountLabel"),
};

export const inspectorFields = Array.from(document.querySelectorAll("[data-field]"));

export function makeId() {
  if (globalThis.crypto?.randomUUID) {
    return globalThis.crypto.randomUUID();
  }
  return `annotation-${Date.now()}-${Math.floor(Math.random() * 100000)}`;
}

export function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

export function snap(value) {
  if (!state.snapEnabled) {
    return Math.round(value);
  }
  const grid = Math.max(1, state.gridSize);
  return Math.round(value / grid) * grid;
}
