import { serializeTransaction } from "viem";

type IncomingPayload = {
  type?: string | number;
  version?: string;
  data?: {
    hash?: string;
    signature?: { r: `0x${string}`; s: `0x${string}`; v: number };
    transaction?: {
      to?: `0x${string}`;
      value?: string;
      nonce?: number;
      gasPrice?: string;
      gasLimit?: string;
      data?: `0x${string}`;
      chainId?: number;
      maxFeePerGas?: string;
      maxPriorityFeePerGas?: string;
    };
  };
};

export async function POST(request: Request) {
  try {
    const body = await request.json();
    let rawTx = typeof body.rawTx === "string" ? body.rawTx : "";

    // If rawTx is not provided, try to construct it from structured payload
    if (!rawTx && body?.payload) {
      let payload: IncomingPayload | null = null;
      try {
        payload =
          typeof body.payload === "string"
            ? JSON.parse(body.payload)
            : body.payload;
      } catch {
        return Response.json(
          { error: "Invalid payload JSON" },
          { status: 400 },
        );
      }

      const t = payload?.data?.transaction;
      const sig = payload?.data?.signature;
      if (!t || !sig || !t.to || t.value == null || t.nonce == null) {
        return Response.json(
          { error: "Incomplete payload: missing transaction or signature" },
          { status: 400 },
        );
      }

      const isEip1559 =
        t.maxFeePerGas != null && t.maxPriorityFeePerGas != null;
      if (isEip1559) {
        const tx = {
          type: "eip1559" as const,
          chainId: t.chainId ?? 1,
          to: t.to,
          value: BigInt(t.value),
          nonce: t.nonce,
          gas: BigInt(t.gasLimit ?? 21000),
          maxFeePerGas: BigInt(t.maxFeePerGas!),
          maxPriorityFeePerGas: BigInt(t.maxPriorityFeePerGas!),
          data: (t.data ?? "0x") as `0x${string}`,
        };
        rawTx = serializeTransaction(tx, {
          v: BigInt(sig.v),
          r: sig.r,
          s: sig.s,
        });
      } else {
        const tx = {
          type: "legacy" as const,
          chainId: t.chainId ?? 1,
          to: t.to,
          value: BigInt(t.value),
          nonce: t.nonce,
          gas: BigInt(t.gasLimit ?? 21000),
          gasPrice: BigInt(t.gasPrice ?? 0),
          data: (t.data ?? "0x") as `0x${string}`,
        };
        rawTx = serializeTransaction(tx, {
          v: BigInt(sig.v),
          r: sig.r,
          s: sig.s,
        });
      }
    }

    if (!rawTx) {
      return Response.json(
        { error: "Missing rawTx or payload" },
        { status: 400 },
      );
    }

    const rpcUrl =
      process.env.NEXT_PUBLIC_BASE_RPC_URL || "https://base.llamarpc.com";

    const payload = {
      jsonrpc: "2.0",
      id: Date.now(),
      method: "eth_sendRawTransaction",
      params: [rawTx],
    };

    const rpcResponse = await fetch(rpcUrl, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
      // Avoid Next.js caching for RPC calls
      cache: "no-store",
    });

    const contentType = rpcResponse.headers.get("content-type") || "";
    const bodyText = await rpcResponse.text();

    let responseJson: { result?: string; error?: { message?: string } } | null =
      null;
    if (contentType.includes("application/json")) {
      try {
        responseJson = JSON.parse(bodyText);
      } catch {
        // fall through to text error handling
      }
    }

    if (rpcResponse.ok && responseJson && responseJson.result) {
      console.log("Broadcasted tx:", responseJson.result);
      return Response.json({ txHash: responseJson.result });
    }

    const errorMessage =
      responseJson?.error?.message || bodyText || "Broadcast failed";
    return Response.json({ error: errorMessage }, { status: 500 });
  } catch (error) {
    return Response.json(
      { error: error instanceof Error ? error.message : "Unknown error" },
      { status: 500 },
    );
  }
}
