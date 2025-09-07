export default function Page() {
  return (
    <div className="min-h-screen bg-background text-foreground">
      <div className="w-full max-w-md mx-auto px-4 py-8">
        <h1 className="text-2xl font-bold mb-4">
          Increasing your security and privacy
        </h1>

        <div className="space-y-6 text-sm">
          <div>
            <div className="font-semibold">Stay private online: Proton VPN</div>
            <p className="text-[var(--app-foreground-muted)]">
              A trusted, no-logs VPN service based in Switzerland, designed for
              privacy and censorship resistance.
            </p>
            <a
              href="https://protonvpn.com/"
              target="_blank"
              rel="noopener noreferrer"
              className="text-[var(--app-accent)] underline"
            >
              Proton VPN
            </a>
          </div>

          <div>
            <div className="font-semibold">Spyware detection</div>
            <div className="mt-2 space-y-3">
              <div>
                <div className="font-medium">
                  Norton – Pegasus Spyware Guide
                </div>
                <p className="text-[var(--app-foreground-muted)]">
                  Learn how Pegasus works and signs your device may be
                  compromised.
                </p>
                <a
                  href="https://us.norton.com/blog/emerging-threats/pegasus-spyware"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="text-[var(--app-accent)] underline"
                >
                  Norton guide
                </a>
              </div>

              <div>
                <div className="font-medium">Check your phone manually</div>
                <p className="text-[var(--app-foreground-muted)]">
                  Signs include overheating, rapid battery drain, strange
                  pop-ups, or unusual messages. (Wired)
                </p>
                <a
                  href="https://www.wired.com/story/how-to-check-for-stalkerware/"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="text-[var(--app-accent)] underline"
                >
                  Wired guide
                </a>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
