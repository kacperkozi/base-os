/**
 * Multi-QR Code Utilities for BaseOS Mobile App
 * Handles chunked QR codes from BaseOS desktop application
 */

export type QRPart = {
  part: number;
  totalParts: number;
  data: string;
  rawValue: string;
  timestamp: number;
};

export type MultiQRState = {
  parts: Map<number, QRPart>;
  expectedParts: number | null;
  isComplete: boolean;
  assembledData: string | null;
  lastScanTime: number;
  scanStartTime: number;
};

/**
 * Parse QR value to check if it's a multi-part QR code
 * Format: "P1/3:data" where 1 is part number, 3 is total parts
 */
export function parseQRValue(value: string): QRPart | null {
  const match = value.match(/^P(\d+)\/(\d+):(.*)$/);
  if (!match) return null;
  
  const part = parseInt(match[1], 10);
  const totalParts = parseInt(match[2], 10);
  const data = match[3];
  
  // Validation
  if (part < 1 || totalParts < 1 || part > totalParts) {
    return null;
  }
  
  return {
    part,
    totalParts,
    data,
    rawValue: value,
    timestamp: Date.now()
  };
}

/**
 * Create initial multi-QR state
 */
export function createMultiQRState(): MultiQRState {
  return {
    parts: new Map(),
    expectedParts: null,
    isComplete: false,
    assembledData: null,
    lastScanTime: 0,
    scanStartTime: Date.now()
  };
}

/**
 * Update multi-QR state with a new scanned part
 */
export function updateMultiQRState(
  state: MultiQRState, 
  qrPart: QRPart
): MultiQRState {
  const newParts = new Map(state.parts);
  
  // Check for duplicate
  if (newParts.has(qrPart.part)) {
    // Return state unchanged if duplicate
    return {
      ...state,
      lastScanTime: Date.now()
    };
  }
  
  // Add new part
  newParts.set(qrPart.part, qrPart);
  
  // Check if complete
  const isComplete = newParts.size === qrPart.totalParts;
  const assembledData = isComplete ? assembleQRParts(newParts) : null;
  
  return {
    parts: newParts,
    expectedParts: qrPart.totalParts,
    isComplete,
    assembledData,
    lastScanTime: Date.now(),
    scanStartTime: state.scanStartTime
  };
}

/**
 * Assemble QR parts into original data
 */
export function assembleQRParts(parts: Map<number, QRPart>): string | null {
  if (parts.size === 0) return null;
  
  const firstPart = Array.from(parts.values())[0];
  if (parts.size !== firstPart.totalParts) return null;
  
  // Sort parts by part number and concatenate data
  let assembled = "";
  for (let i = 1; i <= firstPart.totalParts; i++) {
    const part = parts.get(i);
    if (!part) return null; // Missing part
    assembled += part.data;
  }
  
  return assembled;
}

/**
 * Validate assembled data
 */
export function validateAssembledData(data: string): boolean {
  if (!data || data.length === 0) return false;
  
  try {
    // Try to parse as JSON transaction
    const parsed = JSON.parse(data);
    
    // Check for required transaction fields
    return !!(
      parsed?.type &&
      parsed?.data?.transaction &&
      parsed?.data?.signature
    );
  } catch {
    // Could be raw hex transaction
    return data.startsWith('0x') && data.length > 10;
  }
}

/**
 * Get missing parts list
 */
export function getMissingParts(state: MultiQRState): number[] {
  if (!state.expectedParts) return [];
  
  const missing: number[] = [];
  for (let i = 1; i <= state.expectedParts; i++) {
    if (!state.parts.has(i)) {
      missing.push(i);
    }
  }
  return missing;
}

/**
 * Check if multi-QR scan has timed out (60 seconds)
 */
export function hasMultiQRTimedOut(state: MultiQRState): boolean {
  const now = Date.now();
  return (now - state.lastScanTime) > 60000 && state.parts.size > 0;
}

/**
 * Reset multi-QR state
 */
export function resetMultiQRState(): MultiQRState {
  return createMultiQRState();
}

/**
 * Get scan progress percentage
 */
export function getScanProgress(state: MultiQRState): number {
  if (!state.expectedParts) return 0;
  return Math.round((state.parts.size / state.expectedParts) * 100);
}

/**
 * Format scan duration for display
 */
export function formatScanDuration(state: MultiQRState): string {
  const duration = Math.round((state.lastScanTime - state.scanStartTime) / 1000);
  if (duration < 60) {
    return `${duration}s`;
  } else {
    const minutes = Math.floor(duration / 60);
    const seconds = duration % 60;
    return `${minutes}m ${seconds}s`;
  }
}

/**
 * Create payload metadata for storage
 */
export function createPayloadMetadata(state: MultiQRState) {
  return {
    type: 'multi' as const,
    totalParts: state.expectedParts || 0,
    scanDuration: state.lastScanTime - state.scanStartTime,
    partsOrder: Array.from(state.parts.keys()).sort((a, b) => a - b),
    completedAt: new Date().toISOString()
  };
}
