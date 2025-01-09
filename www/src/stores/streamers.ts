import { ref } from 'vue'
import { defineStore } from 'pinia'

declare const APIPort: number

interface Streamer {
  id: string
  description: string
  sourceUrl: string
  enabled: boolean
}

export const useStreamersStore = defineStore('streamers', () => {
  const fetchingStreamers = ref(false)

  const streamers = ref<Streamer[]>([])

  async function fetchStreamers() {
    if(fetchingStreamers.value) return

    fetchingStreamers.value = true
    try {
      const url = `${window.location.protocol}//${window.location.hostname}:${APIPort}/api/streamers`
      const response = await fetch(url)
      if(!response.ok) return // FIXME?

      // eslint-disable-next-line @typescript-eslint/no-explicit-any
      const fetchedStreamers = (await response.json()).map((inStreamer: any) => {
        return {
          id: inStreamer.id,
          description: inStreamer.description,
          sourceUrl: inStreamer.source,
          enabled: inStreamer.enabled,
        }
      })

      streamers.value = fetchedStreamers
    } catch(error: unknown) {
      console.error(`${error}`)
    } finally {
      fetchingStreamers.value = false
    }
  }

  fetchStreamers()

  return { fetchingStreamers, fetchStreamers, streamers }
})
