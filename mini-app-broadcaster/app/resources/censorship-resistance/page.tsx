export default function Page() {
  return (
    <div className="min-h-screen bg-background text-foreground">
      <div className="w-full max-w-md mx-auto px-4 py-8">
        <h1 className="text-2xl font-bold mb-4">Censorship Resistance</h1>
        <div className="space-y-5 text-sm">
          <div>
            <div className="font-semibold">
              Amnesty International’s Tor site
            </div>
            <p className="text-[var(--app-foreground-muted)]">
              Access reporting and human rights resources via their official
              .onion mirror for secure, uncensored access.
            </p>
            <a
              href="https://www.amnesty.org/en/latest/news/2023/12/global-amnesty-international-website-launches-on-tor-network-to-help-universal-access/"
              target="_blank"
              rel="noopener noreferrer"
              className="text-[var(--app-accent)] underline"
            >
              Amnesty announcement
            </a>
          </div>

          <div>
            <div className="font-semibold">Trusted onion services list</div>
            <p className="text-[var(--app-foreground-muted)]">
              Includes nonprofits and news outlets like EFF, The Guardian,
              ProPublica, and Radio Free Europe/Radio Liberty.
            </p>
            <a
              href="https://en.wikipedia.org/wiki/List_of_Tor_onion_services"
              target="_blank"
              rel="noopener noreferrer"
              className="text-[var(--app-accent)] underline"
            >
              Wikipedia: List of Tor onion services
            </a>
          </div>

          <div>
            <div className="font-semibold">How onion addresses work</div>
            <p className="text-[var(--app-foreground-muted)]">
              Learn how Tor enhances censorship resistance and privacy.
            </p>
            <a
              href="https://freedom.press/issues/onions-side-tracking-tor-availability-reader-privacy-major-news-sites/"
              target="_blank"
              rel="noopener noreferrer"
              className="text-[var(--app-accent)] underline"
            >
              Freedom of the Press Foundation explainer
            </a>
          </div>
        </div>
      </div>
    </div>
  );
}
