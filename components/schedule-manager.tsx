"use client"

import { Card } from "@/components/ui/card"
import { Button } from "@/components/ui/button"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"
import { Switch } from "@/components/ui/switch"
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs"
import { Clock, Plus, Trash2, Lightbulb, Waves } from "lucide-react"
import type { AquariumBLE, ScheduleEntry } from "@/lib/bluetooth"
import { useState } from "react"

interface ScheduleManagerProps {
  ble: AquariumBLE | null
}

export function ScheduleManager({ ble }: ScheduleManagerProps) {
  const [lightSchedules, setLightSchedules] = useState<ScheduleEntry[]>([])
  const [filterSchedules, setFilterSchedules] = useState<ScheduleEntry[]>([])

  const [newLightSchedule, setNewLightSchedule] = useState<{ hour: number; minute: number; on: boolean }>({
    hour: 8,
    minute: 0,
    on: true,
  })

  const [newFilterSchedule, setNewFilterSchedule] = useState<{ hour: number; minute: number; on: boolean }>({
    hour: 8,
    minute: 0,
    on: true,
  })

  const handleAddLightSchedule = async () => {
    if (!ble) return

    const entry: ScheduleEntry = {
      id: lightSchedules.length,
      hour: newLightSchedule.hour,
      minute: newLightSchedule.minute,
      lightOn: newLightSchedule.on,
      pumpOn: false,
      heaterOn: false,
    }

    try {
      await ble.setSchedule(entry)
      setLightSchedules([...lightSchedules, entry])
    } catch (error) {
      console.error("[v0] Light schedule add error:", error)
    }
  }

  const handleAddFilterSchedule = async () => {
    if (!ble) return

    const entry: ScheduleEntry = {
      id: filterSchedules.length + 100, // Offset to avoid ID conflicts
      hour: newFilterSchedule.hour,
      minute: newFilterSchedule.minute,
      lightOn: false,
      pumpOn: newFilterSchedule.on,
      heaterOn: false,
    }

    try {
      await ble.setSchedule(entry)
      setFilterSchedules([...filterSchedules, entry])
    } catch (error) {
      console.error("[v0] Filter schedule add error:", error)
    }
  }

  const handleDeleteLightSchedule = async (id: number) => {
    if (!ble) return
    try {
      await ble.deleteSchedule(id)
      setLightSchedules(lightSchedules.filter((s) => s.id !== id))
    } catch (error) {
      console.error("[v0] Light schedule delete error:", error)
    }
  }

  const handleDeleteFilterSchedule = async (id: number) => {
    if (!ble) return
    try {
      await ble.deleteSchedule(id)
      setFilterSchedules(filterSchedules.filter((s) => s.id !== id))
    } catch (error) {
      console.error("[v0] Filter schedule delete error:", error)
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
            <h3 className="font-semibold text-foreground">Harmonogram</h3>
            <p className="text-sm text-muted-foreground">Automatyzacja akwarium</p>
          </div>
        </div>
        <Button
          onClick={handleSyncTime}
          disabled={!ble}
          variant="outline"
          size="sm"
          className="border-primary text-primary hover:bg-primary hover:text-primary-foreground bg-transparent"
        >
          Synchronizuj
        </Button>
      </div>

      <Tabs defaultValue="light" className="w-full">
        <TabsList className="grid w-full grid-cols-2 mb-4">
          <TabsTrigger value="light" className="flex items-center gap-2">
            <Lightbulb className="h-4 w-4" />
            Światło
          </TabsTrigger>
          <TabsTrigger value="filter" className="flex items-center gap-2">
            <Waves className="h-4 w-4" />
            Filtr
          </TabsTrigger>
        </TabsList>

        <TabsContent value="light" className="space-y-4">
          <div className="p-4 bg-secondary/50 rounded-lg space-y-4">
            <div className="grid grid-cols-2 gap-4">
              <div>
                <Label htmlFor="light-hour" className="text-foreground">
                  Godzina
                </Label>
                <Input
                  id="light-hour"
                  type="number"
                  min={0}
                  max={23}
                  value={newLightSchedule.hour}
                  onChange={(e) => setNewLightSchedule({ ...newLightSchedule, hour: Number.parseInt(e.target.value) })}
                  className="bg-background border-border text-foreground"
                />
              </div>
              <div>
                <Label htmlFor="light-minute" className="text-foreground">
                  Minuta
                </Label>
                <Input
                  id="light-minute"
                  type="number"
                  min={0}
                  max={59}
                  value={newLightSchedule.minute}
                  onChange={(e) =>
                    setNewLightSchedule({ ...newLightSchedule, minute: Number.parseInt(e.target.value) })
                  }
                  className="bg-background border-border text-foreground"
                />
              </div>
            </div>

            <div className="flex items-center justify-between">
              <Label htmlFor="light-on" className="text-foreground">
                Włącz światło
              </Label>
              <Switch
                id="light-on"
                checked={newLightSchedule.on}
                onCheckedChange={(v) => setNewLightSchedule({ ...newLightSchedule, on: v })}
              />
            </div>

            <Button
              onClick={handleAddLightSchedule}
              disabled={!ble}
              className="w-full bg-primary text-primary-foreground hover:bg-primary/90"
            >
              <Plus className="mr-2 h-4 w-4" />
              Dodaj Harmonogram
            </Button>
          </div>

          {lightSchedules.length > 0 && (
            <div className="space-y-2">
              <h4 className="text-sm font-medium text-foreground">Aktywne Harmonogramy</h4>
              {lightSchedules.map((schedule) => (
                <div key={schedule.id} className="flex items-center justify-between p-3 bg-secondary/50 rounded-lg">
                  <div>
                    <p className="font-medium text-foreground">
                      {schedule.hour.toString().padStart(2, "0")}:{schedule.minute.toString().padStart(2, "0")}
                    </p>
                    <p className="text-xs text-muted-foreground">{schedule.lightOn ? "Włącz" : "Wyłącz"}</p>
                  </div>
                  <Button
                    onClick={() => handleDeleteLightSchedule(schedule.id)}
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
        </TabsContent>

        <TabsContent value="filter" className="space-y-4">
          <div className="p-4 bg-secondary/50 rounded-lg space-y-4">
            <div className="grid grid-cols-2 gap-4">
              <div>
                <Label htmlFor="filter-hour" className="text-foreground">
                  Godzina
                </Label>
                <Input
                  id="filter-hour"
                  type="number"
                  min={0}
                  max={23}
                  value={newFilterSchedule.hour}
                  onChange={(e) =>
                    setNewFilterSchedule({ ...newFilterSchedule, hour: Number.parseInt(e.target.value) })
                  }
                  className="bg-background border-border text-foreground"
                />
              </div>
              <div>
                <Label htmlFor="filter-minute" className="text-foreground">
                  Minuta
                </Label>
                <Input
                  id="filter-minute"
                  type="number"
                  min={0}
                  max={59}
                  value={newFilterSchedule.minute}
                  onChange={(e) =>
                    setNewFilterSchedule({ ...newFilterSchedule, minute: Number.parseInt(e.target.value) })
                  }
                  className="bg-background border-border text-foreground"
                />
              </div>
            </div>

            <div className="flex items-center justify-between">
              <Label htmlFor="filter-on" className="text-foreground">
                Włącz filtr
              </Label>
              <Switch
                id="filter-on"
                checked={newFilterSchedule.on}
                onCheckedChange={(v) => setNewFilterSchedule({ ...newFilterSchedule, on: v })}
              />
            </div>

            <Button
              onClick={handleAddFilterSchedule}
              disabled={!ble}
              className="w-full bg-primary text-primary-foreground hover:bg-primary/90"
            >
              <Plus className="mr-2 h-4 w-4" />
              Dodaj Harmonogram
            </Button>
          </div>

          {filterSchedules.length > 0 && (
            <div className="space-y-2">
              <h4 className="text-sm font-medium text-foreground">Aktywne Harmonogramy</h4>
              {filterSchedules.map((schedule) => (
                <div key={schedule.id} className="flex items-center justify-between p-3 bg-secondary/50 rounded-lg">
                  <div>
                    <p className="font-medium text-foreground">
                      {schedule.hour.toString().padStart(2, "0")}:{schedule.minute.toString().padStart(2, "0")}
                    </p>
                    <p className="text-xs text-muted-foreground">{schedule.pumpOn ? "Włącz" : "Wyłącz"}</p>
                  </div>
                  <Button
                    onClick={() => handleDeleteFilterSchedule(schedule.id)}
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
        </TabsContent>
      </Tabs>
    </Card>
  )
}
