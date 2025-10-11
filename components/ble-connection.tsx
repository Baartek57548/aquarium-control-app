"use client"

import { useState } from "react"
import { Button } from "@/components/ui/button"
import { Bluetooth, BluetoothConnected, BluetoothOff, Wifi } from "lucide-react"
import { AquariumBLE } from "@/lib/bluetooth"

interface BLEConnectionProps {
  onConnectionChange: (ble: AquariumBLE | null) => void
}

export function BLEConnection({ onConnectionChange }: BLEConnectionProps) {
  const [isConnected, setIsConnected] = useState(false)
  const [isConnecting, setIsConnecting] = useState(false)
  const [error, setError] = useState<string | null>(null)
  const [ble] = useState(() => new AquariumBLE())

  const handleConnect = async () => {
    setIsConnecting(true)
    setError(null)

    try {
      await ble.connect()
      setIsConnected(true)
      onConnectionChange(ble)

      await ble.syncTime()
      await ble.requestStatus()
    } catch (err) {
      setError(err instanceof Error ? err.message : "Błąd połączenia")
      setIsConnected(false)
      onConnectionChange(null)
    } finally {
      setIsConnecting(false)
    }
  }

  const handleDisconnect = async () => {
    await ble.disconnect()
    setIsConnected(false)
    onConnectionChange(null)
  }

  return (
    <div className="glass-card rounded-2xl p-6">
      <div className="flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4">
        <div className="flex items-center gap-4">
          <div
            className={`p-4 rounded-2xl transition-all ${
              isConnected
                ? "bg-gradient-to-br from-green-500 to-emerald-600 shadow-lg shadow-green-500/50"
                : "bg-gray-800/50"
            }`}
          >
            {isConnected ? (
              <BluetoothConnected className="h-7 w-7 text-white" />
            ) : (
              <BluetoothOff className="h-7 w-7 text-gray-400" />
            )}
          </div>
          <div>
            <div className="flex items-center gap-2">
              <h3 className="font-bold text-white text-lg">{isConnected ? "Połączono" : "Rozłączono"}</h3>
              {isConnected && (
                <div className="flex items-center gap-1 px-2 py-1 bg-green-500/20 rounded-full">
                  <Wifi className="h-3 w-3 text-green-400" />
                  <span className="text-xs text-green-400 font-medium">Na żywo</span>
                </div>
              )}
            </div>
            <p className="text-sm text-gray-400 mt-1">
              {isConnected ? "Kontroler Akwarium" : "Dotknij aby połączyć z urządzeniem"}
            </p>
          </div>
        </div>

        {isConnected ? (
          <Button
            onClick={handleDisconnect}
            variant="outline"
            size="lg"
            className="border-red-500/50 text-red-400 hover:bg-red-500/20 hover:text-red-300 bg-transparent hover:border-red-500"
          >
            Rozłącz
          </Button>
        ) : (
          <Button
            onClick={handleConnect}
            disabled={isConnecting}
            size="lg"
            className="bg-gradient-to-r from-cyan-500 to-blue-600 text-white hover:from-cyan-600 hover:to-blue-700 shadow-lg shadow-cyan-500/30"
          >
            <Bluetooth className="mr-2 h-5 w-5" />
            {isConnecting ? "Łączenie..." : "Połącz Urządzenie"}
          </Button>
        )}
      </div>

      {error && (
        <div className="mt-4 p-4 bg-red-500/10 border border-red-500/30 rounded-xl">
          <p className="text-sm text-red-400">{error}</p>
        </div>
      )}
    </div>
  )
}
