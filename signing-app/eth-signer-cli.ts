#!/usr/bin/env ts-node

/**
 * Simple CLI script to connect to Ledger device and sign Ethereum transfer transactions
 * 
 * Usage:
 *   npx ts-node eth-signer-cli.ts --to 0x0000000000000000000000000000000000000000 --amount 0.1 --nonce 0
 * 
 * This script will:
 * 1. Connect to the first available Ledger device
 * 2. Get the Ethereum address for the default derivation path
 * 3. Build a simple ETH transfer transaction
 * 4. Sign the transaction with the Ledger device
 * 5. Output the signed raw transaction as a hex string
 */

import { ethers } from 'ethers';
import TransportNodeHid from '@ledgerhq/hw-transport-node-hid';
import Eth from '@ledgerhq/hw-app-eth';
import * as qrcode from 'qrcode-terminal';

// Types
interface TransactionOptions {
  to: string;
  amount: string;
  nonce?: string;
  gasPrice?: string;
  gasLimit?: string;
  chainId?: string;
}

interface ParsedOptions {
  to: string;
  amount: ethers.BigNumber;
  nonce: number;
  gasPrice: ethers.BigNumber;
  gasLimit: number;
  chainId: number;
}

interface LedgerDevice {
  path: string;
  productId?: number;
  vendorId?: number;
}

interface EthAddress {
  publicKey: string;
  address: string;
  chainCode?: string;
}

interface EthSignature {
  r: string;
  s: string;
  v: string;
}

// Default values
const DEFAULT_DERIVATION_PATH = "44'/60'/0'/0/0"; // First Ethereum account
const DEFAULT_CHAIN_ID = 1; // Ethereum mainnet
const DEFAULT_GAS_LIMIT = 21000; // Standard ETH transfer gas limit
const DEFAULT_GAS_PRICE = ethers.utils.parseUnits('0.027', 'gwei'); // 20 gwei

// Parse command line arguments
function parseArgs(): TransactionOptions {
  const args = process.argv.slice(2);
  const options: Partial<TransactionOptions> = {};
  
  for (let i = 0; i < args.length; i += 2) {
    const key = args[i].replace('--', '') as keyof TransactionOptions;
    const value = args[i + 1];
    
    if (key === 'to' || key === 'amount' || key === 'nonce' || key === 'gasPrice' || key === 'gasLimit' || key === 'chainId') {
      options[key] = value;
    }
  }
  
  return options as TransactionOptions;
}

// Validate required arguments
function validateArgs(options: TransactionOptions): ParsedOptions {
  if (!options.to) {
    throw new Error('--to address is required');
  }
  
  if (!options.amount) {
    throw new Error('--amount is required');
  }
  
  // Validate Ethereum address
  if (!ethers.utils.isAddress(options.to)) {
    throw new Error('Invalid Ethereum address for --to');
  }
  
  return {
    to: options.to,
    amount: ethers.utils.parseEther(options.amount),
    nonce: options.nonce ? parseInt(options.nonce) : 0,
    gasPrice: options.gasPrice ? ethers.utils.parseUnits(options.gasPrice, 'gwei') : DEFAULT_GAS_PRICE,
    gasLimit: options.gasLimit ? parseInt(options.gasLimit) : DEFAULT_GAS_LIMIT,
    chainId: options.chainId ? parseInt(options.chainId) : DEFAULT_CHAIN_ID
  };
}

// Get the first available Ledger device
async function getLedgerDevice(): Promise<string> {
  console.log('🔍 Looking for Ledger devices...');
  
  const devices = await TransportNodeHid.list();
  if (devices.length === 0) {
    throw new Error('No Ledger devices found. Please connect your Ledger device and make sure the Ethereum app is open.');
  }
  
  console.log(`✅ Found ${devices.length} Ledger device(s)`);
  return devices[0].path;
}

// Connect to Ledger and get Ethereum app instance
async function connectToLedger(devicePath: string): Promise<{ transport: TransportNodeHid; eth: Eth }> {
  console.log('🔌 Connecting to Ledger device...');
  
  const transport = await TransportNodeHid.open(devicePath);
  const eth = new Eth(transport);
  
  console.log('✅ Connected to Ledger device');
  return { transport, eth };
}

// Get address from Ledger
async function getAddress(eth: Eth, derivationPath: string = DEFAULT_DERIVATION_PATH): Promise<string> {
  console.log(`📍 Getting address for derivation path: ${derivationPath}`);
  
  const result: EthAddress = await eth.getAddress(derivationPath, false, false);
  console.log(`✅ Address: ${result.address}`);
  
  return result.address;
}

