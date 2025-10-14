const CACHE_NAME = "aquarium-control-v1"
const urlsToCache = ["/", "/manifest.json", "/icon-192.jpg", "/icon-512.jpg", "/images/aquarium-background.jpg"]

// Instalacja Service Workera
self.addEventListener("install", (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME).then((cache) => {
      console.log("Opened cache")
      return cache.addAll(urlsToCache)
    }),
  )
  self.skipWaiting()
})

// Aktywacja Service Workera
self.addEventListener("activate", (event) => {
  event.waitUntil(
    caches.keys().then((cacheNames) => {
      return Promise.all(
        cacheNames.map((cacheName) => {
          if (cacheName !== CACHE_NAME) {
            console.log("Deleting old cache:", cacheName)
            return caches.delete(cacheName)
          }
        }),
      )
    }),
  )
  self.clients.claim()
})

// Obsługa żądań - strategia Cache First z fallbackiem do sieci
self.addEventListener("fetch", (event) => {
  event.respondWith(
    caches.match(event.request).then((response) => {
      // Zwróć z cache jeśli dostępne
      if (response) {
        return response
      }

      // Sklonuj żądanie
      const fetchRequest = event.request.clone()

      return fetch(fetchRequest)
        .then((response) => {
          // Sprawdź czy odpowiedź jest prawidłowa
          if (!response || response.status !== 200 || response.type !== "basic") {
            return response
          }

          // Sklonuj odpowiedź
          const responseToCache = response.clone()

          // Dodaj do cache
          caches.open(CACHE_NAME).then((cache) => {
            cache.put(event.request, responseToCache)
          })

          return response
        })
        .catch(() => {
          // Jeśli offline i nie ma w cache, zwróć podstawową stronę
          return caches.match("/")
        })
    }),
  )
})
