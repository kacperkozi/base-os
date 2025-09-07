import Link from "next/link";

export const dynamic = "force-dynamic";

export default async function BroadcastConfirmationPage({
  searchParams,
}: {
  searchParams: Promise<Record<string, string | string[] | undefined>>;
}) {
  const params = await searchParams;
  const txHashParam = params?.txHash;
  const txHash = Array.isArray(txHashParam)
    ? txHashParam[0]
    : txHashParam || "";

  type GlobalWithProcess = typeof globalThis & {
    process?: { env?: { NEXT_PUBLIC_BASESCAN_URL?: string } };
  };
  const basescanBaseUrl =
    (globalThis as GlobalWithProcess).process?.env?.NEXT_PUBLIC_BASESCAN_URL ??
    "https://basescan.org";
  const basescanTxUrl = txHash
    ? `${basescanBaseUrl}/tx/${txHash}`
    : basescanBaseUrl;

  return (
    <div className="min-h-screen bg-background text-foreground">
      <div className="w-full max-w-md mx-auto px-4 py-8">
        <h1 className="text-2xl font-bold mb-6 text-center">
          Transaction sent successfully
        </h1>

        <div className="rounded-xl overflow-hidden border border-[var(--app-card-border)] bg-[var(--app-card-bg)] shadow">
          <div className="px-4 py-2 border-b border-[var(--app-card-border)] text-xs text-[var(--app-foreground-muted)]">
            Transaction details
          </div>
          <div className="p-4">
            <div className="flex items-start gap-3">
              <div className="flex-1 min-w-0">
                <div className="text-xs text-[var(--app-foreground-muted)] mb-1">
                  Transaction hash
                </div>
                <div className="font-mono text-sm break-all">
                  {txHash || "—"}
                </div>
              </div>
              <a
                href={basescanTxUrl}
                target="_blank"
                rel="noopener noreferrer"
                className="shrink-0 inline-flex items-center gap-2 bg-[var(--app-accent)] hover:bg-[var(--app-accent-hover)] text-white px-3 py-2 rounded-lg"
              >
                View on Basescan
              </a>
            </div>
          </div>
        </div>

        <div className="mt-8">
          <Link
            href="/"
            className="block text-center bg-[var(--app-accent)] hover:bg-[var(--app-accent-hover)] text-white px-4 py-2 rounded-lg"
          >
            Broadcast more
          </Link>
        </div>
      </div>
    </div>
  );
}