// Build transaction
function buildTransaction(
  from: string,
  to: string,
  amount: ethers.BigNumber,
  nonce: number,
  gasPrice: ethers.BigNumber,
  gasLimit: number,
  chainId: number
): ethers.utils.UnsignedTransaction {
  console.log('🔨 Building transaction...');
  
  const transaction: ethers.utils.UnsignedTransaction = {
    to: to,
    value: amount,
    nonce: nonce,
    gasPrice: gasPrice,
    gasLimit: gasLimit,
    chainId: chainId,
    type: 0 // Legacy transaction
  };
  
  console.log('📋 Transaction details:');
  console.log(`   From: ${from}`);
  console.log(`   To: ${to}`);
  console.log(`   Amount: ${ethers.utils.formatEther(amount)} ETH`);
  console.log(`   Nonce: ${nonce}`);
  console.log(`   Gas Price: ${ethers.utils.formatUnits(gasPrice, 'gwei')} gwei`);
  console.log(`   Gas Limit: ${gasLimit}`);
  console.log(`   Chain ID: ${chainId}`);
  
  return transaction;
}

// Serialize transaction for signing
function serializeTransaction(transaction: ethers.utils.UnsignedTransaction): string {
  // Convert BigNumber objects to proper values for serialization
  const cleanTransaction = {
    to: transaction.to,
    value: (transaction.value && typeof transaction.value === 'object' && 'hex' in transaction.value) 
      ? (transaction.value as any).hex 
      : transaction.value,
    nonce: transaction.nonce,
    gasPrice: (transaction.gasPrice && typeof transaction.gasPrice === 'object' && 'hex' in transaction.gasPrice) 
      ? (transaction.gasPrice as any).hex 
      : transaction.gasPrice,
    gasLimit: (transaction.gasLimit && typeof transaction.gasLimit === 'object' && 'hex' in transaction.gasLimit) 
      ? (transaction.gasLimit as any).hex 
      : transaction.gasLimit,
    chainId: transaction.chainId,
    type: transaction.type
  };
  
  const serialized = ethers.utils.serializeTransaction(cleanTransaction);
  // Remove 0x prefix for Ledger
  return serialized.slice(2);
}

// Sign transaction with Ledger
async function signTransaction(eth: Eth, derivationPath: string, rawTxHex: string): Promise<EthSignature> {
  console.log('✍️  Signing transaction with Ledger...');
  console.log('   Please confirm the transaction on your Ledger device');
  
  try {
    const signature: EthSignature = await eth.signTransaction(derivationPath, rawTxHex, null);
    console.log('✅ Transaction signed successfully');
    
    return signature;
  } catch (error: any) {
    if (error.message && error.message.includes('Please enable Blind signing')) {
      throw new Error('Please enable "Blind signing" in the Ethereum app settings on your Ledger device');
    }
    throw error;
  }
}

// To fix an error of 2 leading 0s - from MOACChain issue #24
const trimLeadingZero = function (hex: string): string {
  while (hex && hex.startsWith('0x00')) {
    hex = '0x' + hex.slice(4);
  }
  return hex;
};

