export const state = {
  sheets: [],
  filteredSheets: [],
  projectSheets: [],
  currentSheet: null,
  currentImage: null,
  annotations: [],
  selectedId: null,
  zoom: 2,
  dirty: false,
  drag: null,
  colorPickArmed: false,
  sampleCanvas: null,
  sampleContext: null,
};

export const elements = {
  sheetList: document.querySelector("#sheetList"),
  sheetSearchInput: document.querySelector("#sheetSearchInput"),
  refreshSheetsButton: document.querySelector("#refreshSheetsButton"),
  addAnnotationButton: document.querySelector("#addAnnotationButton"),
  duplicateAnnotationButton: document.querySelector("#duplicateAnnotationButton"),
  deleteAnnotationButton: document.querySelector("#deleteAnnotationButton"),
  saveButton: document.querySelector("#saveButton"),
  pickChromaButton: document.querySelector("#pickChromaButton"),
  zoomOutButton: document.querySelector("#zoomOutButton"),
  zoomLabel: document.querySelector("#zoomLabel"),
  zoomInButton: document.querySelector("#zoomInButton"),
  statusLabel: document.querySelector("#statusLabel"),
  canvasScroller: document.querySelector("#canvasScroller"),
  canvasStage: document.querySelector("#canvasStage"),
  sheetImage: document.querySelector("#sheetImage"),
  annotationLayer: document.querySelector("#annotationLayer"),
  selectionEmpty: document.querySelector("#selectionEmpty"),
  inspectorForm: document.querySelector("#inspectorForm"),
  annotationList: document.querySelector("#annotationList"),
  annotationCountLabel: document.querySelector("#annotationCountLabel"),
  galleryList: document.querySelector("#galleryList"),
  galleryCountLabel: document.querySelector("#galleryCountLabel"),
};

export const inspectorFields = Array.from(document.querySelectorAll("[data-field]"));
export const ZOOM_MIN = 0.5;
export const ZOOM_MAX = 12;
export const ZOOM_FACTOR = 1.15;

export function makeId() {
  if (globalThis.crypto?.randomUUID) {
    return globalThis.crypto.randomUUID();
  }
  return `annotation-${Date.now()}-${Math.floor(Math.random() * 100000)}`;
}

export function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}
