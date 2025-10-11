"use client"

import { useState } from "react"
import { Button } from "@/components/ui/button"
import { Card } from "@/components/ui/card"
import { Bluetooth, BluetoothConnected, BluetoothOff } from "lucide-react"
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

      // Sync time on connection
      await ble.syncTime()
      await ble.requestStatus()
    } catch (err) {
      setError(err instanceof Error ? err.message : "Connection failed")
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
    <Card className="p-6 bg-card border-border">
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-3">
          {isConnected ? (
            <BluetoothConnected className="h-6 w-6 text-primary" />
          ) : (
            <BluetoothOff className="h-6 w-6 text-muted-foreground" />
          )}
          <div>
            <h3 className="font-semibold text-foreground">{isConnected ? "Connected" : "Not Connected"}</h3>
            <p className="text-sm text-muted-foreground">
              {isConnected ? "Aquarium Controller" : "Connect to your device"}
            </p>
          </div>
        </div>

        {isConnected ? (
          <Button
            onClick={handleDisconnect}
            variant="outline"
            className="border-destructive text-destructive hover:bg-destructive hover:text-destructive-foreground bg-transparent"
          >
            Disconnect
          </Button>
        ) : (
          <Button
            onClick={handleConnect}
            disabled={isConnecting}
            className="bg-primary text-primary-foreground hover:bg-primary/90"
          >
            <Bluetooth className="mr-2 h-4 w-4" />
            {isConnecting ? "Connecting..." : "Connect"}
          </Button>
        )}
      </div>

      {error && (
        <div className="mt-4 p-3 bg-destructive/10 border border-destructive/20 rounded-lg">
          <p className="text-sm text-destructive">{error}</p>
        </div>
      )}
    </Card>
  )
}