// Create final signed transaction
function createSignedTransaction(
  transaction: ethers.utils.UnsignedTransaction,
  signature: EthSignature
): string {

  // Helper function to convert to canonical hex bytes (no leading zeros)
  const toCanonicalBytes = (value: any): Uint8Array => {
    // Handle explicit zero values - RLP canonical encoding for 0 is empty byte array
    if (value === 0 || value === '0' || value === '0x0' || 
        (value && typeof value === 'object' && value.hex === '0x0') ||
        (value && typeof value === 'object' && value.toHexString && value.toHexString() === '0x0') ||
        (value && typeof value === 'object' && value.toHexString && value.toHexString() === '0x00')) {
      return new Uint8Array(0);
    }
    
    let hex: string;
    if (typeof value === 'string' && value.startsWith('0x')) {
      const clean = value.slice(2).replace(/^0+/, '');
      hex = clean === '' ? '0' : clean;
    } else if (typeof value === 'number' || typeof value === 'bigint') {
      hex = value.toString(16);
    } else if (value && typeof value === 'object' && value.hex) {
      // Handle BigNumber objects
      hex = value.hex.slice(2).replace(/^0+/, '');
      hex = hex === '' ? '0' : hex;
    } else if (value && typeof value === 'object' && value.toHexString) {
      // Handle ethers BigNumber objects
      const hexString = value.toHexString();
      hex = hexString.slice(2).replace(/^0+/, '');
      hex = hex === '' ? '0' : hex;
    } else {
      hex = value.toString(16);
    }
    
    // If after cleaning, it's empty or just '0', return empty array for canonical RLP
    if (hex === '' || hex === '0') {
      return new Uint8Array(0);
    }
    
    // Convert hex string to bytes
    if (hex.length % 2 === 1) {
      hex = '0' + hex;
    }
    return ethers.utils.arrayify('0x' + hex);
  };
  
  // Create transaction array with canonical byte values
  const txArray = [
    toCanonicalBytes(transaction.nonce || 0),
    toCanonicalBytes(transaction.gasPrice || 0),
    toCanonicalBytes(transaction.gasLimit || 0),
    ethers.utils.arrayify(transaction.to || '0x0'),
    toCanonicalBytes(transaction.value || 0),
    new Uint8Array(0), // data (empty for simple transfers)
    toCanonicalBytes(parseInt(signature.v, 16) || 0),
    ethers.utils.arrayify(trimLeadingZero(signature.r.startsWith('0x') ? signature.r : '0x' + signature.r)),
    ethers.utils.arrayify(trimLeadingZero(signature.s.startsWith('0x') ? signature.s : '0x' + signature.s))
  ];
  
  // Use ethers RLP encoding
  const serialized = '0x' + ethers.utils.RLP.encode(txArray).slice(2);
  return serialized;
}

