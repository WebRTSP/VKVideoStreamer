<script setup lang="ts">
import { useStreamersStore } from './stores/streamers';
import Player from './WebRTSPPlayer.vue'

const streamers = useStreamersStore()
</script>

<template>
  <div id="streamers" v-if="streamers.streamers.length != 0">
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
  </div>
  <i v-else class="pi pi-ban stub"/>
</template>

<style scoped>
  #streamers {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: var(--cards-gap);
  }

  /* min-width: calc(var(--card-width) * var(--cards-in-row) + (var(--cards-in-row) + 1) * var(--cards-gap))*/
  @media (min-width: 79rem) {
    #streamers {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      grid-auto-flow: row;
      gap: var(--cards-gap);
    }
  }

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
    text-overflow: ellipsis;
  }

  .card-footer {
    text-align: center;
  }

  .stub {
    font-size: calc(var(--card-width) / 2);
    text-align: center;
    color: var(--p-surface-300);
    display: block;
    line-height: 100vh;
    margin: auto;
  }
</style>
