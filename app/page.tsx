"use client"

import { useState, useEffect } from "react"
import { BLEConnection } from "@/components/ble-connection"
import { TemperatureMonitor } from "@/components/temperature-monitor"
import { DeviceControls } from "@/components/device-controls"
import { ScheduleManager } from "@/components/schedule-manager"
import type { AquariumBLE, DeviceStatus } from "@/lib/bluetooth"
import { Waves } from "lucide-react"

export default function AquariumControlApp() {
  const [ble, setBle] = useState<AquariumBLE | null>(null)
  const [status, setStatus] = useState<Partial<DeviceStatus>>({
    temperature: 0,
    lightOn: false,
    pumpOn: false,
    heaterOn: false,
    feederOn: false,
    servoPosition: 90,
    quietMode: false,
  })

  useEffect(() => {
    if (ble) {
      ble.onStatusUpdate((newStatus) => {
        setStatus((prev) => ({ ...prev, ...newStatus }))
      })
    }
  }, [ble])

  return (
    <div className="min-h-screen bg-background">
      <div className="container mx-auto px-4 py-8 max-w-7xl">
        {/* Header */}
        <div className="mb-8">
          <div className="flex items-center gap-3 mb-2">
            <div className="p-3 bg-primary/10 rounded-xl">
              <Waves className="h-8 w-8 text-primary" />
            </div>
            <div>
              <h1 className="text-3xl font-bold text-foreground">Aquarium Control</h1>
              <p className="text-muted-foreground">Advanced monitoring and automation</p>
            </div>
          </div>
        </div>

        {/* Connection */}
        <div className="mb-6">
          <BLEConnection onConnectionChange={setBle} />
        </div>

        {/* Main Grid */}
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          {/* Left Column - Temperature & Controls */}
          <div className="lg:col-span-2 space-y-6">
            <TemperatureMonitor currentTemp={status.temperature ?? 0} />
            <DeviceControls
              ble={ble}
              lightOn={status.lightOn ?? false}
              pumpOn={status.pumpOn ?? false}
              heaterOn={status.heaterOn ?? false}
              feederOn={status.feederOn ?? false}
              servoPosition={status.servoPosition ?? 90}
              quietMode={status.quietMode ?? false}
            />
          </div>

          {/* Right Column - Schedule */}
          <div>
            <ScheduleManager ble={ble} />
          </div>
        </div>

        {/* Footer Info */}
        <div className="mt-8 text-center text-sm text-muted-foreground">
          <p>Current Time: {new Date().toLocaleString("pl-PL")}</p>
        </div>
      </div>
    </div>
  )
}
