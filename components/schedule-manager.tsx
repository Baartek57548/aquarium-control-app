"use client"

import { Card } from "@/components/ui/card"
import { Button } from "@/components/ui/button"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs"
import { Clock, Plus, Trash2, Lightbulb, Waves, Fish } from "lucide-react"
import type { AquariumBLE, ScheduleEntry } from "@/lib/bluetooth"
import { useState } from "react"

interface ScheduleManagerProps {
  ble: AquariumBLE | null
}

export function ScheduleManager({ ble }: ScheduleManagerProps) {
  const [lightSchedules, setLightSchedules] = useState<ScheduleEntry[]>([])
  const [filterSchedules, setFilterSchedules] = useState<ScheduleEntry[]>([])
  const [feederSchedules, setFeederSchedules] = useState<ScheduleEntry[]>([])

  const [newLightSchedule, setNewLightSchedule] = useState<{
    onHour: number
    onMinute: number
    offHour: number
    offMinute: number
  }>({
    onHour: 8,
    onMinute: 0,
    offHour: 20,
    offMinute: 0,
  })

  const [newFilterSchedule, setNewFilterSchedule] = useState<{
    onHour: number
    onMinute: number
    offHour: number
    offMinute: number
  }>({
    onHour: 8,
    onMinute: 0,
    offHour: 22,
    offMinute: 0,
  })

  const [newFeederSchedule, setNewFeederSchedule] = useState<{
    hour: number
    minute: number
  }>({
    hour: 9,
    minute: 0,
  })

  const handleAddLightSchedule = async () => {
    if (!ble) return

    const onEntry: ScheduleEntry = {
      id: lightSchedules.length,
      hour: newLightSchedule.onHour,
      minute: newLightSchedule.onMinute,
      lightOn: true,
      pumpOn: false,
      heaterOn: false,
    }

    const offEntry: ScheduleEntry = {
      id: lightSchedules.length + 1,
      hour: newLightSchedule.offHour,
      minute: newLightSchedule.offMinute,
      lightOn: false,
      pumpOn: false,
      heaterOn: false,
    }

    try {
      await ble.setSchedule(onEntry)
      await ble.setSchedule(offEntry)
      setLightSchedules([...lightSchedules, onEntry, offEntry])
    } catch (error) {
      console.error("[v0] Light schedule add error:", error)
    }
  }

  const handleAddFilterSchedule = async () => {
    if (!ble) return

    const onEntry: ScheduleEntry = {
      id: filterSchedules.length + 100,
      hour: newFilterSchedule.onHour,
      minute: newFilterSchedule.onMinute,
      lightOn: false,
      pumpOn: true,
      heaterOn: false,
    }

    const offEntry: ScheduleEntry = {
      id: filterSchedules.length + 101,
      hour: newFilterSchedule.offHour,
      minute: newFilterSchedule.offMinute,
      lightOn: false,
      pumpOn: false,
      heaterOn: false,
    }

    try {
      await ble.setSchedule(onEntry)
      await ble.setSchedule(offEntry)
      setFilterSchedules([...filterSchedules, onEntry, offEntry])
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

  const handleAddFeederSchedule = async () => {
    if (!ble) return

    const entry: ScheduleEntry = {
      id: feederSchedules.length + 200,
      hour: newFeederSchedule.hour,
      minute: newFeederSchedule.minute,
      lightOn: false,
      pumpOn: false,
      heaterOn: false,
    }

    try {
      await ble.setSchedule(entry)
      setFeederSchedules([...feederSchedules, entry])
    } catch (error) {
      console.error("[v0] Feeder schedule add error:", error)
    }
  }

  const handleDeleteFeederSchedule = async (id: number) => {
    if (!ble) return
    try {
      await ble.deleteSchedule(id)
      setFeederSchedules(feederSchedules.filter((s) => s.id !== id))
    } catch (error) {
      console.error("[v0] Feeder schedule delete error:", error)
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
        <TabsList className="grid w-full grid-cols-3 mb-4">
          <TabsTrigger value="light" className="flex items-center gap-2">
            <Lightbulb className="h-4 w-4" />
            Światło
          </TabsTrigger>
          <TabsTrigger value="filter" className="flex items-center gap-2">
            <Waves className="h-4 w-4" />
            Filtr
          </TabsTrigger>
          <TabsTrigger value="feeder" className="flex items-center gap-2">
            <Fish className="h-4 w-4" />
            Karmnik
          </TabsTrigger>
        </TabsList>

        <TabsContent value="light" className="space-y-4">
          <div className="p-4 bg-secondary/50 rounded-lg space-y-4">
            <div className="space-y-4">
              <div>
                <h4 className="text-sm font-semibold text-foreground mb-3">Włączenie</h4>
                <div className="grid grid-cols-2 gap-4">
                  <div>
                    <Label htmlFor="light-on-hour" className="text-foreground">
                      Godzina
                    </Label>
                    <Input
                      id="light-on-hour"
                      type="number"
                      min={0}
                      max={23}
                      value={newLightSchedule.onHour}
                      onChange={(e) =>
                        setNewLightSchedule({ ...newLightSchedule, onHour: Number.parseInt(e.target.value) })
                      }
                      className="bg-background border-border text-foreground"
                    />
                  </div>
                  <div>
                    <Label htmlFor="light-on-minute" className="text-foreground">
                      Minuta
                    </Label>
                    <Input
                      id="light-on-minute"
                      type="number"
                      min={0}
                      max={59}
                      value={newLightSchedule.onMinute}
                      onChange={(e) =>
                        setNewLightSchedule({ ...newLightSchedule, onMinute: Number.parseInt(e.target.value) })
                      }
                      className="bg-background border-border text-foreground"
                    />
                  </div>
                </div>
              </div>

              <div>
                <h4 className="text-sm font-semibold text-foreground mb-3">Wyłączenie</h4>
                <div className="grid grid-cols-2 gap-4">
                  <div>
                    <Label htmlFor="light-off-hour" className="text-foreground">
                      Godzina
                    </Label>
                    <Input
                      id="light-off-hour"
                      type="number"
                      min={0}
                      max={23}
                      value={newLightSchedule.offHour}
                      onChange={(e) =>
                        setNewLightSchedule({ ...newLightSchedule, offHour: Number.parseInt(e.target.value) })
                      }
                      className="bg-background border-border text-foreground"
                    />
                  </div>
                  <div>
                    <Label htmlFor="light-off-minute" className="text-foreground">
                      Minuta
                    </Label>
                    <Input
                      id="light-off-minute"
                      type="number"
                      min={0}
                      max={59}
                      value={newLightSchedule.offMinute}
                      onChange={(e) =>
                        setNewLightSchedule({ ...newLightSchedule, offMinute: Number.parseInt(e.target.value) })
                      }
                      className="bg-background border-border text-foreground"
                    />
                  </div>
                </div>
              </div>
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
            <div className="space-y-4">
              <div>
                <h4 className="text-sm font-semibold text-foreground mb-3">Włączenie</h4>
                <div className="grid grid-cols-2 gap-4">
                  <div>
                    <Label htmlFor="filter-on-hour" className="text-foreground">
                      Godzina
                    </Label>
                    <Input
                      id="filter-on-hour"
                      type="number"
                      min={0}
                      max={23}
                      value={newFilterSchedule.onHour}
                      onChange={(e) =>
                        setNewFilterSchedule({ ...newFilterSchedule, onHour: Number.parseInt(e.target.value) })
                      }
                      className="bg-background border-border text-foreground"
                    />
                  </div>
                  <div>
                    <Label htmlFor="filter-on-minute" className="text-foreground">
                      Minuta
                    </Label>
                    <Input
                      id="filter-on-minute"
                      type="number"
                      min={0}
                      max={59}
                      value={newFilterSchedule.onMinute}
                      onChange={(e) =>
                        setNewFilterSchedule({ ...newFilterSchedule, onMinute: Number.parseInt(e.target.value) })
                      }
                      className="bg-background border-border text-foreground"
                    />
                  </div>
                </div>
              </div>

              <div>
                <h4 className="text-sm font-semibold text-foreground mb-3">Wyłączenie</h4>
                <div className="grid grid-cols-2 gap-4">
                  <div>
                    <Label htmlFor="filter-off-hour" className="text-foreground">
                      Godzina
                    </Label>
                    <Input
                      id="filter-off-hour"
                      type="number"
                      min={0}
                      max={23}
                      value={newFilterSchedule.offHour}
                      onChange={(e) =>
                        setNewFilterSchedule({ ...newFilterSchedule, offHour: Number.parseInt(e.target.value) })
                      }
                      className="bg-background border-border text-foreground"
                    />
                  </div>
                  <div>
                    <Label htmlFor="filter-off-minute" className="text-foreground">
                      Minuta
                    </Label>
                    <Input
                      id="filter-off-minute"
                      type="number"
                      min={0}
                      max={59}
                      value={newFilterSchedule.offMinute}
                      onChange={(e) =>
                        setNewFilterSchedule({ ...newFilterSchedule, offMinute: Number.parseInt(e.target.value) })
                      }
                      className="bg-background border-border text-foreground"
                    />
                  </div>
                </div>
              </div>
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

        <TabsContent value="feeder" className="space-y-4">
          <div className="p-4 bg-secondary/50 rounded-lg space-y-4">
            <div>
              <h4 className="text-sm font-semibold text-foreground mb-3">Czas Karmienia</h4>
              <div className="grid grid-cols-2 gap-4">
                <div>
                  <Label htmlFor="feeder-hour" className="text-foreground">
                    Godzina
                  </Label>
                  <Input
                    id="feeder-hour"
                    type="number"
                    min={0}
                    max={23}
                    value={newFeederSchedule.hour}
                    onChange={(e) =>
                      setNewFeederSchedule({ ...newFeederSchedule, hour: Number.parseInt(e.target.value) })
                    }
                    className="bg-background border-border text-foreground"
                  />
                </div>
                <div>
                  <Label htmlFor="feeder-minute" className="text-foreground">
                    Minuta
                  </Label>
                  <Input
                    id="feeder-minute"
                    type="number"
                    min={0}
                    max={59}
                    value={newFeederSchedule.minute}
                    onChange={(e) =>
                      setNewFeederSchedule({ ...newFeederSchedule, minute: Number.parseInt(e.target.value) })
                    }
                    className="bg-background border-border text-foreground"
                  />
                </div>
              </div>
            </div>

            <Button
              onClick={handleAddFeederSchedule}
              disabled={!ble}
              className="w-full bg-primary text-primary-foreground hover:bg-primary/90"
            >
              <Plus className="mr-2 h-4 w-4" />
              Dodaj Harmonogram
            </Button>
          </div>

          {feederSchedules.length > 0 && (
            <div className="space-y-2">
              <h4 className="text-sm font-medium text-foreground">Aktywne Harmonogramy</h4>
              {feederSchedules.map((schedule) => (
                <div key={schedule.id} className="flex items-center justify-between p-3 bg-secondary/50 rounded-lg">
                  <div>
                    <p className="font-medium text-foreground">
                      {schedule.hour.toString().padStart(2, "0")}:{schedule.minute.toString().padStart(2, "0")}
                    </p>
                    <p className="text-xs text-muted-foreground">Karmienie</p>
                  </div>
                  <Button
                    onClick={() => handleDeleteFeederSchedule(schedule.id)}
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