// Main function
async function main(): Promise<void> {
  try {
    console.log('🚀 Ethereum Ledger Signer CLI (TypeScript)');
    console.log('==========================================\n');
    
    // Parse and validate arguments
    const rawOptions = parseArgs();
    const options = validateArgs(rawOptions);
    
    // Get Ledger device
    const devicePath = await getLedgerDevice();
    
    // Connect to Ledger
    const { transport, eth } = await connectToLedger(devicePath);
    
    try {
      // Get address
      const fromAddress = await getAddress(eth);
      //const fromAddress = "0xa8afD84cd993A24BeC10FDC5d6e2A0dB878C1D92";
      
      // Build transaction
      const transaction = buildTransaction(
        fromAddress,
        options.to,
        options.amount,
        options.nonce,
        options.gasPrice,
        options.gasLimit,
        options.chainId
      );
      
      // const transaction = JSON.parse(
      //   '{"to":"0x8c47B9fADF822681C68f34fd9b0D3063569245A1","value":{"type":"BigNumber","hex":"0x0f4240"},"nonce":1,"gasPrice":{"type":"BigNumber","hex":"0x04a817c800"},"gasLimit":21000,"chainId":8453,"type":0}'
      // );

      // Serialize transaction
      const rawTxHex = serializeTransaction(transaction);
      
      // Display unsigned transaction hex
      console.log(`📄 Unsigned Transaction Hex: 0x${rawTxHex}`);
      
      // Sign transaction
      const signature = await signTransaction(eth, DEFAULT_DERIVATION_PATH, rawTxHex);

      // const signature = {
      //   r: "0x60dd71f1a9cd8bfa6a84e360826ba4e66b4ef789291c0592a80b9d14719ec2da",
      //   s: "0x3f486ca88006e7c4bf6fc055b4613632c891cf60fcf976ad10f1fc34c0070087",
      //   v: "422d"
      // }

      console.log('Signature:', signature);


        // Display signature components
        console.log('\n🔐 Signature Components:');
        console.log(JSON.stringify({
          r: trimLeadingZero(signature.r.startsWith('0x') ? signature.r : '0x' + signature.r),
          s: trimLeadingZero(signature.s.startsWith('0x') ? signature.s : '0x' + signature.s),
          v: signature.v
        }, null, 2));

        console.log('Transaction:');
        console.log(JSON.stringify(transaction));
        
      // Create final signed transaction
      const signedTx = createSignedTransaction(transaction, signature);
      
    
      // Create final JSON object
      const txHash = ethers.utils.keccak256(signedTx);
      const finalJson = {
        type: "1",
        version: "1.0",
        data: {
          hash: txHash,
          signature: {
            r: trimLeadingZero(signature.r.startsWith('0x') ? signature.r : '0x' + signature.r),
            s: trimLeadingZero(signature.s.startsWith('0x') ? signature.s : '0x' + signature.s),
            v: signature.v.startsWith('0x') ? signature.v : '0x' + signature.v
          },
          transaction: {
            to: transaction.to,
            value: (transaction.value && typeof transaction.value === 'object' && 'hex' in transaction.value) 
              ? (transaction.value as any).hex 
              : ethers.utils.hexlify(transaction.value || 0),
            nonce: transaction.nonce || 0,
            gasPrice: (transaction.gasPrice && typeof transaction.gasPrice === 'object' && 'hex' in transaction.gasPrice) 
              ? (transaction.gasPrice as any).hex 
              : ethers.utils.hexlify(transaction.gasPrice || 0),
            gasLimit: (transaction.gasLimit && typeof transaction.gasLimit === 'object' && 'hex' in transaction.gasLimit) 
              ? (transaction.gasLimit as any).hex 
              : ethers.utils.hexlify(transaction.gasLimit || 0),
            data: "0x",
            chainId: transaction.chainId || 1
          },
          timestamp: Date.now(),
          network: transaction.chainId === 1 ? "ethereum" : transaction.chainId === 8453 ? "base" : `chain-${transaction.chainId || 1}`
        },
        checksum: ethers.utils.keccak256(ethers.utils.toUtf8Bytes(JSON.stringify({
          hash: txHash,
          signature: signature,
          transaction: {
            to: transaction.to,
            value: (transaction.value && typeof transaction.value === 'object' && 'hex' in transaction.value) 
              ? (transaction.value as any).hex 
              : ethers.utils.hexlify(transaction.value || 0),
            nonce: transaction.nonce || 0,
            gasPrice: (transaction.gasPrice && typeof transaction.gasPrice === 'object' && 'hex' in transaction.gasPrice) 
              ? (transaction.gasPrice as any).hex 
              : ethers.utils.hexlify(transaction.gasPrice || 0),
            gasLimit: (transaction.gasLimit && typeof transaction.gasLimit === 'object' && 'hex' in transaction.gasLimit) 
              ? (transaction.gasLimit as any).hex 
              : ethers.utils.hexlify(transaction.gasLimit || 0),
            data: "0x",
            chainId: transaction.chainId || 1
          }
        }))).slice(2, 18) // First 8 bytes as checksum
      };
      
      console.log('\n🎉 SUCCESS!');
      console.log('============');
      console.log(`Signed Raw Transaction: ${signedTx}`);
      console.log(`Transaction Hash: ${txHash}`);
      
      // Debug: Decode the signed transaction
      console.log('\n🔍 Transaction Decoding Debug:');
      try {
        const decoded = ethers.utils.parseTransaction(signedTx);
        console.log('✅ Transaction decoded successfully:');
        console.log('  Nonce:', decoded.nonce);
        console.log('  Gas Price:', decoded.gasPrice?.toString());
        console.log('  Gas Limit:', decoded.gasLimit?.toString());
        console.log('  To:', decoded.to);
        console.log('  Value:', decoded.value?.toString());
        console.log('  Chain ID:', decoded.chainId);
        console.log('  V:', decoded.v);
        console.log('  R:', decoded.r);
        console.log('  S:', decoded.s);
        
        // Recover sender address from signature
        if (decoded.from) {
          console.log('  From (recovered):', decoded.from);
        } else {
          console.log('  From (recovered): Unable to recover sender address');
        }
      } catch (error: any) {
        console.log('❌ Failed to decode transaction:', error.message);
      }
      
      console.log('\n📄 Final JSON Object:');
      console.log(JSON.stringify(finalJson, null, 2));
      
      // Generate QR code with minified JSON
      const minifiedJson = JSON.stringify(finalJson);
      console.log('\n📱 QR Code (Minified JSON):');
      qrcode.generate(minifiedJson, { small: true });
      
    } finally {
      // Always close the transport
      await transport.close();
      console.log('\n🔌 Disconnected from Ledger device');
    }
    
  } catch (error: any) {
    console.error('\n❌ Error:', error.message);
    process.exit(1);
  }
}

// Handle uncaught exceptions
process.on('uncaughtException', (error: Error) => {
  console.error('\n❌ Uncaught Exception:', error.message);
  process.exit(1);
});

process.on('unhandledRejection', (reason: any, promise: Promise<any>) => {
  console.error('\n❌ Unhandled Rejection at:', promise, 'reason:', reason);
  process.exit(1);
});

// Run the script
if (require.main === module) {
  main();
}

export { 
  main, 
  parseArgs, 
  validateArgs, 
  getLedgerDevice, 
  connectToLedger, 
  getAddress, 
  buildTransaction, 
  serializeTransaction, 
  signTransaction, 
  createSignedTransaction,
  type TransactionOptions,
  type ParsedOptions,
  type LedgerDevice,
  type EthAddress,
  type EthSignature
};
