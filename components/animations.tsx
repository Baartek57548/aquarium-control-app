"use client"

import { useEffect } from "react"

interface FeedAnimationProps {
  show: boolean
  onComplete: () => void
}

export function FeedAnimation({ show, onComplete }: FeedAnimationProps) {
  useEffect(() => {
    if (show) {
      const timer = setTimeout(onComplete, 2000)
      return () => clearTimeout(timer)
    }
  }, [show, onComplete])

  if (!show) return null

  return (
    <div className="fixed inset-0 pointer-events-none z-50 overflow-hidden">
      {Array.from({ length: 30 }).map((_, i) => (
        <div
          key={i}
          className="absolute animate-feed-fall"
          style={{
            left: `${Math.random() * 100}%`,
            top: "-20px",
            animationDelay: `${Math.random() * 0.5}s`,
            animationDuration: `${1.5 + Math.random() * 0.5}s`,
          }}
        >
          <div
            className="w-2 h-2 rounded-full bg-gradient-to-br from-yellow-400 to-orange-500 shadow-lg shadow-orange-500/50"
            style={{
              transform: `scale(${0.5 + Math.random() * 0.5})`,
            }}
          />
        </div>
      ))}
    </div>
  )
}

interface QuietModeAnimationProps {
  show: boolean
}

export function QuietModeAnimation({ show }: QuietModeAnimationProps) {
  if (!show) return null

  return (
    <div className="fixed inset-0 pointer-events-none z-40 overflow-hidden">
      {Array.from({ length: 5 }).map((_, i) => (
        <div
          key={i}
          className="absolute inset-0 animate-quiet-wave opacity-20"
          style={{
            animationDelay: `${i * 0.3}s`,
          }}
        >
          <div className="absolute inset-0 bg-gradient-to-b from-purple-500/30 to-transparent rounded-full blur-3xl transform scale-150" />
        </div>
      ))}
    </div>
  )
}

interface DeviceToggleAnimationProps {
  show: boolean
  color: "yellow" | "blue" | "red"
  onComplete: () => void
}

export function DeviceToggleAnimation({ show, color, onComplete }: DeviceToggleAnimationProps) {
  useEffect(() => {
    if (show) {
      const timer = setTimeout(onComplete, 1000)
      return () => clearTimeout(timer)
    }
  }, [show, onComplete])

  if (!show) return null

  const colorMap = {
    yellow: "from-yellow-400 to-orange-500",
    blue: "from-blue-400 to-cyan-500",
    red: "from-red-400 to-pink-500",
  }

  return (
    <div className="fixed inset-0 pointer-events-none z-40 flex items-center justify-center">
      <div
        className={`animate-device-pulse w-32 h-32 rounded-full bg-gradient-to-br ${colorMap[color]} opacity-50 blur-3xl`}
      />
    </div>
  )
}

interface BubbleAnimationProps {
  show: boolean
}

export function BubbleAnimation({ show }: BubbleAnimationProps) {
  if (!show) return null

  return (
    <div className="fixed inset-0 pointer-events-none z-40 overflow-hidden">
      {Array.from({ length: 20 }).map((_, i) => (
        <div
          key={i}
          className="absolute animate-bubble-rise"
          style={{
            left: `${20 + Math.random() * 60}%`,
            bottom: "-20px",
            animationDelay: `${Math.random() * 2}s`,
            animationDuration: `${3 + Math.random() * 2}s`,
          }}
        >
          <div
            className="rounded-full bg-gradient-to-br from-cyan-400/40 to-blue-500/40 border border-cyan-300/30 backdrop-blur-sm"
            style={{
              width: `${10 + Math.random() * 20}px`,
              height: `${10 + Math.random() * 20}px`,
            }}
          />
        </div>
      ))}
    </div>
  )
}
