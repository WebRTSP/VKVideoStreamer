<script setup lang="ts">
import { ProgressSpinner } from 'primevue';
import { ref, useTemplateRef, onMounted } from 'vue'
import { WebRTSP } from 'webrtsp/WebRTSP.mjs'

const { uri } = defineProps<{ uri: string }>()

declare const WebRTSPPort: number

const protocol = window.location.protocol === 'http:' ? "ws:" : "wss:"
const url = `${protocol}//${window.location.hostname}:${WebRTSPPort}/`

const videoElement = useTemplateRef('video')
const canplay = ref(false)
let webrtsp = null

onMounted(() => {
  webrtsp = new WebRTSP(
    videoElement.value,
    [{
      urls: ["stun:stun.l.google.com:19302"]
    }],
    { debug: true })

  webrtsp.events.addEventListener("disconnected", () => {
    canplay.value = false
  })

  videoElement.value?.addEventListener('canplay', () => {
    canplay.value = true
  })

  webrtsp.connect(url, uri)
})
</script>

<template>
  <video ref="video" autoplay="true" muted="true" v-show="canplay"> </video>
  <ProgressSpinner class="spinner" v-if="!canplay"/>
</template>

<style scoped>
  video {
    position: absolute;

    max-width: 100%;
    max-height: 100%;
    width: 100%;
    height: 100%;

    top: 0px;
    bottom: 0px;
    left: 0px;
    right: 0px;

    margin: auto;
  }

  .spinner {
    position: absolute;

    width: 25%;
    height: 25%;

    top: 0px;
    bottom: 0px;
    left: 0px;
    right: 0px;

    margin: auto;
  }
</style>
