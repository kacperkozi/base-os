"use client";

import {
  useMiniKit,
  useAddFrame,
  useOpenUrl,
} from "@coinbase/onchainkit/minikit";
// import {
//   Name,
//   Identity,
//   Address,
//   Avatar,
//   EthBalance,
// } from "@coinbase/onchainkit/identity";
// import {
//   ConnectWallet,
//   Wallet,
//   WalletDropdown,
//   WalletDropdownDisconnect,
// } from "@coinbase/onchainkit/wallet";
import { useEffect, useMemo, useState, useCallback, useRef } from "react";
import { Button } from "./components/DemoComponents";
import { Icon } from "./components/DemoComponents";
import { Features } from "./components/DemoComponents";
import Link from "next/link";
import { useRouter } from "next/navigation";
import { BrowserQRCodeReader } from "@zxing/browser";

// Minimal typings for the BarcodeDetector API to avoid 'any'
type QRDetector = {
  detect: (source: HTMLVideoElement) => Promise<Array<{ rawValue?: string }>>;
};
declare global {
  interface Window {
    BarcodeDetector?: new (options: { formats: string[] }) => QRDetector;
  }
}

export default function App() {
  const { setFrameReady, isFrameReady, context } = useMiniKit();
  const [frameAdded, setFrameAdded] = useState(false);
  const [activeTab, setActiveTab] = useState("home");
  const [isScannerOpen, setIsScannerOpen] = useState(false);
  const [cameraError, setCameraError] = useState<string | null>(null);
  const videoRef = useRef<HTMLVideoElement | null>(null);
  const mediaStreamRef = useRef<MediaStream | null>(null);
  const rafIdRef = useRef<number | null>(null);
  const detectorRef = useRef<QRDetector | null>(null);
  const [decodedPayload, setDecodedPayload] = useState<string | null>(null);
  const router = useRouter();
  const fileInputRef = useRef<HTMLInputElement | null>(null);
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const zxingReaderRef = useRef<BrowserQRCodeReader | null>(null);
  const zxingAbortRef = useRef<AbortController | null>(null);

  const addFrame = useAddFrame();
  const openUrl = useOpenUrl();

  useEffect(() => {
    if (!isFrameReady) {
      setFrameReady();
    }
  }, [setFrameReady, isFrameReady]);

  const handleAddFrame = useCallback(async () => {
    const frameAdded = await addFrame();
    setFrameAdded(Boolean(frameAdded));
  }, [addFrame]);

  const saveFrameButton = useMemo(() => {
    if (context && !context.client.added) {
      return (
        <Button
          variant="ghost"
          size="sm"
          onClick={handleAddFrame}
          className="text-[var(--app-accent)] p-4"
          icon={<Icon name="plus" size="sm" />}
        >
          Save Frame
        </Button>
      );
    }

    if (frameAdded) {
      return (
        <div className="flex items-center space-x-1 text-sm font-medium text-[#0052FF] animate-fade-out">
          <Icon name="check" size="sm" className="text-[#0052FF]" />
          <span>Saved</span>
        </div>
      );
    }

    return null;
  }, [context, frameAdded, handleAddFrame]);

  const stopZxing = useCallback(() => {
    if (zxingAbortRef.current) {
      zxingAbortRef.current.abort();
      zxingAbortRef.current = null;
    }
    zxingReaderRef.current = null;
  }, []);

  const closeScanner = useCallback(() => {
    setIsScannerOpen(false);
    stopZxing();
    if (mediaStreamRef.current) {
      mediaStreamRef.current.getTracks().forEach((track) => track.stop());
      mediaStreamRef.current = null;
    }
    if (rafIdRef.current !== null) {
      cancelAnimationFrame(rafIdRef.current);
      rafIdRef.current = null;
    }
    detectorRef.current = null;
  }, [stopZxing]);

  const handleImageUpload = useCallback(
    async (e: React.ChangeEvent<HTMLInputElement>) => {
      const file = e.target.files?.[0];
      if (!file) return;
      try {
        const bitmap = await createImageBitmap(file);
        const canvas = canvasRef.current || document.createElement("canvas");
        canvasRef.current = canvas;
        const ctx = canvas.getContext("2d");
        if (!ctx) return;
        canvas.width = bitmap.width;
        canvas.height = bitmap.height;
        ctx.drawImage(bitmap, 0, 0);
        // Use ZXing to decode directly from the canvas
        const reader = new BrowserQRCodeReader();
        const result = await reader.decodeFromCanvas(canvas);
        const value = result.getText();
        if (value) {
          setDecodedPayload(value);
          try {
            sessionStorage.setItem("decodedPayload", value);
          } catch {}
          closeScanner();
          router.push("/broadcast");
        }
      } catch (err) {
        setCameraError(
          err instanceof Error ? err.message : "Unable to decode image",
        );
      }
    },
    [closeScanner, router],
  );

  const openScanner = useCallback(async () => {
    setCameraError(null);
    setIsScannerOpen(true);
    try {
      const constraints: MediaStreamConstraints = {
        video: {
          facingMode: { ideal: "environment" },
        },
        audio: false,
      };

      const stream = await navigator.mediaDevices.getUserMedia(constraints);
      mediaStreamRef.current = stream;
      if (videoRef.current) {
        videoRef.current.srcObject = stream;
        await videoRef.current.play();
      }

      const isBarcodeSupported = typeof window.BarcodeDetector !== "undefined";

      // First try native BarcodeDetector
      if (isBarcodeSupported) {
        if (!detectorRef.current) {
          const DetectorCtor = window.BarcodeDetector!;
          detectorRef.current = new DetectorCtor({ formats: ["qr_code"] });
        }

        const detectFrame = async () => {
          try {
            if (!videoRef.current || !detectorRef.current) return;
            const results = await detectorRef.current.detect(videoRef.current);
            if (results && results.length > 0) {
              const value = results[0].rawValue;
              if (value) {
                setDecodedPayload(value);
                try {
                  sessionStorage.setItem("decodedPayload", value);
                } catch {}
                closeScanner();
                router.push("/broadcast");
                return;
              }
            }
          } catch {
            // swallow
          }
          rafIdRef.current = requestAnimationFrame(detectFrame);
        };
        rafIdRef.current = requestAnimationFrame(detectFrame);
        return;
      }

      // Fallback to ZXing continuous decode if BarcodeDetector is not available
      try {
        const reader = new BrowserQRCodeReader();
        zxingReaderRef.current = reader;
        const abort = new AbortController();
        zxingAbortRef.current = abort;
        await reader.decodeFromVideoDevice(
          undefined,
          videoRef.current!,
          (result) => {
            if (result) {
              const value = result.getText();
              if (value) {
                setDecodedPayload(value);
                try {
                  sessionStorage.setItem("decodedPayload", value);
                } catch {}
                closeScanner();
                router.push("/broadcast");
              }
            }
          },
        );
      } catch (err) {
        setCameraError(
          err instanceof Error ? err.message : "Unable to access camera",
        );
      }
    } catch (err) {
      setCameraError(
        err instanceof Error ? err.message : "Unable to access camera",
      );
    }
  }, [closeScanner, router]);

  return (
    <div className="flex flex-col min-h-screen font-sans text-[var(--app-foreground)] mini-app-theme from-[var(--app-background)] to-[var(--app-gray)]">
      <div
        className="w-full max-w-md mx-auto px-4 pt-9"
        style={{ paddingTop: "calc(env(safe-area-inset-top) + 1.8rem)" }}
      >
        <header className="mb-4 mt-[20vh]">
          <div className="rounded-xl border border-[var(--app-card-border)] bg-[var(--app-card-bg)] p-4 text-center">
            <div className="font-jetbrains text-2xl md:text-3xl tracking-wider leading-tight">
              BASE OS MOBILE
            </div>
            <div className="mt-0.5 text-xs text-[var(--app-foreground-muted)] font-jetbrains">
              Hardware Wallet Transaction Broadcasting for Base
            </div>
          </div>
          {saveFrameButton && (
            <div className="flex justify-end mt-2">{saveFrameButton}</div>
          )}
        </header>

        <main className="flex-1">
          {activeTab === "home" && (
            <div className="space-y-6 animate-fade-in">
              <p className="text-[var(--app-foreground-muted)] text-sm md:text-base max-w-prose">
                Scan your signed transaction payload from BaseOS desktop in
                order to broadcast your transaction
              </p>
              <div className="flex justify-center mb-16">
                <Button
                  variant="primary"
                  size="lg"
                  className="w-full text-white justify-center mx-auto"
                  icon={
                    <svg
                      xmlns="http://www.w3.org/2000/svg"
                      viewBox="0 0 24 24"
                      fill="none"
                      stroke="currentColor"
                      className="w-5 h-5"
                      strokeWidth="2"
                      strokeLinecap="round"
                      strokeLinejoin="round"
                      aria-hidden="true"
                    >
                      <path d="M23 19a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2V9a2 2 0 0 1 2-2h3l2-3h8l2 3h3a2 2 0 0 1 2 2z" />
                      <circle cx="12" cy="13" r="4" />
                    </svg>
                  }
                  onClick={openScanner}
                >
                  Scan
                </Button>
              </div>
              {decodedPayload && (
                <div className="mt-4 p-3 border border-[var(--app-card-border)] rounded-lg bg-[var(--app-card-bg)]">
                  <div className="text-sm font-medium mb-1">
                    Decoded Payload
                  </div>
                  <pre className="whitespace-pre-wrap break-words text-xs">
                    {decodedPayload}
                  </pre>
                </div>
              )}
              <div className="mt-14 space-y-3">
                <h2 className="text-lg md:text-xl font-semibold">
                  Resilience Resources
                </h2>
                <ul className="space-y-1">
                  <li>
                    <Link
                      href="/resources/how-to-use"
                      className="flex items-center justify-between py-2 px-2 rounded-lg hover:bg-[var(--app-accent-light)]"
                    >
                      <span className="text-[var(--app-foreground)]">
                        How to use BaseOS
                      </span>
                      <span className="text-[var(--app-accent)]">→</span>
                    </Link>
                  </li>
                  <li>
                    <Link
                      href="/resources/security-privacy"
                      className="flex items-center justify-between py-2 px-2 rounded-lg hover:bg-[var(--app-accent-light)]"
                    >
                      <span className="text-[var(--app-foreground)]">
                        Increasing your security and privacy
                      </span>
                      <span className="text-[var(--app-accent)]">→</span>
                    </Link>
                  </li>
                  <li>
                    <Link
                      href="/resources/censorship-resistance"
                      className="flex items-center justify-between py-2 px-2 rounded-lg hover:bg-[var(--app-accent-light)]"
                    >
                      <span className="text-[var(--app-foreground)]">
                        Censorship resistance
                      </span>
                      <span className="text-[var(--app-accent)]">→</span>
                    </Link>
                  </li>
                </ul>
              </div>
            </div>
          )}
          {activeTab === "features" && <Features setActiveTab={setActiveTab} />}
        </main>

        <footer className="mt-2 pt-4 flex justify-center">
          <Button
            variant="ghost"
            size="sm"
            className="text-[var(--ock-text-foreground-muted)] text-xs"
            onClick={() => openUrl("https://base.org/builders/minikit")}
          >
            Built on Base with MiniKit
          </Button>
        </footer>
      </div>

      {isScannerOpen && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/70">
          <div className="w-full max-w-md mx-auto px-4">
            <div className="bg-[var(--app-card-bg)] backdrop-blur-md border border-[var(--app-card-border)] rounded-xl overflow-hidden shadow-2xl">
              <div className="flex items-center justify-between px-4 py-3 border-b border-[var(--app-card-border)]">
                <h3 className="font-medium">Scan</h3>
                <button
                  type="button"
                  className="text-[var(--app-foreground-muted)] hover:text-[var(--app-foreground)]"
                  onClick={closeScanner}
                >
                  ×
                </button>
              </div>
              <div className="p-4 space-y-3">
                {cameraError ? (
                  <div className="text-red-500 text-sm">
                    {cameraError}. Please ensure camera permissions are granted
                    and try again. Alternatively, upload a QR image below.
                  </div>
                ) : (
                  <video
                    ref={videoRef}
                    className="w-full h-80 object-cover rounded-lg bg-black"
                    playsInline
                    autoPlay
                    muted
                  />
                )}
                <div className="flex items-center justify-between gap-3">
                  <button
                    type="button"
                    className="border border-[var(--app-card-border)] text-[var(--app-foreground)] px-3 py-2 rounded-lg bg-[var(--app-gray)] hover:bg-[var(--app-gray-dark)]"
                    onClick={() => fileInputRef.current?.click()}
                  >
                    Upload QR Image
                  </button>
                  <input
                    ref={fileInputRef}
                    type="file"
                    accept="image/*"
                    capture="environment"
                    className="hidden"
                    onChange={handleImageUpload}
                  />
                </div>
                <canvas ref={canvasRef} className="hidden" />
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
