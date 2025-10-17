"use client"

import type React from "react"

import { useState } from "react"
import { useRouter } from "next/navigation"
import { Button } from "@/components/ui/button"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"
import { Card } from "@/components/ui/card"
import { Waves } from "lucide-react"

export default function LoginPage() {
  const router = useRouter()
  const [username, setUsername] = useState("")
  const [password, setPassword] = useState("")
  const [error, setError] = useState("")

  const handleLogin = (e: React.FormEvent) => {
    e.preventDefault()

    if (username === "Admin" && password === "0000") {
      localStorage.setItem("aquarium_auth", "true")
      router.push("/")
    } else {
      setError("Nieprawidłowa nazwa użytkownika lub hasło")
    }
  }

  return (
    <>
      <div className="aquarium-bg" />
      <div className="min-h-screen flex items-center justify-center p-4">
        <Card className="glass-card w-full max-w-md p-8">
          <div className="flex flex-col items-center mb-8">
            <div className="p-4 bg-gradient-to-br from-cyan-500 to-blue-600 rounded-2xl shadow-lg mb-4">
              <Waves className="h-12 w-12 text-white" />
            </div>
            <h1 className="text-2xl font-bold text-white">Sterownik Akwarium</h1>
            <p className="text-cyan-300 mt-2">Zaloguj się aby kontynuować</p>
          </div>

          <form onSubmit={handleLogin} className="space-y-4">
            <div>
              <Label htmlFor="username" className="text-white">
                Nazwa użytkownika
              </Label>
              <Input
                id="username"
                type="text"
                value={username}
                onChange={(e) => setUsername(e.target.value)}
                autoComplete="username"
                className="mt-2 bg-gray-800/50 border-gray-700 text-white"
                placeholder="Admin"
              />
            </div>

            <div>
              <Label htmlFor="password" className="text-white">
                Hasło
              </Label>
              <Input
                id="password"
                type="password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                autoComplete="current-password"
                className="mt-2 bg-gray-800/50 border-gray-700 text-white"
                placeholder="0000"
              />
            </div>

            {error && <p className="text-red-400 text-sm">{error}</p>}

            <Button type="submit" className="w-full bg-gradient-to-r from-cyan-500 to-blue-600">
              Zaloguj się
            </Button>
          </form>
        </Card>
      </div>
    </>
  )
}
