"use client"

import { useState, useEffect } from "react"
import { BLEConnection } from "@/components/ble-connection"
import { TemperatureMonitor } from "@/components/temperature-monitor"
import { DeviceControls } from "@/components/device-controls"
import { ScheduleManager } from "@/components/schedule-manager"
import type { AquariumBLE, DeviceStatus } from "@/lib/bluetooth"
import { Waves, Droplets } from "lucide-react"

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
    <>
      <div className="aquarium-bg" />

      <div className="min-h-screen relative">
        <div className="container mx-auto px-4 py-6 max-w-7xl">
          <div className="mb-6">
            <div className="glass-card rounded-2xl p-6">
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-4">
                  <div className="p-4 bg-gradient-to-br from-cyan-500 to-blue-600 rounded-2xl shadow-lg">
                    <Waves className="h-8 w-8 text-white" />
                  </div>
                  <div>
                    <h1 className="text-3xl font-bold text-white">Sterownik Akwarium</h1>
                    <p className="text-cyan-300 flex items-center gap-2 mt-1">
                      <Droplets className="h-4 w-4" />
                      Bartosz Wolny
                    </p>
                  </div>
                </div>
                <div className="hidden md:block text-right">
                  <p className="text-sm text-gray-400">Aktualny czas</p>
                  <p className="text-white font-semibold">
                    {new Date().toLocaleDateString("pl-PL")} {new Date().toLocaleTimeString("pl-PL")}
                  </p>
                </div>
              </div>
            </div>
          </div>

          <div className="mb-6">
            <BLEConnection onConnectionChange={setBle} />
          </div>

          <div className="grid grid-cols-1 xl:grid-cols-3 gap-6">
            <div className="xl:col-span-2">
              <TemperatureMonitor currentTemp={status.temperature ?? 0} />
            </div>

            <div className="glass-card rounded-2xl p-6">
              <h3 className="font-semibold text-white mb-4 text-lg">Status</h3>
              <div className="space-y-3">
                <StatusBadge label="Światło" active={status.lightOn} color="yellow" />
                <StatusBadge label="Filtr" active={status.pumpOn} color="blue" />
                <StatusBadge label="Grzałka" active={status.heaterOn} color="red" />
                <StatusBadge label="Tryb Cichy" active={status.quietMode} color="purple" />
              </div>
            </div>
          </div>

          <div className="mt-6 grid grid-cols-1 xl:grid-cols-3 gap-6">
            <div className="xl:col-span-2">
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

            <div>
              <ScheduleManager ble={ble} />
            </div>
          </div>
        </div>
      </div>
    </>
  )
}

function StatusBadge({ label, active, color }: { label: string; active?: boolean; color: string }) {
  const colors = {
    yellow: active
      ? "bg-yellow-500/20 border-yellow-500/50 text-yellow-300"
      : "bg-gray-800/50 border-gray-700 text-gray-500",
    blue: active ? "bg-blue-500/20 border-blue-500/50 text-blue-300" : "bg-gray-800/50 border-gray-700 text-gray-500",
    red: active ? "bg-red-500/20 border-red-500/50 text-red-300" : "bg-gray-800/50 border-gray-700 text-gray-500",
    purple: active
      ? "bg-purple-500/20 border-purple-500/50 text-purple-300"
      : "bg-gray-800/50 border-gray-700 text-gray-500",
  }

  return (
    <div
      className={`flex items-center justify-between px-4 py-3 rounded-xl border ${colors[color as keyof typeof colors]}`}
    >
      <span className="font-medium">{label}</span>
      <div className={`w-3 h-3 rounded-full ${active ? "bg-current animate-pulse" : "bg-gray-600"}`} />
    </div>
  )
}
