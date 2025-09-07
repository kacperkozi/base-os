/**
 * MultiQRProgress Component
 * Displays progress and status for multi-part QR code scanning
 */

import { type MultiQRState, getMissingParts, getScanProgress, formatScanDuration } from '@/lib/multi-qr';

type MultiQRProgressProps = {
  state: MultiQRState;
  onReset: () => void;
  onComplete: () => void;
};

export function MultiQRProgress({ state, onReset, onComplete }: MultiQRProgressProps) {
  const { parts, expectedParts, isComplete } = state;
  const collectedCount = parts.size;
  const progress = getScanProgress(state);
  const missingParts = getMissingParts(state);
  const duration = formatScanDuration(state);

  return (
    <div className="space-y-4 p-4 border-t border-[var(--app-card-border)] bg-[var(--app-card-bg)]">
      {/* Header */}
      <div className="flex justify-between items-center">
        <h3 className="text-sm font-medium text-[var(--app-foreground)]">
          Multi-Part QR Code
        </h3>
        <span className="text-xs text-[var(--app-foreground-muted)]">
          {duration}
        </span>
      </div>
      
      {/* Progress Bar */}
      <div className="space-y-2">
        <div className="flex justify-between text-sm">
          <span className="text-[var(--app-foreground-muted)]">
            Progress
          </span>
          <span className="text-[var(--app-foreground)]">
            {collectedCount} of {expectedParts || '?'} parts
          </span>
        </div>
        
        <div className="w-full bg-[var(--app-gray)] rounded-full h-2">
          <div 
            className="bg-[var(--app-accent)] h-2 rounded-full transition-all duration-300 ease-in-out"
            style={{ 
              width: `${progress}%`
            }}
          />
        </div>
      </div>
      
      {/* Parts Grid */}
      {expectedParts && (
        <div className="space-y-2">
          <div className="text-xs text-[var(--app-foreground-muted)]">
            Scanned parts:
          </div>
          <div className="flex flex-wrap gap-1">
            {Array.from({ length: expectedParts }, (_, i) => {
              const partNum = i + 1;
              const isCollected = parts.has(partNum);
              
              return (
                <div
                  key={partNum}
                  className={`w-8 h-8 rounded text-xs flex items-center justify-center font-medium transition-colors ${
                    isCollected 
                      ? 'bg-[var(--app-accent)] text-white' 
                      : 'bg-[var(--app-gray)] text-[var(--app-foreground-muted)] border border-[var(--app-card-border)]'
                  }`}
                >
                  {partNum}
                </div>
              );
            })}
          </div>
        </div>
      )}
      
      {/* Status Message */}
      <div className="text-sm">
        {isComplete ? (
          <div className="space-y-3">
            <div className="flex items-center gap-2 text-green-500">
              <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M5 13l4 4L19 7" />
              </svg>
              <span className="font-medium">All parts collected!</span>
            </div>
            <button
              onClick={onComplete}
              className="w-full bg-[var(--app-accent)] hover:bg-[var(--app-accent-hover)] text-white py-2 px-4 rounded-lg font-medium transition-colors"
            >
              Proceed to Broadcast
            </button>
          </div>
        ) : (
          <div className="space-y-3">
            <div className="text-[var(--app-foreground-muted)]">
              {missingParts.length > 0 ? (
                <div>
                  <div className="mb-1">Continue scanning to collect remaining parts:</div>
                  <div className="text-xs">
                    Missing: {missingParts.slice(0, 5).join(', ')}
                    {missingParts.length > 5 && ` and ${missingParts.length - 5} more`}
                  </div>
                </div>
              ) : (
                "Keep scanning to collect all QR code parts..."
              )}
            </div>
            
            <div className="flex gap-2">
              <button
                onClick={onReset}
                className="text-xs text-red-500 hover:text-red-400 underline"
              >
                Reset and start over
              </button>
            </div>
          </div>
        )}
      </div>
      
      {/* Instructions */}
      <div className="text-xs text-[var(--app-foreground-muted)] border-t border-[var(--app-card-border)] pt-3">
        <div className="space-y-1">
          <div>• Scan QR codes in any order</div>
          <div>• Already scanned parts will be ignored</div>
          <div>• Scanner will reset after 60 seconds of inactivity</div>
        </div>
      </div>
    </div>
  );
}

/**
 * Toast component for showing brief messages
 */
type ToastProps = {
  message: string;
  type: 'success' | 'warning' | 'error';
  onClose: () => void;
};

export function Toast({ message, type, onClose }: ToastProps) {
  const bgColor = {
    success: 'bg-green-500',
    warning: 'bg-yellow-500', 
    error: 'bg-red-500'
  }[type];

  return (
    <div className="fixed top-4 left-1/2 transform -translate-x-1/2 z-50 animate-fade-in">
      <div className={`${bgColor} text-white px-4 py-2 rounded-lg shadow-lg flex items-center gap-2`}>
        <span className="text-sm">{message}</span>
        <button 
          onClick={onClose}
          className="text-white hover:text-gray-200 ml-2"
        >
          ×
        </button>
      </div>
    </div>
  );
}
