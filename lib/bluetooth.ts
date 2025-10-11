export interface AquariumDevice {
  device: BluetoothDevice
  server?: BluetoothRemoteGATTServer
  characteristics?: {
    temperature: BluetoothRemoteGATTCharacteristic
    light: BluetoothRemoteGATTCharacteristic
    filter: BluetoothRemoteGATTCharacteristic
    heater: BluetoothRemoteGATTCharacteristic
    feeder: BluetoothRemoteGATTCharacteristic
    servo: BluetoothRemoteGATTCharacteristic
    quietMode: BluetoothRemoteGATTCharacteristic
    timeSync: BluetoothRemoteGATTCharacteristic
    scheduleLight: BluetoothRemoteGATTCharacteristic
    scheduleFilter: BluetoothRemoteGATTCharacteristic
    scheduleFeeder: BluetoothRemoteGATTCharacteristic
    feedNow: BluetoothRemoteGATTCharacteristic
  }
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

// ESP32-S3 BLE Service and Characteristic UUIDs
const SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
const CHAR_TEMPERATURE = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
const CHAR_LIGHT = "beb5483e-36e1-4688-b7f5-ea07361b26a9"
const CHAR_FILTER = "beb5483e-36e1-4688-b7f5-ea07361b26aa"
const CHAR_HEATER = "beb5483e-36e1-4688-b7f5-ea07361b26ab"
const CHAR_FEEDER = "beb5483e-36e1-4688-b7f5-ea07361b26ac"
const CHAR_SERVO = "beb5483e-36e1-4688-b7f5-ea07361b26ad"
const CHAR_QUIET_MODE = "beb5483e-36e1-4688-b7f5-ea07361b26ae"
const CHAR_TIME_SYNC = "beb5483e-36e1-4688-b7f5-ea07361b26af"
const CHAR_SCHEDULE_LIGHT = "beb5483e-36e1-4688-b7f5-ea07361b26b0"
const CHAR_SCHEDULE_FILTER = "beb5483e-36e1-4688-b7f5-ea07361b26b1"
const CHAR_SCHEDULE_FEEDER = "beb5483e-36e1-4688-b7f5-ea07361b26b2"
const CHAR_FEED_NOW = "beb5483e-36e1-4688-b7f5-ea07361b26b3"

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

      // Get all characteristics
      const characteristics = {
        temperature: await service.getCharacteristic(CHAR_TEMPERATURE),
        light: await service.getCharacteristic(CHAR_LIGHT),
        filter: await service.getCharacteristic(CHAR_FILTER),
        heater: await service.getCharacteristic(CHAR_HEATER),
        feeder: await service.getCharacteristic(CHAR_FEEDER),
        servo: await service.getCharacteristic(CHAR_SERVO),
        quietMode: await service.getCharacteristic(CHAR_QUIET_MODE),
        timeSync: await service.getCharacteristic(CHAR_TIME_SYNC),
        scheduleLight: await service.getCharacteristic(CHAR_SCHEDULE_LIGHT),
        scheduleFilter: await service.getCharacteristic(CHAR_SCHEDULE_FILTER),
        scheduleFeeder: await service.getCharacteristic(CHAR_SCHEDULE_FEEDER),
        feedNow: await service.getCharacteristic(CHAR_FEED_NOW),
      }

      // Setup temperature notifications
      await characteristics.temperature.startNotifications()
      characteristics.temperature.addEventListener("characteristicvaluechanged", (event) => {
        this.handleTemperatureNotification(event)
      })

      this.device = { device, server, characteristics }

      // Read initial states
      await this.readInitialStates()

      return this.device
    } catch (error) {
      console.error("[v0] BLE connection error:", error)
      throw error
    }
  }

  private async readInitialStates(): Promise<void> {
    if (!this.device?.characteristics) return

    try {
      // Read all device states
      const lightValue = await this.device.characteristics.light.readValue()
      const filterValue = await this.device.characteristics.filter.readValue()
      const heaterValue = await this.device.characteristics.heater.readValue()
      const feederValue = await this.device.characteristics.feeder.readValue()
      const servoValue = await this.device.characteristics.servo.readValue()
      const quietValue = await this.device.characteristics.quietMode.readValue()

      this.statusCallback?.({
        lightOn: lightValue.getUint8(0) === 1,
        pumpOn: filterValue.getUint8(0) === 1,
        heaterOn: heaterValue.getUint8(0) === 1,
        feederOn: feederValue.getUint8(0) === 1,
        servoPosition: servoValue.getUint8(0),
        quietMode: quietValue.getUint8(0) === 1,
      })
    } catch (error) {
      console.error("[v0] Error reading initial states:", error)
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

  private handleTemperatureNotification(event: Event): void {
    const target = event.target as BluetoothRemoteGATTCharacteristic
    const value = target.value
    if (!value) return

    // Temperature is sent as float (4 bytes)
    const temp = value.getFloat32(0, true) // true = little endian
    console.log("[v0] Temperature received:", temp)
    this.statusCallback?.({ temperature: temp })
  }

  // Device control commands
  async setLight(on: boolean): Promise<void> {
    if (!this.device?.characteristics) throw new Error("Device not connected")

    const value = new Uint8Array([on ? 1 : 0])
    await this.device.characteristics.light.writeValue(value)
    console.log("[v0] Light set to:", on)

    this.statusCallback?.({ lightOn: on })
  }

  async setPump(on: boolean): Promise<void> {
    if (!this.device?.characteristics) throw new Error("Device not connected")

    const value = new Uint8Array([on ? 1 : 0])
    await this.device.characteristics.filter.writeValue(value)
    console.log("[v0] Filter set to:", on)

    this.statusCallback?.({ pumpOn: on })
  }

  async setHeater(on: boolean): Promise<void> {
    if (!this.device?.characteristics) throw new Error("Device not connected")

    const value = new Uint8Array([on ? 1 : 0])
    await this.device.characteristics.heater.writeValue(value)
    console.log("[v0] Heater set to:", on)

    this.statusCallback?.({ heaterOn: on })
  }

  async setFeeder(on: boolean): Promise<void> {
    if (!this.device?.characteristics) throw new Error("Device not connected")

    const value = new Uint8Array([on ? 1 : 0])
    await this.device.characteristics.feeder.writeValue(value)
    console.log("[v0] Feeder set to:", on)

    this.statusCallback?.({ feederOn: on })
  }

  async setServo(position: number): Promise<void> {
    if (!this.device?.characteristics) throw new Error("Device not connected")

    const value = new Uint8Array([Math.max(0, Math.min(180, position))])
    await this.device.characteristics.servo.writeValue(value)
    console.log("[v0] Servo set to:", position)

    this.statusCallback?.({ servoPosition: position })
  }

  async setQuietMode(on: boolean, duration = 30): Promise<void> {
    if (!this.device?.characteristics) throw new Error("Device not connected")

    const encoder = new TextEncoder()
    const value = encoder.encode(on ? duration.toString() : "0")
    await this.device.characteristics.quietMode.writeValue(value)
    console.log("[v0] Quiet mode set to:", on, "duration:", duration)

    this.statusCallback?.({ quietMode: on })
  }

  async feedNow(): Promise<void> {
    if (!this.device?.characteristics) throw new Error("Device not connected")

    const value = new Uint8Array([1]) // Any value triggers feeding
    await this.device.characteristics.feedNow.writeValue(value)
    console.log("[v0] Feed now triggered")
  }

  async syncTime(): Promise<void> {
    if (!this.device?.characteristics) throw new Error("Device not connected")

    const now = new Date()
    const year = now.getFullYear()
    const month = (now.getMonth() + 1).toString().padStart(2, "0")
    const day = now.getDate().toString().padStart(2, "0")
    const hour = now.getHours().toString().padStart(2, "0")
    const minute = now.getMinutes().toString().padStart(2, "0")
    const second = now.getSeconds().toString().padStart(2, "0")

    const timeStr = `${year}-${month}-${day} ${hour}:${minute}:${second}`
    const encoder = new TextEncoder()
    const value = encoder.encode(timeStr)

    await this.device.characteristics.timeSync.writeValue(value)
    console.log("[v0] Time synced:", timeStr)
  }

  async setLightSchedule(onHour: number, onMinute: number, offHour: number, offMinute: number): Promise<void> {
    if (!this.device?.characteristics) throw new Error("Device not connected")

    const onTime = `${onHour.toString().padStart(2, "0")}:${onMinute.toString().padStart(2, "0")}`
    const offTime = `${offHour.toString().padStart(2, "0")}:${offMinute.toString().padStart(2, "0")}`
    const scheduleStr = `${onTime},${offTime}`

    const encoder = new TextEncoder()
    const value = encoder.encode(scheduleStr)
    await this.device.characteristics.scheduleLight.writeValue(value)
    console.log("[v0] Light schedule set:", scheduleStr)
  }

  async setFilterSchedule(onHour: number, onMinute: number, offHour: number, offMinute: number): Promise<void> {
    if (!this.device?.characteristics) throw new Error("Device not connected")

    const onTime = `${onHour.toString().padStart(2, "0")}:${onMinute.toString().padStart(2, "0")}`
    const offTime = `${offHour.toString().padStart(2, "0")}:${offMinute.toString().padStart(2, "0")}`
    const scheduleStr = `${onTime},${offTime}`

    const encoder = new TextEncoder()
    const value = encoder.encode(scheduleStr)
    await this.device.characteristics.scheduleFilter.writeValue(value)
    console.log("[v0] Filter schedule set:", scheduleStr)
  }

  async setFeederSchedule(hour: number, minute: number): Promise<void> {
    if (!this.device?.characteristics) throw new Error("Device not connected")

    const timeStr = `${hour.toString().padStart(2, "0")}:${minute.toString().padStart(2, "0")}`

    const encoder = new TextEncoder()
    const value = encoder.encode(timeStr)
    await this.device.characteristics.scheduleFeeder.writeValue(value)
    console.log("[v0] Feeder schedule set:", timeStr)
  }

  // Legacy methods for compatibility (not used with new ESP32 code)
  async requestStatus(): Promise<void> {
    // Not needed - we read states directly from characteristics
    await this.readInitialStates()
  }

  async setSchedule(entry: ScheduleEntry): Promise<void> {
    // Legacy method - use specific schedule methods instead
    console.warn("[v0] setSchedule is deprecated, use specific schedule methods")
  }

  async deleteSchedule(id: number): Promise<void> {
    // Not implemented in ESP32 code yet
    console.warn("[v0] deleteSchedule not implemented in ESP32")
  }
}
