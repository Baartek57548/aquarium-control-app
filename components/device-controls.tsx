"use client"

import { Button } from "@/components/ui/button"
import { Switch } from "@/components/ui/switch"
import { Slider } from "@/components/ui/slider"
import { Label } from "@/components/ui/label"
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select"
import { Lightbulb, Waves, Flame, Fish, Wind, Volume2, VolumeX } from "lucide-react"
import type { AquariumBLE } from "@/lib/bluetooth"
import { useState } from "react"
import { FeedAnimation, QuietModeAnimation, DeviceToggleAnimation, BubbleAnimation } from "./animations"

interface DeviceControlsProps {
  ble: AquariumBLE | null
  lightOn: boolean
  pumpOn: boolean
  heaterOn: boolean
  feederOn: boolean
  servoPosition: number
  quietMode: boolean
}

export function DeviceControls({
  ble,
  lightOn,
  pumpOn,
  heaterOn,
  feederOn,
  servoPosition,
  quietMode,
}: DeviceControlsProps) {
  const [localServo, setLocalServo] = useState(servoPosition)
  const [quietDuration, setQuietDuration] = useState<string>("30")

  const [showFeedAnimation, setShowFeedAnimation] = useState(false)
  const [showQuietAnimation, setShowQuietAnimation] = useState(false)
  const [showDeviceAnimation, setShowDeviceAnimation] = useState<"yellow" | "blue" | "red" | null>(null)
  const [showBubbleAnimation, setShowBubbleAnimation] = useState(false)

  const handleToggle = async (device: string, value: boolean) => {
    if (!ble) return

    if (device === "light" && value) {
      setShowDeviceAnimation("yellow")
    } else if (device === "pump" && value) {
      setShowDeviceAnimation("blue")
    } else if (device === "heater" && value) {
      setShowDeviceAnimation("red")
    } else if (device === "quiet" && value) {
      setShowQuietAnimation(true)
    }

    try {
      switch (device) {
        case "light":
          await ble.setLight(value)
          break
        case "pump":
          await ble.setPump(value)
          break
        case "heater":
          await ble.setHeater(value)
          break
        case "feeder":
          await ble.setFeeder(value)
          break
        case "quiet":
          await ble.setQuietMode(value, value ? Number.parseInt(quietDuration) : 0)
          if (!value) setShowQuietAnimation(false)
          break
      }
    } catch (error) {
      console.error("[v0] Control error:", error)
    }
  }

  const handleServoChange = async (value: number[]) => {
    setLocalServo(value[0])
    setShowBubbleAnimation(true)
  }

  const handleServoCommit = async () => {
    if (!ble) return
    try {
      await ble.setServo(localServo)
      setTimeout(() => setShowBubbleAnimation(false), 3000)
    } catch (error) {
      console.error("[v0] Servo error:", error)
    }
  }

  const handleFeedNow = async () => {
    if (!ble) return
    setShowFeedAnimation(true)
    try {
      await ble.feedNow()
    } catch (error) {
      console.error("[v0] Feed error:", error)
    }
  }

  const ControlItem = ({
    icon: Icon,
    label,
    checked,
    onCheckedChange,
    gradient,
  }: {
    icon: any
    label: string
    checked: boolean
    onCheckedChange: (checked: boolean) => void
    gradient: string
  }) => (
    <div
      className={`flex items-center justify-between p-5 rounded-xl transition-all ${
        checked ? "glass-card-light" : "bg-gray-900/30"
      } border ${checked ? "border-white/10" : "border-gray-800"}`}
    >
      <div className="flex items-center gap-4">
        <div className={`p-3 rounded-xl transition-all ${checked ? gradient : "bg-gray-800"}`}>
          <Icon className={`h-6 w-6 ${checked ? "text-white" : "text-gray-500"}`} />
        </div>
        <Label
          htmlFor={label}
          className={`font-semibold cursor-pointer text-base ${checked ? "text-white" : "text-gray-400"}`}
        >
          {label}
        </Label>
      </div>
      <Switch id={label} checked={checked} onCheckedChange={onCheckedChange} disabled={!ble} className="scale-110" />
    </div>
  )

  return (
    <div className="space-y-4">
      <FeedAnimation show={showFeedAnimation} onComplete={() => setShowFeedAnimation(false)} />
      <QuietModeAnimation show={showQuietAnimation} />
      <DeviceToggleAnimation
        show={showDeviceAnimation !== null}
        color={showDeviceAnimation || "yellow"}
        onComplete={() => setShowDeviceAnimation(null)}
      />
      <BubbleAnimation show={showBubbleAnimation} />

      <div className="glass-card rounded-2xl p-6">
        <h3 className="font-bold text-white mb-4 text-lg">Szybkie Akcje</h3>
        <div className="space-y-3">
          <Button
            onClick={handleFeedNow}
            disabled={!ble}
            className="w-full bg-gradient-to-r from-green-500 to-emerald-600 hover:from-green-600 hover:to-emerald-700 text-white shadow-lg shadow-green-500/30 h-14 text-base font-semibold"
          >
            <Fish className="mr-2 h-6 w-6" />
            Nakarm Rybki Teraz
          </Button>

          <div
            className={`p-5 rounded-xl transition-all ${
              quietMode ? "glass-card-light border-purple-500/30" : "bg-gray-900/30 border-gray-800"
            } border`}
          >
            <div className="flex items-center justify-between mb-3">
              <div className="flex items-center gap-4">
                <div
                  className={`p-3 rounded-xl transition-all ${
                    quietMode
                      ? "bg-gradient-to-br from-purple-500 to-violet-600 shadow-lg shadow-purple-500/30"
                      : "bg-gray-800"
                  }`}
                >
                  {quietMode ? (
                    <VolumeX className="h-6 w-6 text-white" />
                  ) : (
                    <Volume2 className="h-6 w-6 text-gray-500" />
                  )}
                </div>
                <Label
                  htmlFor="quiet"
                  className={`font-semibold cursor-pointer text-base ${quietMode ? "text-white" : "text-gray-400"}`}
                >
                  Tryb Cichy
                </Label>
              </div>
              <Switch
                id="quiet"
                checked={quietMode}
                onCheckedChange={(v) => handleToggle("quiet", v)}
                disabled={!ble}
                className="scale-110"
              />
            </div>
            <div className="mt-3">
              <Label htmlFor="quiet-duration" className="text-sm text-gray-400 mb-2 block">
                Czas trwania
              </Label>
              <Select value={quietDuration} onValueChange={setQuietDuration} disabled={!ble || quietMode}>
                <SelectTrigger
                  id="quiet-duration"
                  className="bg-gray-900/50 border-gray-700 text-white hover:bg-gray-900/70"
                >
                  <SelectValue />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="30">30 minut</SelectItem>
                  <SelectItem value="60">1 godzina</SelectItem>
                  <SelectItem value="120">2 godziny</SelectItem>
                  <SelectItem value="240">4 godziny</SelectItem>
                </SelectContent>
              </Select>
            </div>
          </div>
        </div>
      </div>

      <div className="glass-card rounded-2xl p-6">
        <h3 className="font-bold text-white mb-5 text-lg">Sterowanie Urządzeniami</h3>
        <div className="space-y-3">
          <ControlItem
            icon={Lightbulb}
            label="Oświetlenie"
            checked={lightOn}
            onCheckedChange={(v) => handleToggle("light", v)}
            gradient="bg-gradient-to-br from-yellow-500 to-orange-500 shadow-lg shadow-yellow-500/30"
          />
          <ControlItem
            icon={Waves}
            label="Filtr i Pompa"
            checked={pumpOn}
            onCheckedChange={(v) => handleToggle("pump", v)}
            gradient="bg-gradient-to-br from-blue-500 to-cyan-500 shadow-lg shadow-blue-500/30"
          />
          <ControlItem
            icon={Flame}
            label="Grzałka"
            checked={heaterOn}
            onCheckedChange={(v) => handleToggle("heater", v)}
            gradient="bg-gradient-to-br from-red-500 to-pink-500 shadow-lg shadow-red-500/30"
          />
        </div>
      </div>

      <div className="glass-card rounded-2xl p-6">
        <div className="flex items-center justify-between mb-5">
          <div className="flex items-center gap-3">
            <div className="p-3 bg-gradient-to-br from-cyan-500 to-blue-600 rounded-xl shadow-lg shadow-cyan-500/30">
              <Wind className="h-6 w-6 text-white" />
            </div>
            <div>
              <h3 className="font-bold text-white">Napowietrzanie</h3>
              <p className="text-sm text-cyan-300">Pozycja Servo: {localServo}°</p>
            </div>
          </div>
        </div>
        <div className="bg-gray-900/50 rounded-xl p-4">
          <Slider
            value={[localServo]}
            onValueChange={handleServoChange}
            onValueCommit={handleServoCommit}
            min={0}
            max={180}
            step={1}
            disabled={!ble}
            className="mb-3"
          />
          <div className="flex justify-between text-xs text-gray-400 font-medium">
            <span>Min (0°)</span>
            <span>Środek (90°)</span>
            <span>Max (180°)</span>
          </div>
        </div>
      </div>
    </div>
  )
}
