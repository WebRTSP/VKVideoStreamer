<script setup lang="ts">
import { useStreamersStore } from './stores/streamers';

const streamers = useStreamersStore()
</script>

<template>
  <Card class="card" v-for="streamer of streamers.streamers" :key="streamer.id">
    <template #header>
      <div class="card-header-container">
        <img class="header-img" src="@/assets/video-camera-alt.svg"/>
      </div>
    </template>
    <template #title>
      <span
        class="title"
        :class="{ 'disabled-title': !streamer.enabled }"
        :title="streamer.sourceUrl"
      >
        {{ streamer.sourceUrl }}
      </span>
    </template>
    <template #subtitle>
      {{ streamer.description }}
    </template>
  </Card>
</template>

<style scoped>
  .card {
    width: var(--card-width);
    overflow: hidden;
  }

  .card-header-container {
    background-color: var(--p-surface-300);
    height: calc(var(--card-width) / var(--preview-aspect-ratio));
  }

  .header-img {
    display: block;
    margin: auto;
    height: 100%;
  }

  .disabled-title {
    text-decoration: line-through;
  }
</style>
