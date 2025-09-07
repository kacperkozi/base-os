"use client";

import { useEffect, useState, useCallback } from "react";
import Link from "next/link";
import { useRouter } from "next/navigation";

type PayloadMetadata = {
  type: 'single' | 'multi';
  totalParts?: number;
  scanDuration?: number;
  partsOrder?: number[];
  completedAt?: string;
};

export default function BroadcastPage() {
  const [displayPayload, setDisplayPayload] = useState<string>("");
  const [rawTx, setRawTx] = useState<string>("");
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [txHash, setTxHash] = useState<string | null>(null);
  const [metadata, setMetadata] = useState<PayloadMetadata | null>(null);
  const router = useRouter();

  useEffect(() => {
    try {
      const value = sessionStorage.getItem("decodedPayload") || "";
      const metadataStr = sessionStorage.getItem("payloadMetadata") || "";
      
      // Load metadata if available
      if (metadataStr) {
        try {
          const parsedMetadata = JSON.parse(metadataStr) as PayloadMetadata;
          setMetadata(parsedMetadata);
        } catch {
          setMetadata(null);
        }
      }
      
      if (!value) {
        setDisplayPayload("");
        setRawTx("");
        return;
      }
      
      // Pretty-print JSON for display; extract rawTx if present
      try {
        const parsed = JSON.parse(value);
        setDisplayPayload(JSON.stringify(parsed, null, 2));
        const maybeRaw = parsed?.data?.raw;
        if (typeof maybeRaw === "string" && maybeRaw.startsWith("0x")) {
          setRawTx(maybeRaw.trim());
        } else if (value.startsWith("0x")) {
          setRawTx(value.trim());
        } else {
          setRawTx("");
        }
      } catch {
        setDisplayPayload(value);
        setRawTx(value.startsWith("0x") ? value.trim() : "");
      }
    } catch {
      setDisplayPayload("");
      setRawTx("");
    }
  }, []);

  const handleReject = useCallback(() => {
    router.back();
  }, [router]);

  const handleBroadcast = useCallback(() => {
    setError(null);
    setTxHash(null);
    setIsSubmitting(true);
    (async () => {
      try {
        console.log("raw tx", rawTx);
        const res = await fetch("/api/broadcast", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(
            rawTx ? { rawTx: rawTx.trim() } : { payload: displayPayload },
          ),
          cache: "no-store",
        });
        const json = await res.json();
        if (!res.ok) {
          throw new Error(json?.error || "Broadcast failed");
        }
        setTxHash(json.txHash);
        console.log("Broadcast successful:", json.txHash);
        const url = `/broadcast/confirmation?txHash=${encodeURIComponent(json.txHash)}`;
        try {
          router.push(url);
        } catch {}
        // Safari fallback: force navigation after a short delay
        setTimeout(() => {
          if (typeof window !== "undefined") {
            window.location.assign(url);
          }
        }, 50);
      } catch (e) {
        setError(e instanceof Error ? e.message : "Broadcast failed");
      } finally {
        setIsSubmitting(false);
      }
    })();
  }, [rawTx, displayPayload, router]);

  return (
    <>
      <div className="min-h-screen bg-background text-foreground">
        <div className="w-full max-w-md mx-auto px-4 py-8">
          <h1 className="text-2xl font-bold mb-4">Decoded Payload</h1>
          
          {/* Multi-QR Metadata Display */}
          {metadata?.type === 'multi' && (
            <div className="mb-4 rounded-xl overflow-hidden border border-blue-300 bg-blue-50 shadow">
              <div className="px-4 py-2 border-b border-blue-200 text-xs text-blue-800">
                Multi-part transaction
              </div>
              <div className="p-4 space-y-2">
                <div className="flex justify-between text-sm">
                  <span className="text-blue-700">Parts assembled:</span>
                  <span className="font-medium text-blue-900">{metadata.totalParts}</span>
                </div>
                <div className="flex justify-between text-sm">
                  <span className="text-blue-700">Scan duration:</span>
                  <span className="font-medium text-blue-900">
                    {metadata.scanDuration ? `${Math.round(metadata.scanDuration / 1000)}s` : 'Unknown'}
                  </span>
                </div>
                <div className="flex justify-between text-sm">
                  <span className="text-blue-700">Parts order:</span>
                  <span className="font-medium text-blue-900 font-mono text-xs">
                    {metadata.partsOrder?.join(' → ') || 'Unknown'}
                  </span>
                </div>
                <div className="text-xs text-blue-600 mt-2">
                  ✅ Successfully reconstructed from {metadata.totalParts} QR code parts
                </div>
              </div>
            </div>
          )}
          
          {displayPayload ? (
            <div className="mb-4 rounded-xl overflow-hidden border border-[var(--app-card-border)] bg-[var(--app-card-bg)] shadow">
              <div className="px-4 py-2 border-b border-[var(--app-card-border)] text-xs text-[var(--app-foreground-muted)]">
                Preview
              </div>
              <pre className="p-4 whitespace-pre-wrap break-words text-sm font-mono max-h-[55vh] overflow-auto">
                {displayPayload}
              </pre>
            </div>
          ) : (
            <div className="mb-4 text-[var(--app-foreground-muted)] text-sm">
              No payload found.
            </div>
          )}
          {error && <div className="mb-3 text-red-500 text-sm">{error}</div>}
          {txHash && (
            <div className="mb-3 text-sm">
              Broadcasted. Tx Hash: <span className="break-all">{txHash}</span>
            </div>
          )}
          <div className="sticky bottom-0 bg-background pt-3 pb-[calc(env(safe-area-inset-bottom)+12px)]">
            <div className="flex items-center justify-between gap-3">
              <button
                type="button"
                className="border border-[var(--app-card-border)] text-[var(--app-foreground)] px-4 py-2 rounded-lg bg-[var(--app-gray)] hover:bg-[var(--app-gray-dark)] w-full sm:w-auto"
                onClick={handleReject}
              >
                Reject
              </button>
              <button
                type="button"
                className="bg-[var(--app-accent)] hover:bg-[var(--app-accent-hover)] text-white px-4 py-2 rounded-lg w-full sm:w-auto"
                onClick={handleBroadcast}
                disabled={isSubmitting || (!rawTx && !displayPayload)}
              >
                {isSubmitting ? "Broadcasting..." : "Broadcast"}
              </button>
            </div>
          </div>
          <div className="mt-6 text-xs text-[var(--app-foreground-muted)]">
            <Link href="/">Back to Home</Link>
          </div>
        </div>
      </div>
    </>
  );
}
