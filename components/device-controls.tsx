"use client"

import { Card } from "@/components/ui/card"
import { Button } from "@/components/ui/button"
import { Switch } from "@/components/ui/switch"
import { Slider } from "@/components/ui/slider"
import { Label } from "@/components/ui/label"
import { Lightbulb, Waves, Flame, Fish, Wind, Volume2, VolumeX } from "lucide-react"
import type { AquariumBLE } from "@/lib/bluetooth"
import { useState } from "react"

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

  const handleToggle = async (device: string, value: boolean) => {
    if (!ble) return

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
          await ble.setQuietMode(value)
          break
      }
    } catch (error) {
      console.error("[v0] Control error:", error)
    }
  }

  const handleServoChange = async (value: number[]) => {
    setLocalServo(value[0])
  }

  const handleServoCommit = async () => {
    if (!ble) return
    try {
      await ble.setServo(localServo)
    } catch (error) {
      console.error("[v0] Servo error:", error)
    }
  }

  const handleFeedNow = async () => {
    if (!ble) return
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
    color,
  }: {
    icon: any
    label: string
    checked: boolean
    onCheckedChange: (checked: boolean) => void
    color: string
  }) => (
    <div className="flex items-center justify-between p-4 bg-secondary/50 rounded-lg">
      <div className="flex items-center gap-3">
        <div className={`p-2 rounded-lg ${checked ? color : "bg-muted"}`}>
          <Icon className={`h-5 w-5 ${checked ? "text-white" : "text-muted-foreground"}`} />
        </div>
        <Label htmlFor={label} className="text-foreground font-medium cursor-pointer">
          {label}
        </Label>
      </div>
      <Switch id={label} checked={checked} onCheckedChange={onCheckedChange} disabled={!ble} />
    </div>
  )

  return (
    <div className="space-y-4">
      <Card className="p-6 bg-card border-border">
        <h3 className="font-semibold text-foreground mb-4">Device Controls</h3>
        <div className="space-y-3">
          <ControlItem
            icon={Lightbulb}
            label="Light"
            checked={lightOn}
            onCheckedChange={(v) => handleToggle("light", v)}
            color="bg-yellow-500"
          />
          <ControlItem
            icon={Waves}
            label="Filter/Pump"
            checked={pumpOn}
            onCheckedChange={(v) => handleToggle("pump", v)}
            color="bg-blue-500"
          />
          <ControlItem
            icon={Flame}
            label="Heater"
            checked={heaterOn}
            onCheckedChange={(v) => handleToggle("heater", v)}
            color="bg-red-500"
          />
          <ControlItem
            icon={Fish}
            label="Feeder"
            checked={feederOn}
            onCheckedChange={(v) => handleToggle("feeder", v)}
            color="bg-green-500"
          />
        </div>
      </Card>

      <Card className="p-6 bg-card border-border">
        <div className="flex items-center justify-between mb-4">
          <div className="flex items-center gap-3">
            <div className="p-2 bg-primary/10 rounded-lg">
              <Wind className="h-5 w-5 text-primary" />
            </div>
            <div>
              <h3 className="font-semibold text-foreground">Aeration (Servo)</h3>
              <p className="text-sm text-muted-foreground">Position: {localServo}°</p>
            </div>
          </div>
        </div>
        <Slider
          value={[localServo]}
          onValueChange={handleServoChange}
          onValueCommit={handleServoCommit}
          min={0}
          max={180}
          step={1}
          disabled={!ble}
          className="mb-2"
        />
        <div className="flex justify-between text-xs text-muted-foreground">
          <span>0°</span>
          <span>90°</span>
          <span>180°</span>
        </div>
      </Card>

      <Card className="p-6 bg-card border-border">
        <div className="space-y-4">
          <div className="flex items-center justify-between p-4 bg-secondary/50 rounded-lg">
            <div className="flex items-center gap-3">
              <div className={`p-2 rounded-lg ${quietMode ? "bg-purple-500" : "bg-muted"}`}>
                {quietMode ? (
                  <VolumeX className="h-5 w-5 text-white" />
                ) : (
                  <Volume2 className="h-5 w-5 text-muted-foreground" />
                )}
              </div>
              <Label htmlFor="quiet" className="text-foreground font-medium cursor-pointer">
                Quiet Mode
              </Label>
            </div>
            <Switch id="quiet" checked={quietMode} onCheckedChange={(v) => handleToggle("quiet", v)} disabled={!ble} />
          </div>

          <Button
            onClick={handleFeedNow}
            disabled={!ble}
            className="w-full bg-green-600 hover:bg-green-700 text-white"
            size="lg"
          >
            <Fish className="mr-2 h-5 w-5" />
            Feed Now
          </Button>
        </div>
      </Card>
    </div>
  )
}
