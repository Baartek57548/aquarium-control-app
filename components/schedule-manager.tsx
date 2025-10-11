"use client"

import { Card } from "@/components/ui/card"
import { Button } from "@/components/ui/button"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"
import { Switch } from "@/components/ui/switch"
import { Clock, Plus, Trash2 } from "lucide-react"
import type { AquariumBLE, ScheduleEntry } from "@/lib/bluetooth"
import { useState } from "react"

interface ScheduleManagerProps {
  ble: AquariumBLE | null
}

export function ScheduleManager({ ble }: ScheduleManagerProps) {
  const [schedules, setSchedules] = useState<ScheduleEntry[]>([])
  const [newSchedule, setNewSchedule] = useState<Partial<ScheduleEntry>>({
    hour: 8,
    minute: 0,
    lightOn: true,
    pumpOn: true,
    heaterOn: true,
  })

  const handleAddSchedule = async () => {
    if (!ble || !newSchedule.hour || newSchedule.minute === undefined) return

    const entry: ScheduleEntry = {
      id: schedules.length,
      hour: newSchedule.hour,
      minute: newSchedule.minute,
      lightOn: newSchedule.lightOn ?? false,
      pumpOn: newSchedule.pumpOn ?? false,
      heaterOn: newSchedule.heaterOn ?? false,
    }

    try {
      await ble.setSchedule(entry)
      setSchedules([...schedules, entry])
    } catch (error) {
      console.error("[v0] Schedule add error:", error)
    }
  }

  const handleDeleteSchedule = async (id: number) => {
    if (!ble) return

    try {
      await ble.deleteSchedule(id)
      setSchedules(schedules.filter((s) => s.id !== id))
    } catch (error) {
      console.error("[v0] Schedule delete error:", error)
    }
  }

  const handleSyncTime = async () => {
    if (!ble) return
    try {
      await ble.syncTime()
    } catch (error) {
      console.error("[v0] Time sync error:", error)
    }
  }

  return (
    <Card className="p-6 bg-card border-border">
      <div className="flex items-center justify-between mb-6">
        <div className="flex items-center gap-3">
          <div className="p-2 bg-primary/10 rounded-lg">
            <Clock className="h-6 w-6 text-primary" />
          </div>
          <div>
            <h3 className="font-semibold text-foreground">Schedule Manager</h3>
            <p className="text-sm text-muted-foreground">Automate your aquarium</p>
          </div>
        </div>
        <Button
          onClick={handleSyncTime}
          disabled={!ble}
          variant="outline"
          size="sm"
          className="border-primary text-primary hover:bg-primary hover:text-primary-foreground bg-transparent"
        >
          Sync Time
        </Button>
      </div>

      <div className="space-y-4">
        <div className="p-4 bg-secondary/50 rounded-lg space-y-4">
          <div className="grid grid-cols-2 gap-4">
            <div>
              <Label htmlFor="hour" className="text-foreground">
                Hour
              </Label>
              <Input
                id="hour"
                type="number"
                min={0}
                max={23}
                value={newSchedule.hour}
                onChange={(e) => setNewSchedule({ ...newSchedule, hour: Number.parseInt(e.target.value) })}
                className="bg-background border-border text-foreground"
              />
            </div>
            <div>
              <Label htmlFor="minute" className="text-foreground">
                Minute
              </Label>
              <Input
                id="minute"
                type="number"
                min={0}
                max={59}
                value={newSchedule.minute}
                onChange={(e) => setNewSchedule({ ...newSchedule, minute: Number.parseInt(e.target.value) })}
                className="bg-background border-border text-foreground"
              />
            </div>
          </div>

          <div className="space-y-2">
            <div className="flex items-center justify-between">
              <Label htmlFor="sched-light" className="text-foreground">
                Light
              </Label>
              <Switch
                id="sched-light"
                checked={newSchedule.lightOn}
                onCheckedChange={(v) => setNewSchedule({ ...newSchedule, lightOn: v })}
              />
            </div>
            <div className="flex items-center justify-between">
              <Label htmlFor="sched-pump" className="text-foreground">
                Pump
              </Label>
              <Switch
                id="sched-pump"
                checked={newSchedule.pumpOn}
                onCheckedChange={(v) => setNewSchedule({ ...newSchedule, pumpOn: v })}
              />
            </div>
            <div className="flex items-center justify-between">
              <Label htmlFor="sched-heater" className="text-foreground">
                Heater
              </Label>
              <Switch
                id="sched-heater"
                checked={newSchedule.heaterOn}
                onCheckedChange={(v) => setNewSchedule({ ...newSchedule, heaterOn: v })}
              />
            </div>
          </div>

          <Button
            onClick={handleAddSchedule}
            disabled={!ble}
            className="w-full bg-primary text-primary-foreground hover:bg-primary/90"
          >
            <Plus className="mr-2 h-4 w-4" />
            Add Schedule
          </Button>
        </div>

        {schedules.length > 0 && (
          <div className="space-y-2">
            <h4 className="text-sm font-medium text-foreground">Active Schedules</h4>
            {schedules.map((schedule) => (
              <div key={schedule.id} className="flex items-center justify-between p-3 bg-secondary/50 rounded-lg">
                <div>
                  <p className="font-medium text-foreground">
                    {schedule.hour.toString().padStart(2, "0")}:{schedule.minute.toString().padStart(2, "0")}
                  </p>
                  <p className="text-xs text-muted-foreground">
                    {[schedule.lightOn && "Light", schedule.pumpOn && "Pump", schedule.heaterOn && "Heater"]
                      .filter(Boolean)
                      .join(", ")}
                  </p>
                </div>
                <Button
                  onClick={() => handleDeleteSchedule(schedule.id)}
                  variant="ghost"
                  size="sm"
                  className="text-destructive hover:text-destructive hover:bg-destructive/10"
                >
                  <Trash2 className="h-4 w-4" />
                </Button>
              </div>
            ))}
          </div>
        )}
      </div>
    </Card>
  )
}
