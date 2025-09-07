export default function Page() {
  return (
    <div className="min-h-screen bg-background text-foreground">
      <div className="w-full max-w-md mx-auto px-4 py-8">
        <h1 className="text-2xl font-bold mb-4">How to Use BaseOS</h1>
        <ol className="list-decimal pl-5 space-y-3 text-sm leading-6">
          <li>Power off the computer you intend to use.</li>
          <li>Insert the USB stick containing BaseOS.</li>
          <li>
            Boot the device in UEFI mode and select BaseOS as the startup
            option.
          </li>
          <li>Generate your transaction payload using BaseOS.</li>
          <li>
            Connect your hardware wallet and securely sign the transaction.
          </li>
          <li>
            Use the BaseOS mobile app to scan the QR code provided by the
            BaseOS, and broadcast the transaction at your convenience.
          </li>
          <li>That’s it, you’re done!</li>
        </ol>
      </div>
    </div>
  );
}
