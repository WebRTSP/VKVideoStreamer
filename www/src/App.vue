<script setup lang="ts">
import { useStreamersStore } from './stores/streamers';
import Player from './WebRTSPPlayer.vue'

const streamers = useStreamersStore()
</script>

<template>
  <Card class="card" v-for="streamer of streamers.streamers" :key="streamer.id">
    <template #header>
      <div class="card-header-container">
        <Player :uri="streamer.sourceUrl"/>
      </div>
    </template>
    <template #title>
      <span
        class="title-subtitle"
        :title="streamer.sourceUrl"
      >
        {{ streamer.sourceUrl }}
      </span>
    </template>
    <template #subtitle>
      <span
        class="title-subtitle"
        :title="streamer.description"
      >
        {{ streamer.description }}
      </span>
    </template>
    <template #footer>
      <div class="card-footer">
        <Button
          :icon="streamer.enabled ? 'pi pi-stop-circle' : 'pi pi-play-circle'"
          :loading="streamer.sendingUpdate"
          variant="text"
          rounded
          size="large"
          :severity="streamer.enabled ? 'warn' : 'success'"
          @click="streamers.toggleStreaming(streamer.id)"/>
      </div>
    </template>
  </Card>
</template>

<style scoped>
  .card {
    width: var(--card-width);
    overflow: hidden;
  }

  .card-header-container {
    position: relative;
    background-color: var(--p-surface-300);
    height: calc(var(--card-width) / var(--preview-aspect-ratio));
  }

  .title-subtitle {
    display: inline-block;
    width: 100%;
    overflow: hidden;
    text-overflow:ellipsis;
  }

  .card-footer {
    text-align: center;
  }
</style>
