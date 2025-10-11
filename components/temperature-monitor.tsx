"use client"

import { Card } from "@/components/ui/card"
import { Thermometer } from "lucide-react"
import { useEffect, useState } from "react"
import { Line, LineChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from "recharts"

interface TemperatureMonitorProps {
  currentTemp: number
}

export function TemperatureMonitor({ currentTemp }: TemperatureMonitorProps) {
  const [history, setHistory] = useState<{ time: string; temp: number }[]>([])

  useEffect(() => {
    if (currentTemp > 0) {
      const now = new Date()
      const timeStr = `${now.getHours().toString().padStart(2, "0")}:${now.getMinutes().toString().padStart(2, "0")}`

      setHistory((prev) => {
        const newHistory = [...prev, { time: timeStr, temp: currentTemp }]
        return newHistory.slice(-20)
      })
    }
  }, [currentTemp])

  const getTempColor = (temp: number) => {
    if (temp < 22) return "text-blue-400"
    if (temp > 28) return "text-red-400"
    return "text-green-400"
  }

  return (
    <Card className="p-6 bg-card border-border">
      <div className="flex items-center justify-between mb-6">
        <div className="flex items-center gap-3">
          <div className="p-2 bg-primary/10 rounded-lg">
            <Thermometer className="h-6 w-6 text-primary" />
          </div>
          <div>
            <h3 className="font-semibold text-foreground">Temperatura</h3>
            <p className="text-sm text-muted-foreground">Monitoring w czasie rzeczywistym</p>
          </div>
        </div>
        <div className="text-right">
          <div className={`text-3xl font-bold ${getTempColor(currentTemp)}`}>
            {currentTemp > 0 ? `${currentTemp.toFixed(1)}°C` : "--°C"}
          </div>
          <p className="text-xs text-muted-foreground mt-1">Optymalna: 24-26°C</p>
        </div>
      </div>

      {history.length > 1 && (
        <div className="h-48 mt-4">
          <ResponsiveContainer width="100%" height="100%">
            <LineChart data={history}>
              <XAxis dataKey="time" stroke="hsl(var(--muted-foreground))" fontSize={12} tickLine={false} />
              <YAxis stroke="hsl(var(--muted-foreground))" fontSize={12} tickLine={false} domain={[20, 30]} />
              <Tooltip
                contentStyle={{
                  backgroundColor: "hsl(var(--card))",
                  border: "1px solid hsl(var(--border))",
                  borderRadius: "8px",
                }}
                labelStyle={{ color: "hsl(var(--foreground))" }}
              />
              <Line type="monotone" dataKey="temp" stroke="hsl(var(--primary))" strokeWidth={2} dot={false} />
            </LineChart>
          </ResponsiveContainer>
        </div>
      )}
    </Card>
  )
}
