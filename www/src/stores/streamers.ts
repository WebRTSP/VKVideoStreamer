import { ref } from 'vue'
import { defineStore } from 'pinia'

declare const APIPort: number

interface Streamer {
  id: string
  description: string
  sourceUrl: string
  enabled: boolean
  sendingUpdate: boolean
}

export const useStreamersStore = defineStore('streamers', () => {
  const fetchingStreamers = ref(false)

  const streamers = ref<Streamer[]>([])

  const apiUrl = `${window.location.protocol}//${window.location.hostname}:${APIPort}/api/streamers`

  async function fetchStreamers() {
    if(fetchingStreamers.value) return

    fetchingStreamers.value = true
    try {
      const response = await fetch(apiUrl)
      if(!response.ok) return // FIXME?

      // eslint-disable-next-line @typescript-eslint/no-explicit-any
      const fetchedStreamers = (await response.json()).map((inStreamer: any) => {
        return {
          id: inStreamer.id,
          description: inStreamer.description,
          sourceUrl: inStreamer.source,
          enabled: inStreamer.enabled,
          sendingUpdate: false,
        }
      })

      streamers.value = fetchedStreamers
    } catch(error: unknown) {
      console.error(`${error}`)
    } finally {
      fetchingStreamers.value = false
    }
  }

  async function toggleStreaming(streamerId: string) {
    const streamer = streamers.value.find((streamer) => streamer.id == streamerId)
    if(!streamer) return

    try {
      streamer.sendingUpdate = true
      const newEnabled = !streamer.enabled
      const response = await fetch(`${apiUrl}/${streamer.id}`, {
        method: "PATCH",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({ enable: newEnabled }),
      })
      if(!response.ok) return // FIXME?

      streamer.enabled = newEnabled
    } catch(error: unknown) {
      console.error(`${error}`)
    } finally {
      streamer.sendingUpdate = false
    }
  }

  fetchStreamers()

  return { fetchingStreamers, fetchStreamers, streamers, toggleStreaming }
})
