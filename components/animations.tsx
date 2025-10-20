"use client"

import { useEffect } from "react"

interface FeedAnimationProps {
  show: boolean
  onComplete: () => void
}

export function FeedAnimation({ show, onComplete }: FeedAnimationProps) {
  useEffect(() => {
    if (show) {
      const timer = setTimeout(onComplete, 5000) // Extended to 5 seconds to match Arduino feeding duration
      return () => clearTimeout(timer)
    }
  }, [show, onComplete])

  if (!show) return null

  return (
    <div className="fixed inset-0 pointer-events-none z-50 overflow-hidden">
      {Array.from({ length: 5 }).map((_, i) => (
        <div
          key={`fish-${i}`}
          className="absolute animate-fish-swim"
          style={{
            top: `${20 + i * 15}%`,
            left: i % 2 === 0 ? "-100px" : "100%",
            animationDelay: `${i * 0.4}s`,
            animationDuration: `${3 + i * 0.5}s`,
            animationDirection: i % 2 === 0 ? "normal" : "reverse",
          }}
        >
          <div className="relative">
            {/* Fish body */}
            <div className="w-12 h-8 bg-gradient-to-r from-orange-400 via-orange-500 to-orange-600 rounded-full shadow-lg">
              {/* Eye */}
              <div className="absolute top-2 right-2 w-2 h-2 bg-white rounded-full">
                <div className="absolute top-0.5 right-0.5 w-1 h-1 bg-black rounded-full" />
              </div>
              {/* Tail */}
              <div className="absolute -left-4 top-1 w-6 h-6 bg-gradient-to-l from-orange-500 to-orange-400 transform rotate-45 rounded-tl-full" />
              {/* Fins */}
              <div className="absolute top-0 left-4 w-3 h-4 bg-orange-400/70 rounded-full transform -rotate-12" />
              <div className="absolute bottom-0 left-4 w-3 h-4 bg-orange-400/70 rounded-full transform rotate-12" />
            </div>
          </div>
        </div>
      ))}

      {Array.from({ length: 50 }).map((_, i) => (
        <div
          key={`pellet-${i}`}
          className="absolute animate-feed-fall"
          style={{
            left: `${Math.random() * 100}%`,
            top: "-20px",
            animationDelay: `${Math.random() * 2}s`,
            animationDuration: `${2 + Math.random() * 1.5}s`,
          }}
        >
          <div
            className="rounded-full bg-gradient-to-br from-yellow-300 via-orange-400 to-red-500 shadow-lg shadow-orange-500/50 animate-pellet-spin"
            style={{
              width: `${6 + Math.random() * 6}px`,
              height: `${6 + Math.random() * 6}px`,
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
    <div className="fixed inset-0 pointer-events-none z-30 overflow-hidden">
      {Array.from({ length: 8 }).map((_, i) => (
        <div
          key={`wave-${i}`}
          className="absolute inset-0 animate-quiet-wave"
          style={{
            animationDelay: `${i * 0.2}s`,
            opacity: 0.15 - i * 0.015,
          }}
        >
          <div
            className="absolute inset-0 bg-gradient-to-b from-purple-500 via-violet-500 to-transparent rounded-full blur-3xl transform scale-150"
            style={{
              transform: `scale(${1.5 + i * 0.1}) translateY(${i * 5}%)`,
            }}
          />
        </div>
      ))}

      {Array.from({ length: 15 }).map((_, i) => (
        <div
          key={`particle-${i}`}
          className="absolute animate-float-particle"
          style={{
            left: `${Math.random() * 100}%`,
            top: `${Math.random() * 100}%`,
            animationDelay: `${Math.random() * 3}s`,
            animationDuration: `${4 + Math.random() * 2}s`,
          }}
        >
          <div
            className="w-2 h-2 rounded-full bg-purple-400/30 blur-sm"
            style={{
              boxShadow: "0 0 10px rgba(168, 85, 247, 0.4)",
            }}
          />
        </div>
      ))}

      <div className="absolute inset-0 bg-gradient-radial from-purple-500/10 via-transparent to-transparent animate-pulse-slow" />
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
      const timer = setTimeout(onComplete, 1500) // Extended duration for better visibility
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
      {Array.from({ length: 3 }).map((_, i) => (
        <div
          key={i}
          className={`absolute animate-device-pulse w-32 h-32 rounded-full bg-gradient-to-br ${colorMap[color]} blur-3xl`}
          style={{
            animationDelay: `${i * 0.2}s`,
            opacity: 0.5 - i * 0.15,
            transform: `scale(${1 + i * 0.3})`,
          }}
        />
      ))}
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
      {Array.from({ length: 30 }).map((_, i) => (
        <div
          key={i}
          className="absolute animate-bubble-rise"
          style={{
            left: `${20 + Math.random() * 60}%`,
            bottom: "-20px",
            animationDelay: `${Math.random() * 3}s`,
            animationDuration: `${3 + Math.random() * 2}s`,
          }}
        >
          <div
            className="rounded-full bg-gradient-to-br from-cyan-400/40 to-blue-500/40 border border-cyan-300/30 backdrop-blur-sm animate-bubble-wobble"
            style={{
              width: `${10 + Math.random() * 20}px`,
              height: `${10 + Math.random() * 20}px`,
              boxShadow: "0 0 10px rgba(34, 211, 238, 0.3), inset 0 0 5px rgba(255, 255, 255, 0.5)",
            }}
          />
        </div>
      ))}
    </div>
  )
}
