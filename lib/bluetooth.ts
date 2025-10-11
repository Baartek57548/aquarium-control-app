// Bluetooth communication utilities for aquarium controller

export interface AquariumDevice {
  device: BluetoothDevice
  server?: BluetoothRemoteGATTServer
  characteristic?: BluetoothRemoteGATTCharacteristic
}

export interface DeviceStatus {
  temperature: number
  lightOn: boolean
  pumpOn: boolean
  heaterOn: boolean
  feederOn: boolean
  servoPosition: number
  quietMode: boolean
  dateTime: string
}

export interface ScheduleEntry {
  id: number
  hour: number
  minute: number
  lightOn: boolean
  pumpOn: boolean
  heaterOn: boolean
}

const SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
const CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

export class AquariumBLE {
  private device: AquariumDevice | null = null
  private statusCallback: ((status: Partial<DeviceStatus>) => void) | null = null

  async connect(): Promise<AquariumDevice> {
    try {
      const device = await navigator.bluetooth.requestDevice({
        filters: [{ name: "AquariumController" }],
        optionalServices: [SERVICE_UUID],
      })

      const server = await device.gatt?.connect()
      if (!server) throw new Error("Failed to connect to GATT server")

      const service = await server.getPrimaryService(SERVICE_UUID)
      const characteristic = await service.getCharacteristic(CHARACTERISTIC_UUID)

      // Setup notifications
      await characteristic.startNotifications()
      characteristic.addEventListener("characteristicvaluechanged", (event) => {
        this.handleNotification(event)
      })

      this.device = { device, server, characteristic }
      return this.device
    } catch (error) {
      console.error("[v0] BLE connection error:", error)
      throw error
    }
  }

  async disconnect(): Promise<void> {
    if (this.device?.server?.connected) {
      this.device.server.disconnect()
    }
    this.device = null
  }

  isConnected(): boolean {
    return this.device?.server?.connected ?? false
  }

  onStatusUpdate(callback: (status: Partial<DeviceStatus>) => void): void {
    this.statusCallback = callback
  }

  private handleNotification(event: Event): void {
    const target = event.target as BluetoothRemoteGATTCharacteristic
    const value = target.value
    if (!value) return

    const decoder = new TextDecoder()
    const message = decoder.decode(value)

    console.log("[v0] Received from device:", message)

    // Parse status updates
    if (message.startsWith("TEMP:")) {
      const temp = Number.parseFloat(message.substring(5))
      this.statusCallback?.({ temperature: temp })
    } else if (message.startsWith("STATUS:")) {
      // Parse status message: STATUS:L1P1H0F0S90Q0
      const status = message.substring(7)
      this.statusCallback?.({
        lightOn: status[1] === "1",
        pumpOn: status[3] === "1",
        heaterOn: status[5] === "1",
        feederOn: status[7] === "1",
        servoPosition: Number.parseInt(status.substring(9, 11)),
        quietMode: status[12] === "1",
      })
    }
  }

  async sendCommand(command: string): Promise<void> {
    if (!this.device?.characteristic) {
      throw new Error("Device not connected")
    }

    const encoder = new TextEncoder()
    const data = encoder.encode(command)
    await this.device.characteristic.writeValue(data)
    console.log("[v0] Sent command:", command)
  }

  // Device control commands
  async setLight(on: boolean): Promise<void> {
    await this.sendCommand(`LIGHT:${on ? "1" : "0"}`)
  }

  async setPump(on: boolean): Promise<void> {
    await this.sendCommand(`PUMP:${on ? "1" : "0"}`)
  }

  async setHeater(on: boolean): Promise<void> {
    await this.sendCommand(`HEATER:${on ? "1" : "0"}`)
  }

  async setFeeder(on: boolean): Promise<void> {
    await this.sendCommand(`FEEDER:${on ? "1" : "0"}`)
  }

  async setServo(position: number): Promise<void> {
    await this.sendCommand(`SERVO:${position}`)
  }

  async setQuietMode(on: boolean): Promise<void> {
    await this.sendCommand(`QUIET:${on ? "1" : "0"}`)
  }

  async feedNow(): Promise<void> {
    await this.sendCommand("FEED")
  }

  async syncTime(): Promise<void> {
    const now = new Date()
    const timeStr = `TIME:${now.getFullYear()},${now.getMonth() + 1},${now.getDate()},${now.getHours()},${now.getMinutes()},${now.getSeconds()}`
    await this.sendCommand(timeStr)
  }

  async requestStatus(): Promise<void> {
    await this.sendCommand("GET_STATUS")
  }

  async setSchedule(entry: ScheduleEntry): Promise<void> {
    const scheduleStr = `SCHEDULE:${entry.id},${entry.hour},${entry.minute},${entry.lightOn ? "1" : "0"},${entry.pumpOn ? "1" : "0"},${entry.heaterOn ? "1" : "0"}`
    await this.sendCommand(scheduleStr)
  }

  async deleteSchedule(id: number): Promise<void> {
    await this.sendCommand(`DEL_SCHEDULE:${id}`)
  }
}
