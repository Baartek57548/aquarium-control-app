"use client"
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

    try {
      await ble.setLightSchedule(
        newLightSchedule.onHour,
        newLightSchedule.onMinute,
        newLightSchedule.offHour,
        newLightSchedule.offMinute,
      )

      // Add to local state for display
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
      setLightSchedules([...lightSchedules, onEntry, offEntry])
    } catch (error) {
      console.error("[v0] Light schedule add error:", error)
    }
  }

  const handleAddFilterSchedule = async () => {
    if (!ble) return

    try {
      await ble.setFilterSchedule(
        newFilterSchedule.onHour,
        newFilterSchedule.onMinute,
        newFilterSchedule.offHour,
        newFilterSchedule.offMinute,
      )

      // Add to local state for display
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

    try {
      await ble.setFeederSchedule(newFeederSchedule.hour, newFeederSchedule.minute)

      // Add to local state for display
      const entry: ScheduleEntry = {
        id: feederSchedules.length + 200,
        hour: newFeederSchedule.hour,
        minute: newFeederSchedule.minute,
        lightOn: false,
        pumpOn: false,
        heaterOn: false,
      }
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
    <div className="glass-card rounded-2xl p-6">
      <div className="flex items-center justify-between mb-6">
        <div className="flex items-center gap-3">
          <div className="p-3 bg-gradient-to-br from-cyan-500 to-blue-600 rounded-xl shadow-lg shadow-cyan-500/30">
            <Clock className="h-6 w-6 text-white" />
          </div>
          <div>
            <h3 className="font-bold text-white text-lg">Harmonogram</h3>
            <p className="text-sm text-cyan-300">Automatyzacja akwarium</p>
          </div>
        </div>
        <Button
          onClick={handleSyncTime}
          disabled={!ble}
          size="sm"
          className="bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-600 hover:to-blue-700 text-white shadow-lg shadow-cyan-500/20"
        >
          Synchronizuj
        </Button>
      </div>

      <Tabs defaultValue="light" className="w-full">
        <TabsList className="grid w-full grid-cols-3 mb-6 bg-gray-900/50 p-1 rounded-xl">
          <TabsTrigger
            value="light"
            className="flex items-center gap-2 data-[state=active]:bg-gradient-to-r data-[state=active]:from-yellow-500 data-[state=active]:to-orange-500 data-[state=active]:text-white rounded-lg"
          >
            <Lightbulb className="h-4 w-4" />
            <span className="hidden sm:inline">Światło</span>
          </TabsTrigger>
          <TabsTrigger
            value="filter"
            className="flex items-center gap-2 data-[state=active]:bg-gradient-to-r data-[state=active]:from-blue-500 data-[state=active]:to-cyan-500 data-[state=active]:text-white rounded-lg"
          >
            <Waves className="h-4 w-4" />
            <span className="hidden sm:inline">Filtr</span>
          </TabsTrigger>
          <TabsTrigger
            value="feeder"
            className="flex items-center gap-2 data-[state=active]:bg-gradient-to-r data-[state=active]:from-green-500 data-[state=active]:to-emerald-500 data-[state=active]:text-white rounded-lg"
          >
            <Fish className="h-4 w-4" />
            <span className="hidden sm:inline">Karmnik</span>
          </TabsTrigger>
        </TabsList>

        <TabsContent value="light" className="space-y-4">
          <div className="bg-gradient-to-br from-gray-900/70 to-gray-800/50 rounded-xl p-5 border border-yellow-500/20 space-y-5">
            <div className="grid grid-cols-2 gap-4">
              <div className="space-y-3">
                <div className="flex items-center gap-2 mb-2">
                  <div className="w-2 h-2 rounded-full bg-green-500"></div>
                  <h4 className="text-sm font-bold text-white">Włączenie</h4>
                </div>
                <div className="space-y-2">
                  <Label htmlFor="light-on-hour" className="text-gray-300 text-xs">
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
                    className="bg-gray-900/80 border-gray-700 text-white text-center text-lg font-semibold h-12"
                  />
                </div>
                <div className="space-y-2">
                  <Label htmlFor="light-on-minute" className="text-gray-300 text-xs">
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
                    className="bg-gray-900/80 border-gray-700 text-white text-center text-lg font-semibold h-12"
                  />
                </div>
              </div>

              <div className="space-y-3">
                <div className="flex items-center gap-2 mb-2">
                  <div className="w-2 h-2 rounded-full bg-red-500"></div>
                  <h4 className="text-sm font-bold text-white">Wyłączenie</h4>
                </div>
                <div className="space-y-2">
                  <Label htmlFor="light-off-hour" className="text-gray-300 text-xs">
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
                    className="bg-gray-900/80 border-gray-700 text-white text-center text-lg font-semibold h-12"
                  />
                </div>
                <div className="space-y-2">
                  <Label htmlFor="light-off-minute" className="text-gray-300 text-xs">
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
                    className="bg-gray-900/80 border-gray-700 text-white text-center text-lg font-semibold h-12"
                  />
                </div>
              </div>
            </div>

            <Button
              onClick={handleAddLightSchedule}
              disabled={!ble}
              className="w-full bg-gradient-to-r from-yellow-500 to-orange-500 hover:from-yellow-600 hover:to-orange-600 text-white shadow-lg shadow-yellow-500/30 h-12 font-semibold"
            >
              <Plus className="mr-2 h-5 w-5" />
              Dodaj Harmonogram
            </Button>
          </div>

          {lightSchedules.length > 0 && (
            <div className="space-y-2">
              <h4 className="text-sm font-semibold text-white flex items-center gap-2">
                <Clock className="h-4 w-4 text-yellow-500" />
                Aktywne Harmonogramy
              </h4>
              {lightSchedules.map((schedule) => (
                <div
                  key={schedule.id}
                  className="flex items-center justify-between p-4 bg-gray-900/50 rounded-xl border border-gray-800 hover:border-yellow-500/30 transition-colors"
                >
                  <div className="flex items-center gap-3">
                    <div
                      className={`w-3 h-3 rounded-full ${schedule.lightOn ? "bg-green-500 animate-pulse" : "bg-red-500"}`}
                    ></div>
                    <div>
                      <p className="font-bold text-white text-lg">
                        {schedule.hour.toString().padStart(2, "0")}:{schedule.minute.toString().padStart(2, "0")}
                      </p>
                      <p className="text-xs text-gray-400">{schedule.lightOn ? "Włączenie" : "Wyłączenie"}</p>
                    </div>
                  </div>
                  <Button
                    onClick={() => handleDeleteLightSchedule(schedule.id)}
                    variant="ghost"
                    size="sm"
                    className="text-red-400 hover:text-red-300 hover:bg-red-500/10"
                  >
                    <Trash2 className="h-4 w-4" />
                  </Button>
                </div>
              ))}
            </div>
          )}
        </TabsContent>

        <TabsContent value="filter" className="space-y-4">
          <div className="bg-gradient-to-br from-gray-900/70 to-gray-800/50 rounded-xl p-5 border border-blue-500/20 space-y-5">
            <div className="grid grid-cols-2 gap-4">
              <div className="space-y-3">
                <div className="flex items-center gap-2 mb-2">
                  <div className="w-2 h-2 rounded-full bg-green-500"></div>
                  <h4 className="text-sm font-bold text-white">Włączenie</h4>
                </div>
                <div className="space-y-2">
                  <Label htmlFor="filter-on-hour" className="text-gray-300 text-xs">
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
                    className="bg-gray-900/80 border-gray-700 text-white text-center text-lg font-semibold h-12"
                  />
                </div>
                <div className="space-y-2">
                  <Label htmlFor="filter-on-minute" className="text-gray-300 text-xs">
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
                    className="bg-gray-900/80 border-gray-700 text-white text-center text-lg font-semibold h-12"
                  />
                </div>
              </div>

              <div className="space-y-3">
                <div className="flex items-center gap-2 mb-2">
                  <div className="w-2 h-2 rounded-full bg-red-500"></div>
                  <h4 className="text-sm font-bold text-white">Wyłączenie</h4>
                </div>
                <div className="space-y-2">
                  <Label htmlFor="filter-off-hour" className="text-gray-300 text-xs">
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
                    className="bg-gray-900/80 border-gray-700 text-white text-center text-lg font-semibold h-12"
                  />
                </div>
                <div className="space-y-2">
                  <Label htmlFor="filter-off-minute" className="text-gray-300 text-xs">
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
                    className="bg-gray-900/80 border-gray-700 text-white text-center text-lg font-semibold h-12"
                  />
                </div>
              </div>
            </div>

            <Button
              onClick={handleAddFilterSchedule}
              disabled={!ble}
              className="w-full bg-gradient-to-r from-blue-500 to-cyan-500 hover:from-blue-600 hover:to-cyan-600 text-white shadow-lg shadow-blue-500/30 h-12 font-semibold"
            >
              <Plus className="mr-2 h-5 w-5" />
              Dodaj Harmonogram
            </Button>
          </div>

          {filterSchedules.length > 0 && (
            <div className="space-y-2">
              <h4 className="text-sm font-semibold text-white flex items-center gap-2">
                <Clock className="h-4 w-4 text-blue-500" />
                Aktywne Harmonogramy
              </h4>
              {filterSchedules.map((schedule) => (
                <div
                  key={schedule.id}
                  className="flex items-center justify-between p-4 bg-gray-900/50 rounded-xl border border-gray-800 hover:border-blue-500/30 transition-colors"
                >
                  <div className="flex items-center gap-3">
                    <div
                      className={`w-3 h-3 rounded-full ${schedule.pumpOn ? "bg-green-500 animate-pulse" : "bg-red-500"}`}
                    ></div>
                    <div>
                      <p className="font-bold text-white text-lg">
                        {schedule.hour.toString().padStart(2, "0")}:{schedule.minute.toString().padStart(2, "0")}
                      </p>
                      <p className="text-xs text-gray-400">{schedule.pumpOn ? "Włączenie" : "Wyłączenie"}</p>
                    </div>
                  </div>
                  <Button
                    onClick={() => handleDeleteFilterSchedule(schedule.id)}
                    variant="ghost"
                    size="sm"
                    className="text-red-400 hover:text-red-300 hover:bg-red-500/10"
                  >
                    <Trash2 className="h-4 w-4" />
                  </Button>
                </div>
              ))}
            </div>
          )}
        </TabsContent>

        <TabsContent value="feeder" className="space-y-4">
          <div className="bg-gradient-to-br from-gray-900/70 to-gray-800/50 rounded-xl p-5 border border-green-500/20 space-y-5">
            <div className="flex items-center gap-2 mb-3">
              <div className="w-2 h-2 rounded-full bg-green-500"></div>
              <h4 className="text-sm font-bold text-white">Czas Karmienia</h4>
            </div>
            <div className="grid grid-cols-2 gap-4">
              <div className="space-y-2">
                <Label htmlFor="feeder-hour" className="text-gray-300 text-xs">
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
                  className="bg-gray-900/80 border-gray-700 text-white text-center text-lg font-semibold h-12"
                />
              </div>
              <div className="space-y-2">
                <Label htmlFor="feeder-minute" className="text-gray-300 text-xs">
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
                  className="bg-gray-900/80 border-gray-700 text-white text-center text-lg font-semibold h-12"
                />
              </div>
            </div>

            <Button
              onClick={handleAddFeederSchedule}
              disabled={!ble}
              className="w-full bg-gradient-to-r from-green-500 to-emerald-500 hover:from-green-600 hover:to-emerald-600 text-white shadow-lg shadow-green-500/30 h-12 font-semibold"
            >
              <Plus className="mr-2 h-5 w-5" />
              Dodaj Harmonogram
            </Button>
          </div>

          {feederSchedules.length > 0 && (
            <div className="space-y-2">
              <h4 className="text-sm font-semibold text-white flex items-center gap-2">
                <Clock className="h-4 w-4 text-green-500" />
                Aktywne Harmonogramy
              </h4>
              {feederSchedules.map((schedule) => (
                <div
                  key={schedule.id}
                  className="flex items-center justify-between p-4 bg-gray-900/50 rounded-xl border border-gray-800 hover:border-green-500/30 transition-colors"
                >
                  <div className="flex items-center gap-3">
                    <div className="w-3 h-3 rounded-full bg-green-500 animate-pulse"></div>
                    <div>
                      <p className="font-bold text-white text-lg">
                        {schedule.hour.toString().padStart(2, "0")}:{schedule.minute.toString().padStart(2, "0")}
                      </p>
                      <p className="text-xs text-gray-400">Karmienie</p>
                    </div>
                  </div>
                  <Button
                    onClick={() => handleDeleteFeederSchedule(schedule.id)}
                    variant="ghost"
                    size="sm"
                    className="text-red-400 hover:text-red-300 hover:bg-red-500/10"
                  >
                    <Trash2 className="h-4 w-4" />
                  </Button>
                </div>
              ))}
            </div>
          )}
        </TabsContent>
      </Tabs>
    </div>
  )
}
